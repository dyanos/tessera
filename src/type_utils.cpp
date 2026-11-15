#include "type_utils.h"
#include "builtin_structs.h"

#include "codegen.h"
#include "IdentifierNode.h"
#include "ValueNode.h"
#include "token.h"

#include <llvm/IR/DerivedTypes.h>
#include <string>

namespace {
// An all-digit token (optionally signed) is an integer literal; identifiers can never be all digits.
bool isIntegerLiteralToken(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0] == '+' || s[0] == '-') ? 1 : 0;
    if (i >= s.size()) return false;
    for (; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

llvm::Function* natRuntimeFn(CodeGenerator& cg, const char* name, llvm::Type* ret, llvm::ArrayRef<llvm::Type*> params) {
    if (auto* f = cg.module->getFunction(name)) return f;
    auto* fnTy = llvm::FunctionType::get(ret, params, false);
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, name, cg.module.get());
}
}  // namespace

llvm::Value* coerceToNat(llvm::Value* value, const Type* declaredType, ASTNode* sourceExpr) {
    if (!declaredType || declaredType->getName() != "Nat") return value;
    auto& cg = CodeGenerator::getInstance();
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);

    // Already a Nat handle (a ptr): nothing to do.
    if (value && value->getType()->isPointerTy()) return value;

    // An integer literal becomes its exact big value via from_decimal -- crucially not routed through
    // the i64 codegen of the literal, which would overflow for a value beyond 2^63. Numeric literals
    // are ValueNode; a few contexts also surface a literal as an all-digit IdentifierNode.
    auto emitFromDecimal = [&](const std::string& digits) {
        llvm::Value* text = cg.builder.CreateGlobalString(digits, "nat.lit");
        return cg.builder.CreateCall(
            natRuntimeFn(cg, "csec_bigint_from_decimal", i8PtrTy, {i8PtrTy}), {text}, "nat.from.dec");
    };
    if (auto* lit = dynamic_cast<ValueNode*>(sourceExpr)) {
        if (lit->valueType == TokenType::INTEGER_LITERAL) {
            return emitFromDecimal(lit->value);
        }
    }
    if (auto* id = dynamic_cast<IdentifierNode*>(sourceExpr)) {
        if (isIntegerLiteralToken(id->value) && !cg.symbolTable.lookup(id->value)) {
            return emitFromDecimal(id->value);
        }
    }

    // A runtime integer value promotes through from_i64.
    if (value && value->getType()->isIntegerTy()) {
        llvm::Value* wide = value->getType()->isIntegerTy(64)
            ? value
            : cg.builder.CreateSExt(value, i64Ty, "nat.sext");
        return cg.builder.CreateCall(
            natRuntimeFn(cg, "csec_bigint_from_i64", i8PtrTy, {i64Ty}), {wide}, "nat.from.i64");
    }
    return value;
}


bool areTypesCompatible(const std::vector<std::unique_ptr<Type>>& paramTypes, const std::vector<std::unique_ptr<Type>>& argTypes) {
    if (paramTypes.size() != argTypes.size()) {
        return false;
    }

    for (size_t i = 0; i < paramTypes.size(); ++i) {
        if (!argTypes[i]->equals(paramTypes[i])) {
            return false;
        }
    }
    return true;
}

std::unique_ptr<Type> stripBorrow(const std::unique_ptr<Type>& type) {
    if (type && (type->getKind() == Type::Kind::BORROW ||
                 type->getKind() == Type::Kind::MUTABLE_BORROW)) {
        if (auto* bt = dynamic_cast<BorrowType*>(type.get())) {
            if (bt->baseType) return bt->baseType->clone();
        }
    }
    return type ? type->clone() : std::make_unique<UnknownType>();
}

std::string templateArgumentName(const Type* type) {
    if (!type) return "Unknown";
    if (auto* generic = dynamic_cast<const GenericType*>(type)) {
        std::string name = generic->baseType ? generic->baseType->getName() : generic->getName();
        for (const auto& argument : generic->typeArguments) name += "$" + templateArgumentName(argument.get());
        return name;
    }
    return type->getName();
}

bool isStructClassType(const Type* type) {
    if (!type || type->getKind() != Type::Kind::CLASS) {
        return false;
    }

    if (builtinValueStructFields(type->getName())) {
        return true; // Complex / Rational / Quaternion are value aggregates
    }
    auto& cg = CodeGenerator::getInstance();
    auto* classSymbol = cg.symbolTable.lookupClass(type->getName());
    return classSymbol && classSymbol->isStruct;
}

llvm::Type* getABIStorageType(const Type* type) {
    if (!type) {
        return nullptr;
    }

    auto& cg = CodeGenerator::getInstance();
    llvm::Type* llvmType = cg.getLLVMType(type);
    if (!llvmType) {
        return nullptr;
    }

    if (type->getKind() == Type::Kind::CLASS && !isStructClassType(type)) {
        return llvm::PointerType::getUnqual(llvmType->getContext());
    }

    return llvmType;
}

llvm::Value* coerceValueToLLVMType(llvm::Value* value, llvm::Type* targetType) {
    if (!value || !targetType || value->getType() == targetType) {
        return value;
    }

    auto& cg = CodeGenerator::getInstance();
    llvm::Type* sourceType = value->getType();
    if (sourceType->isPointerTy() && !targetType->isPointerTy()) {
        return cg.builder.CreateLoad(targetType, value, "abi.load");
    }

    if (sourceType->isIntegerTy() && targetType->isIntegerTy()) {
        unsigned sourceBits = sourceType->getIntegerBitWidth();
        unsigned targetBits = targetType->getIntegerBitWidth();
        if (sourceBits < targetBits) {
            return cg.builder.CreateSExt(value, targetType, "abi.sext");
        }
        if (sourceBits > targetBits) {
            return cg.builder.CreateTrunc(value, targetType, "abi.trunc");
        }
    }

    if (sourceType->isFloatingPointTy() && targetType->isFloatingPointTy()) {
        return cg.builder.CreateFPCast(value, targetType, "abi.fpcast");
    }

    if (sourceType->isIntegerTy() && targetType->isFloatingPointTy()) {
        return cg.builder.CreateSIToFP(value, targetType, "abi.sitofp");
    }

    if (sourceType->isFloatingPointTy() && targetType->isIntegerTy()) {
        return cg.builder.CreateFPToSI(value, targetType, "abi.fptosi");
    }

    if (sourceType->isPointerTy() && targetType->isPointerTy()) {
        return cg.builder.CreateBitCast(value, targetType, "abi.bitcast");
    }

    return value;
}
