#include "codegen.h"
#include "dict_support.h"
#include "BinaryExpressionNode.h"
#include "builtin_structs.h"
#include "ASTVisitor.h"
#include "TensorRuntime.h"

#include <iostream>
#include <vector>

namespace {
bool isArithmeticOperator(const std::string& op) {
    return op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
           op == "**" || op == "div" || op == "mod";
}

bool isTensorOperator(const std::string& op) {
    return op == "@" || op == "inner" || op == "outer" || op == "tensor";
}

bool isTensorArithmeticOperator(const std::string& op) {
    return op == "+" || op == "-" || op == "*" || op == "/";
}

bool isSetOperator(const std::string& op) {
    return op == "union" || op == "intersect" || op == "diff" ||
           op == "without" || op == "symdiff";
}

bool isSetPredicateOperator(const std::string& op) {
    return op == "in" || op == "notin" || op == "subset" ||
           op == "superset" || op == "properSubset" || op == "properSuperset";
}

bool isLogicalOperator(const std::string& op) {
    return op == "implies" || op == "iff";
}

bool isNumericTypeName(const std::string& name) {
    return name == "Int" || name == "Short" || name == "Byte" || name == "Long" ||
           name == "Natural" || name == "Integer" || name == "Float" ||
           name == "Double" || name == "Real";
}

llvm::Value* coerceNumericValue(llvm::Value* value, llvm::Type* targetType) {
    auto& cg = CodeGenerator::getInstance();
    if (!value || !targetType || value->getType() == targetType) return value;
    if (value->getType()->isIntegerTy() && targetType->isFloatingPointTy()) {
        return cg.builder.CreateSIToFP(value, targetType, "num.sitofp");
    }
    if (value->getType()->isFloatingPointTy() && targetType->isFloatingPointTy()) {
        return cg.builder.CreateFPCast(value, targetType, "num.fpcast");
    }
    if (value->getType()->isIntegerTy() && targetType->isIntegerTy()) {
        unsigned srcBits = value->getType()->getIntegerBitWidth();
        unsigned dstBits = targetType->getIntegerBitWidth();
        if (srcBits < dstBits) return cg.builder.CreateSExt(value, targetType, "num.sext");
        if (srcBits > dstBits) return cg.builder.CreateTrunc(value, targetType, "num.trunc");
    }
    return value;
}

llvm::Value* coerceToBool(llvm::Value* value) {
    auto& cg = CodeGenerator::getInstance();
    if (!value) return nullptr;
    if (value->getType()->isIntegerTy(1)) return value;
    if (value->getType()->isIntegerTy()) {
        return cg.builder.CreateICmpNE(value, llvm::ConstantInt::get(value->getType(), 0), "tobool");
    }
    if (value->getType()->isFloatingPointTy()) {
        return cg.builder.CreateFCmpONE(value, llvm::ConstantFP::get(value->getType(), 0.0), "tobool");
    }
    return nullptr;
}

llvm::Value* coerceToI64(llvm::Value* value) {
    auto& cg = CodeGenerator::getInstance();
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    if (!value) return nullptr;
    if (value->getType()->isIntegerTy()) {
        return coerceNumericValue(value, i64Ty);
    }
    if (value->getType()->isFloatingPointTy()) {
        return cg.builder.CreateFPToSI(value, i64Ty, "num.fptosi");
    }
    return nullptr;
}

llvm::Function* getOrCreateRuntimeDoubleBinary(const std::string& name) {
    auto& cg = CodeGenerator::getInstance();
    if (auto* function = cg.module->getFunction(name)) return function;
    auto* doubleTy = llvm::Type::getDoubleTy(cg.context);
    auto* functionTy = llvm::FunctionType::get(doubleTy, { doubleTy, doubleTy }, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, name, cg.module.get());
}

llvm::Function* getOrCreateRuntimeFunction(const std::string& name, llvm::Type* returnType, const std::vector<llvm::Type*>& paramTypes) {
    auto& cg = CodeGenerator::getInstance();
    if (auto* function = cg.module->getFunction(name)) return function;
    auto* functionTy = llvm::FunctionType::get(returnType, paramTypes, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, name, cg.module.get());
}

llvm::AllocaInst* createEntryAlloca(llvm::Function* function, llvm::Type* type, const llvm::Twine& name) {
    llvm::IRBuilder<> entryBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return entryBuilder.CreateAlloca(type, nullptr, name);
}

llvm::Value* coerceValueForString(CodeGenerator& cg, llvm::Value* value, const Type* type) {
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());

    if (!value || !type) {
        return llvm::ConstantPointerNull::get(i8PtrTy);
    }
    if (type->isStringTy()) {
        if (!value->getType()->isPointerTy()) {
            return llvm::ConstantPointerNull::get(i8PtrTy);
        }

        // Local String variables are represented by an alloca holding an i8*.
        // Concatenation requires the stored pointer, not the address of that slot.
        if (auto* slot = llvm::dyn_cast<llvm::AllocaInst>(value)) {
            return cg.builder.CreateLoad(i8PtrTy, slot, "str.load");
        }
        return value;
    }
    if (type->isCharTy()) {
        llvm::Value* charValue = value->getType()->isIntegerTy(8)
            ? value
            : cg.builder.CreateTrunc(value, i8Ty, "str.char");
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction("csec_to_string_char", i8PtrTy, {i8Ty}),
            {charValue},
            "str.char.call");
    }
    if (type->getName() == "Boolean" || type->getName() == "Bool") {
        llvm::Value* boolValue = value->getType()->isIntegerTy(32)
            ? value
            : cg.builder.CreateZExt(value, i32Ty, "str.bool");
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction("csec_to_string_bool", i8PtrTy, {i32Ty}),
            {boolValue},
            "str.bool.call");
    }
    if (type->isFloatTy() || type->isDoubleTy()) {
        llvm::Value* doubleValue = value->getType()->isDoubleTy()
            ? value
            : (value->getType()->isFloatTy()
                ? cg.builder.CreateFPExt(value, f64Ty, "str.fpext")
                : cg.builder.CreateSIToFP(value, f64Ty, "str.sitofp"));
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction("csec_to_string_double", i8PtrTy, {f64Ty}),
            {doubleValue},
            "str.double.call");
    }
    if (type->isIntegerTy()) {
        llvm::Value* intValue = value;
        if (!value->getType()->isIntegerTy(64)) {
            unsigned bits = value->getType()->getIntegerBitWidth();
            intValue = bits < 64
                ? cg.builder.CreateSExt(value, i64Ty, "str.sext")
                : cg.builder.CreateTrunc(value, i64Ty, "str.trunc");
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction("csec_to_string_i64", i8PtrTy, {i64Ty}),
            {intValue},
            "str.int.call");
    }
    // A dictionary renders as Python's dict repr: {'a': 1, 'b': [2, 3]}.
    if ((isDictType(type) || isSetType(type)) && value->getType()->isPointerTy()) {
        if (auto* slot = llvm::dyn_cast<llvm::AllocaInst>(value)) {
            if (slot->getAllocatedType()->isPointerTy()) value = cg.builder.CreateLoad(i8PtrTy, slot, "dict.load");
        }
        return codegenDictToString(value);
    }
    // A Nat is a heap handle; render it as its exact decimal expansion.
    if (type->getName() == "Nat" && value->getType()->isPointerTy()) {
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction("csec_bigint_to_string", i8PtrTy, {i8PtrTy}),
            {value},
            "str.nat.call");
    }

    return llvm::ConstantPointerNull::get(i8PtrTy);
}

llvm::Value* codegenStringConcat(CodeGenerator& cg, llvm::Value* leftValue, const Type* leftType, llvm::Value* rightValue, const Type* rightType) {
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());
    llvm::Value* leftString = coerceValueForString(cg, leftValue, leftType);
    llvm::Value* rightString = coerceValueForString(cg, rightValue, rightType);
    return cg.builder.CreateCall(
        getOrCreateRuntimeFunction("csec_string_concat", i8PtrTy, {i8PtrTy, i8PtrTy}),
        {leftString, rightString},
        "str.concat");
}

void collectStringConcatOperands(ASTNode* node, std::vector<ASTNode*>& operands) {
    auto* binary = dynamic_cast<BinaryExpressionNode*>(node);
    auto nodeType = node ? node->getType() : nullptr;
    if (binary && binary->op == "+" && nodeType && nodeType->isStringTy()) {
        collectStringConcatOperands(binary->left.get(), operands);
        collectStringConcatOperands(binary->right.get(), operands);
        return;
    }
    operands.push_back(node);
}

llvm::Value* codegenStringConcatChain(CodeGenerator& cg, ASTNode* root) {
    std::vector<ASTNode*> operands;
    collectStringConcatOperands(root, operands);
    if (operands.size() < 3) return nullptr;

    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    std::vector<llvm::Value*> strings;
    strings.reserve(operands.size());
    for (ASTNode* operand : operands) {
        llvm::Value* value = operand ? operand->codegen() : nullptr;
        auto type = operand ? operand->getType() : nullptr;
        strings.push_back(coerceValueForString(cg, value, type.get()));
    }
    auto* arrayTy = llvm::ArrayType::get(i8PtrTy, strings.size());
    auto* storage = cg.builder.CreateAlloca(arrayTy, nullptr, "str.parts");
    for (size_t i = 0; i < strings.size(); ++i) {
        auto* slot = cg.builder.CreateInBoundsGEP(arrayTy, storage,
            {llvm::ConstantInt::get(i64Ty, 0), llvm::ConstantInt::get(i64Ty, i)}, "str.part");
        cg.builder.CreateStore(strings[i], slot);
    }
    llvm::Value* first = cg.builder.CreateInBoundsGEP(arrayTy, storage,
        {llvm::ConstantInt::get(i64Ty, 0), llvm::ConstantInt::get(i64Ty, 0)}, "str.parts.first");
    return cg.builder.CreateCall(
        getOrCreateRuntimeFunction("csec_string_concat_many", i8PtrTy, {llvm::PointerType::getUnqual(i8PtrTy->getContext()), i64Ty}),
        {first, llvm::ConstantInt::get(i64Ty, strings.size())}, "str.concat.chain");
}

llvm::Value* codegenClassOperator(const std::string& op, llvm::Value* leftValue, ASTNode* leftNode, ASTNode* rightNode) {
    auto& cg = CodeGenerator::getInstance();
    auto leftType = leftNode ? leftNode->getType() : nullptr;
    if (!leftType || leftType->getKind() != Type::Kind::CLASS) {
        return nullptr;
    }

    auto* classSymbol = cg.symbolTable.lookupClass(leftType->getName());
    if (!classSymbol) {
        return nullptr;
    }

    llvm::Value* rightValue = rightNode ? rightNode->codegen() : nullptr;
    if (!rightValue) {
        return nullptr;
    }

    auto rightType = rightNode->getType();
    std::vector<std::unique_ptr<Type>> argTypes;
    argTypes.push_back(rightType ? rightType->clone() : std::make_unique<UnknownType>());

    std::string methodName = "operator" + op;
    auto* methodSymbol = cg.symbolTable.lookupMethod(*classSymbol, methodName, argTypes);
    if (!methodSymbol) {
        return nullptr;
    }

    auto* funcType = dynamic_cast<FunctionType*>(methodSymbol->type.get());
    auto* function = llvm::dyn_cast_or_null<llvm::Function>(methodSymbol->value);
    if (!funcType || !function || funcType->parameterTypes.size() != 2) {
        return nullptr;
    }

    if (!rightType) {
        return nullptr;
    }
    if (!rightType->equals(funcType->parameterTypes[1]) && !rightType->isSubtypeOf(funcType->parameterTypes[1]) &&
        !isNumericWidening(*rightType, *funcType->parameterTypes[1])) {
        return nullptr;
    }
    // The overload may have been chosen through numeric widening (an Int literal for a Long or Double
    // parameter); Type::equals treats numeric types loosely, so convert by the LLVM types.
    llvm::Type* paramType = function->getFunctionType()->getParamType(1);
    if (rightValue->getType() != paramType && (paramType->isIntegerTy() || paramType->isFloatingPointTy())) {
        rightValue = coerceNumericValue(rightValue, paramType);
    }
    // Value structs: `this` is passed by address, other struct parameters by value. A struct operand
    // may arrive as a value (a call result) or as the address of a variable.
    auto adapt = [&](llvm::Value* value, llvm::Type* wanted, const char* name) -> llvm::Value* {
        if (!value || value->getType() == wanted) return value;
        if (wanted->isPointerTy() && value->getType()->isStructTy()) {
            llvm::Value* slot = cg.builder.CreateAlloca(value->getType(), nullptr, name);
            cg.builder.CreateStore(value, slot);
            return slot;
        }
        if (wanted->isStructTy() && value->getType()->isPointerTy()) {
            return cg.builder.CreateLoad(wanted, value, name);
        }
        return value;
    };
    leftValue = adapt(leftValue, function->getFunctionType()->getParamType(0), "op.self");
    rightValue = adapt(rightValue, paramType, "op.arg");

    return cg.builder.CreateCall(function, {leftValue, rightValue}, function->getReturnType()->isVoidTy() ? "" : "op.call");
}
}


void BinaryExpressionNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

/*bool isStringTypeFromLLVM(llvm::Value* value, CodeGenerator* codeGenerator) {
    return value->getType()->isPointerTy() && (static_cast<llvm::PointerType*>(value->getType()))->isValidElementType(llvm::Type::getInt8Ty(CodeGenerator::getInstance().context));
}*/

// Helper: generate Complex arithmetic
static llvm::Value* codegenComplexOp(const std::string& op, llvm::Value* leftValue, llvm::Value* rightValue) {
    auto& builder = CodeGenerator::getInstance().builder;
    auto& context = CodeGenerator::getInstance().context;
    auto* complexTy = leftValue->getType();

    llvm::Value* aRe = builder.CreateExtractValue(leftValue, {0}, "a_re");
    llvm::Value* aIm = builder.CreateExtractValue(leftValue, {1}, "a_im");
    llvm::Value* bRe = builder.CreateExtractValue(rightValue, {0}, "b_re");
    llvm::Value* bIm = builder.CreateExtractValue(rightValue, {1}, "b_im");

    llvm::Value* result = llvm::UndefValue::get(complexTy);

    if (op == "+") {
        result = builder.CreateInsertValue(result, builder.CreateFAdd(aRe, bRe, "re_add"), {0});
        result = builder.CreateInsertValue(result, builder.CreateFAdd(aIm, bIm, "im_add"), {1});
    } else if (op == "-") {
        result = builder.CreateInsertValue(result, builder.CreateFSub(aRe, bRe, "re_sub"), {0});
        result = builder.CreateInsertValue(result, builder.CreateFSub(aIm, bIm, "im_sub"), {1});
    } else if (op == "*") {
        // (ac-bd, ad+bc)
        llvm::Value* ac = builder.CreateFMul(aRe, bRe, "ac");
        llvm::Value* bd = builder.CreateFMul(aIm, bIm, "bd");
        llvm::Value* ad = builder.CreateFMul(aRe, bIm, "ad");
        llvm::Value* bc = builder.CreateFMul(aIm, bRe, "bc");
        result = builder.CreateInsertValue(result, builder.CreateFSub(ac, bd, "re_mul"), {0});
        result = builder.CreateInsertValue(result, builder.CreateFAdd(ad, bc, "im_mul"), {1});
    } else if (op == "/") {
        // ((ac+bd)/(c^2+d^2), (bc-ad)/(c^2+d^2))
        llvm::Value* cc = builder.CreateFMul(bRe, bRe, "cc");
        llvm::Value* dd = builder.CreateFMul(bIm, bIm, "dd");
        llvm::Value* denom = builder.CreateFAdd(cc, dd, "denom");
        llvm::Value* ac = builder.CreateFMul(aRe, bRe, "ac");
        llvm::Value* bd = builder.CreateFMul(aIm, bIm, "bd");
        llvm::Value* bc = builder.CreateFMul(aIm, bRe, "bc");
        llvm::Value* ad = builder.CreateFMul(aRe, bIm, "ad");
        result = builder.CreateInsertValue(result, builder.CreateFDiv(builder.CreateFAdd(ac, bd, "acbd"), denom, "re_div"), {0});
        result = builder.CreateInsertValue(result, builder.CreateFDiv(builder.CreateFSub(bc, ad, "bcad"), denom, "im_div"), {1});
    }
    return result;
}

// Get/declare a csec_bigint_* runtime function taking N pointer handles and returning a pointer handle.
// (The runtime helpers keep the bigint name; the language type they back is Nat.)
static llvm::Function* getNatFn(CodeGenerator& cg, const std::string& name, unsigned arity) {
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    std::vector<llvm::Type*> params(arity, i8PtrTy);
    return getOrCreateRuntimeFunction(name, i8PtrTy, params);
}

// Arbitrary-precision integer op on two Nat handles -> a new handle (csec_bigint_add/sub/mul).
static llvm::Value* natBinary(CodeGenerator& cg, const char* fn, llvm::Value* a, llvm::Value* b) {
    return cg.builder.CreateCall(getNatFn(cg, fn, 2), {a, b}, "nat");
}

// Coerce a Nat operand for arithmetic/comparison. A Nat is already a ptr handle; an integer literal or
// value (e.g. the 1 in `n + 1`, or 1000 in `n < 1000`) is promoted to a handle via the runtime.
static llvm::Value* asNatHandle(CodeGenerator& cg, llvm::Value* v, const Type* t) {
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    if (!v) return nullptr;
    // An actual Nat: the value is already a handle.
    if (t && t->getName() == "Nat" && v->getType()->isPointerTy()) return v;
    // An integer (literal or variable) is promoted to a handle. Only integers are promoted -- a String
    // or other pointer operand is NOT a Nat, so we return null and let `+`/`==` fall through to the
    // string/other path (e.g. `natValue + " apples"` must be concatenation, not bignum addition).
    if (v->getType()->isIntegerTy() && (!t || t->getKind() != Type::Kind::CLASS)) {
        auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
        llvm::Value* wide = v->getType()->isIntegerTy(64) ? v : cg.builder.CreateSExt(v, i64Ty, "nat.sext");
        auto* fn = getOrCreateRuntimeFunction("csec_bigint_from_i64", i8PtrTy, {i64Ty});
        return cg.builder.CreateCall(fn, {wide}, "nat.from.i64");
    }
    return nullptr;
}

// Scalar Nat op: +/-/* -> a new handle; comparisons -> i1 via csec_bigint_cmp. Returns nullptr if this
// is not a Nat operation (neither operand is Nat), so the caller falls through.
static llvm::Value* codegenNatScalarOp(const std::string& op, llvm::Value* leftValue, const Type* leftType,
                                       llvm::Value* rightValue, const Type* rightType) {
    const bool leftNat = leftType && leftType->getName() == "Nat";
    const bool rightNat = rightType && rightType->getName() == "Nat";
    if (!leftNat && !rightNat) return nullptr;
    auto& cg = CodeGenerator::getInstance();
    llvm::Value* a = asNatHandle(cg, leftValue, leftType);
    llvm::Value* b = asNatHandle(cg, rightValue, rightType);
    if (!a || !b) return nullptr;

    if (op == "+") return natBinary(cg, "csec_bigint_add", a, b);
    if (op == "-") return natBinary(cg, "csec_bigint_sub", a, b);
    if (op == "*") return natBinary(cg, "csec_bigint_mul", a, b);

    if (op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=") {
        auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
        auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
        llvm::Value* cmp = cg.builder.CreateCall(
            getOrCreateRuntimeFunction("csec_bigint_cmp", i32Ty, {i8PtrTy, i8PtrTy}), {a, b}, "big.cmp");
        llvm::Value* zero = llvm::ConstantInt::get(i32Ty, 0);
        if (op == "==") return cg.builder.CreateICmpEQ(cmp, zero, "big.eq");
        if (op == "!=") return cg.builder.CreateICmpNE(cmp, zero, "big.ne");
        if (op == "<")  return cg.builder.CreateICmpSLT(cmp, zero, "big.lt");
        if (op == "<=") return cg.builder.CreateICmpSLE(cmp, zero, "big.le");
        if (op == ">")  return cg.builder.CreateICmpSGT(cmp, zero, "big.gt");
        return cg.builder.CreateICmpSGE(cmp, zero, "big.ge");
    }
    return nullptr;
}

// Get or create __gcd_i64(i64, i64) -> i64 in the module
static llvm::Function* getOrCreateGcdI64(llvm::Module& module, llvm::LLVMContext& context) {
    if (auto* f = module.getFunction("__gcd_i64")) return f;

    auto* i64Ty = llvm::Type::getInt64Ty(context);
    auto* funcTy = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty}, false);
    auto* func = llvm::Function::Create(funcTy, llvm::Function::InternalLinkage, "__gcd_i64", module);

    auto* entryBB = llvm::BasicBlock::Create(context, "entry", func);
    auto* loopBB = llvm::BasicBlock::Create(context, "loop", func);
    auto* bodyBB = llvm::BasicBlock::Create(context, "body", func);
    auto* exitBB = llvm::BasicBlock::Create(context, "exit", func);

    llvm::IRBuilder<> b(context);
    auto args = func->arg_begin();
    llvm::Value* aArg = &*args++;
    llvm::Value* bArg = &*args++;

    // entry: abs(a), abs(b)
    b.SetInsertPoint(entryBB);
    llvm::Value* aNeg = b.CreateICmpSLT(aArg, llvm::ConstantInt::get(i64Ty, 0));
    llvm::Value* aAbs = b.CreateSelect(aNeg, b.CreateNeg(aArg), aArg, "a_abs");
    llvm::Value* bNeg = b.CreateICmpSLT(bArg, llvm::ConstantInt::get(i64Ty, 0));
    llvm::Value* bAbs = b.CreateSelect(bNeg, b.CreateNeg(bArg), bArg, "b_abs");
    b.CreateBr(loopBB);

    // loop: Euclidean algorithm
    b.SetInsertPoint(loopBB);
    auto* xPhi = b.CreatePHI(i64Ty, 2, "x");
    auto* yPhi = b.CreatePHI(i64Ty, 2, "y");
    llvm::Value* isZero = b.CreateICmpEQ(yPhi, llvm::ConstantInt::get(i64Ty, 0));
    b.CreateCondBr(isZero, exitBB, bodyBB);

    // body: rem = x % y; x = y; y = rem
    b.SetInsertPoint(bodyBB);
    llvm::Value* rem = b.CreateSRem(xPhi, yPhi, "rem");
    b.CreateBr(loopBB);

    xPhi->addIncoming(aAbs, entryBB);
    xPhi->addIncoming(yPhi, bodyBB);
    yPhi->addIncoming(bAbs, entryBB);
    yPhi->addIncoming(rem, bodyBB);

    // exit: return x
    b.SetInsertPoint(exitBB);
    b.CreateRet(xPhi);

    return func;
}

// Helper: generate Rational arithmetic
static llvm::Value* codegenRationalOp(const std::string& op, llvm::Value* leftValue, llvm::Value* rightValue) {
    auto& cg = CodeGenerator::getInstance();
    auto& builder = cg.builder;
    auto* rationalTy = leftValue->getType();

    llvm::Value* aN = builder.CreateExtractValue(leftValue, {0}, "a_num");
    llvm::Value* aD = builder.CreateExtractValue(leftValue, {1}, "a_den");
    llvm::Value* bN = builder.CreateExtractValue(rightValue, {0}, "b_num");
    llvm::Value* bD = builder.CreateExtractValue(rightValue, {1}, "b_den");

    llvm::Value* num = nullptr;
    llvm::Value* den = nullptr;

    if (op == "+") {
        // (ad+bc, bd)
        llvm::Value* ad = builder.CreateMul(aN, bD, "ad");
        llvm::Value* bc = builder.CreateMul(bN, aD, "bc");
        num = builder.CreateAdd(ad, bc, "num_add");
        den = builder.CreateMul(aD, bD, "den_add");
    } else if (op == "-") {
        // (ad-bc, bd)
        llvm::Value* ad = builder.CreateMul(aN, bD, "ad");
        llvm::Value* bc = builder.CreateMul(bN, aD, "bc");
        num = builder.CreateSub(ad, bc, "num_sub");
        den = builder.CreateMul(aD, bD, "den_sub");
    } else if (op == "*") {
        // (ac, bd)
        num = builder.CreateMul(aN, bN, "num_mul");
        den = builder.CreateMul(aD, bD, "den_mul");
    } else if (op == "/") {
        // (ad, bc)
        num = builder.CreateMul(aN, bD, "num_div");
        den = builder.CreateMul(aD, bN, "den_div");
    }

    if (!num || !den) {
        std::cerr << "Type error: unsupported Rational operator '" << op << "'" << std::endl;
        return nullptr;
    }

    // GCD normalization
    llvm::Function* gcdFunc = getOrCreateGcdI64(*cg.module, cg.context);
    llvm::Value* g = builder.CreateCall(gcdFunc, {num, den}, "gcd");
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    llvm::Value* one = llvm::ConstantInt::get(i64Ty, 1);
    llvm::Value* gIsZero = builder.CreateICmpEQ(g, llvm::ConstantInt::get(i64Ty, 0));
    g = builder.CreateSelect(gIsZero, one, g, "gcd_safe");
    num = builder.CreateSDiv(num, g, "num_reduced");
    den = builder.CreateSDiv(den, g, "den_reduced");

    // Sign normalization: if den < 0, negate both
    llvm::Value* denNeg = builder.CreateICmpSLT(den, llvm::ConstantInt::get(i64Ty, 0));
    num = builder.CreateSelect(denNeg, builder.CreateNeg(num), num, "num_sign");
    den = builder.CreateSelect(denNeg, builder.CreateNeg(den), den, "den_sign");

    llvm::Value* result = llvm::UndefValue::get(rationalTy);
    result = builder.CreateInsertValue(result, num, {0});
    result = builder.CreateInsertValue(result, den, {1});
    return result;
}

// Helper: generate Quaternion arithmetic
static llvm::Value* codegenQuaternionOp(const std::string& op, llvm::Value* leftValue, llvm::Value* rightValue) {
    auto& builder = CodeGenerator::getInstance().builder;
    auto* quatTy = leftValue->getType();

    llvm::Value* aw = builder.CreateExtractValue(leftValue, {0}, "a_w");
    llvm::Value* ax = builder.CreateExtractValue(leftValue, {1}, "a_x");
    llvm::Value* ay = builder.CreateExtractValue(leftValue, {2}, "a_y");
    llvm::Value* az = builder.CreateExtractValue(leftValue, {3}, "a_z");
    llvm::Value* bw = builder.CreateExtractValue(rightValue, {0}, "b_w");
    llvm::Value* bx = builder.CreateExtractValue(rightValue, {1}, "b_x");
    llvm::Value* by = builder.CreateExtractValue(rightValue, {2}, "b_y");
    llvm::Value* bz = builder.CreateExtractValue(rightValue, {3}, "b_z");

    llvm::Value* result = llvm::UndefValue::get(quatTy);

    if (op == "+") {
        result = builder.CreateInsertValue(result, builder.CreateFAdd(aw, bw, "w_res"), {0});
        result = builder.CreateInsertValue(result, builder.CreateFAdd(ax, bx, "x_res"), {1});
        result = builder.CreateInsertValue(result, builder.CreateFAdd(ay, by, "y_res"), {2});
        result = builder.CreateInsertValue(result, builder.CreateFAdd(az, bz, "z_res"), {3});
    } else if (op == "-") {
        result = builder.CreateInsertValue(result, builder.CreateFSub(aw, bw, "w_res"), {0});
        result = builder.CreateInsertValue(result, builder.CreateFSub(ax, bx, "x_res"), {1});
        result = builder.CreateInsertValue(result, builder.CreateFSub(ay, by, "y_res"), {2});
        result = builder.CreateInsertValue(result, builder.CreateFSub(az, bz, "z_res"), {3});
    } else if (op == "*") {
        // Hamilton product
        // w = aw*bw - ax*bx - ay*by - az*bz
        llvm::Value* rw = builder.CreateFSub(
            builder.CreateFSub(
                builder.CreateFSub(builder.CreateFMul(aw, bw), builder.CreateFMul(ax, bx)),
                builder.CreateFMul(ay, by)),
            builder.CreateFMul(az, bz), "q_w");
        // x = aw*bx + ax*bw + ay*bz - az*by
        llvm::Value* rx = builder.CreateFSub(
            builder.CreateFAdd(
                builder.CreateFAdd(builder.CreateFMul(aw, bx), builder.CreateFMul(ax, bw)),
                builder.CreateFMul(ay, bz)),
            builder.CreateFMul(az, by), "q_x");
        // y = aw*by - ax*bz + ay*bw + az*bx
        llvm::Value* ry = builder.CreateFAdd(
            builder.CreateFAdd(
                builder.CreateFSub(builder.CreateFMul(aw, by), builder.CreateFMul(ax, bz)),
                builder.CreateFMul(ay, bw)),
            builder.CreateFMul(az, bx), "q_y");
        // z = aw*bz + ax*by - ay*bx + az*bw
        llvm::Value* rz = builder.CreateFAdd(
            builder.CreateFSub(
                builder.CreateFAdd(builder.CreateFMul(aw, bz), builder.CreateFMul(ax, by)),
                builder.CreateFMul(ay, bx)),
            builder.CreateFMul(az, bw), "q_z");
        result = builder.CreateInsertValue(result, rw, {0});
        result = builder.CreateInsertValue(result, rx, {1});
        result = builder.CreateInsertValue(result, ry, {2});
        result = builder.CreateInsertValue(result, rz, {3});
    }
    return result;
}

llvm::Value* BinaryExpressionNode::codegen() {
    {
        auto leftSetType = left->getType();
        auto rightSetType = right->getType();
        if (isSetType(leftSetType.get()) && isSetType(rightSetType.get()) && isSetBinaryOperator(op)) {
            return codegenSetBinary(op, left.get(), right.get());
        }
    }
    if (op == "in" || op == "notin") {
        auto containerType = right->getType();
        if (isSetType(containerType.get())) {
            llvm::Value* found = codegenSetContains(right.get(), left.get());
            if (!found || op == "in") return found;
            return CodeGenerator::getInstance().builder.CreateNot(found, "set.notin");
        }
        if (isDictType(containerType.get())) {
            llvm::Value* found = codegenDictContains(right.get(), left.get());
            if (!found || op == "in") return found;
            return CodeGenerator::getInstance().builder.CreateNot(found, "dict.notin");
        }
    }
    if (op == "+") {
        auto resultType = getType();
        if (resultType && resultType->isStringTy()) {
            if (auto* concatenated = codegenStringConcatChain(CodeGenerator::getInstance(), this)) {
                return concatenated;
            }
        }
    }
    llvm::Value* leftValue = left->codegen();
    if (!leftValue) return nullptr;

    if (auto* overloaded = codegenClassOperator(op, leftValue, left.get(), right.get())) {
        return overloaded;
    }

    if (leftValue->getType()->isPointerTy()) {
        auto& cg = CodeGenerator::getInstance();
        auto leftType = left->getType();
        if (!leftType) {
            std::cerr << "Type error: Cannot determine type of left operand" << std::endl;
            return nullptr;
        }
        const bool loadStringSlot = leftType->isStringTy() && llvm::isa<llvm::AllocaInst>(leftValue);
        if (!TensorRuntime::isTensorTypeName(leftType->getName()) &&
            (!leftType->isStringTy() || loadStringSlot) &&
            (leftType->getKind() != Type::Kind::CLASS || builtinValueStructFields(leftType->getName()))) {
            leftValue = cg.builder.CreateLoad(cg.getLLVMType(leftType.get()), leftValue, "loadtmp");
        }
    }

    // Short-circuit logical AND
    if (op == "and") {
        auto& cg = CodeGenerator::getInstance();
        llvm::Function* func = cg.builder.GetInsertBlock()->getParent();

        llvm::Value* leftBool = leftValue;
        if (!leftBool->getType()->isIntegerTy(1)) {
            if (leftBool->getType()->isIntegerTy()) {
                leftBool = cg.builder.CreateICmpNE(leftBool, llvm::ConstantInt::get(leftBool->getType(), 0), "tobool");
            }
            else if (leftBool->getType()->isFloatingPointTy()) {
                leftBool = cg.builder.CreateFCmpONE(leftBool, llvm::ConstantFP::get(leftBool->getType(), 0.0), "tobool");
            }
            else {
                std::cerr << "Type error: Operator 'and' requires numeric or boolean operands" << std::endl;
                return nullptr;
            }
        }

        llvm::Value* resultPtr = createEntryAlloca(func, llvm::Type::getInt1Ty(cg.context), "and_result");
        cg.builder.CreateStore(llvm::ConstantInt::getFalse(cg.context), resultPtr);

        llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(cg.context, "and_rhs", func);
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(cg.context, "and_end", func);

        cg.builder.CreateCondBr(leftBool, rhsBB, mergeBB);

        cg.builder.SetInsertPoint(rhsBB);
        llvm::Value* rv = right->codegen();
        if (!rv) return nullptr;
        if (rv->getType()->isPointerTy()) {
            rv = cg.builder.CreateLoad(cg.getLLVMType(right->getType().get()), rv, "loadtmp");
        }
        llvm::Value* rightBool = rv;
        if (!rightBool->getType()->isIntegerTy(1)) {
            if (rightBool->getType()->isIntegerTy()) {
                rightBool = cg.builder.CreateICmpNE(rightBool, llvm::ConstantInt::get(rightBool->getType(), 0), "tobool");
            }
            else if (rightBool->getType()->isFloatingPointTy()) {
                rightBool = cg.builder.CreateFCmpONE(rightBool, llvm::ConstantFP::get(rightBool->getType(), 0.0), "tobool");
            }
            else {
                std::cerr << "Type error: Operator 'and' requires numeric or boolean operands" << std::endl;
                return nullptr;
            }
        }
        cg.builder.CreateStore(rightBool, resultPtr);
        if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
            cg.builder.CreateBr(mergeBB);
        }

        cg.builder.SetInsertPoint(mergeBB);
        return cg.builder.CreateLoad(llvm::Type::getInt1Ty(cg.context), resultPtr, "and_val");
    }

    // Short-circuit logical OR
    if (op == "or") {
        auto& cg = CodeGenerator::getInstance();
        llvm::Function* func = cg.builder.GetInsertBlock()->getParent();

        llvm::Value* leftBool = leftValue;
        if (!leftBool->getType()->isIntegerTy(1)) {
            if (leftBool->getType()->isIntegerTy()) {
                leftBool = cg.builder.CreateICmpNE(leftBool, llvm::ConstantInt::get(leftBool->getType(), 0), "tobool");
            }
            else if (leftBool->getType()->isFloatingPointTy()) {
                leftBool = cg.builder.CreateFCmpONE(leftBool, llvm::ConstantFP::get(leftBool->getType(), 0.0), "tobool");
            }
            else {
                std::cerr << "Type error: Operator 'or' requires numeric or boolean operands" << std::endl;
                return nullptr;
            }
        }

        llvm::Value* resultPtr = createEntryAlloca(func, llvm::Type::getInt1Ty(cg.context), "or_result");
        cg.builder.CreateStore(llvm::ConstantInt::getTrue(cg.context), resultPtr);

        llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(cg.context, "or_rhs", func);
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(cg.context, "or_end", func);

        cg.builder.CreateCondBr(leftBool, mergeBB, rhsBB);

        cg.builder.SetInsertPoint(rhsBB);
        llvm::Value* rv = right->codegen();
        if (!rv) return nullptr;
        if (rv->getType()->isPointerTy()) {
            rv = cg.builder.CreateLoad(cg.getLLVMType(right->getType().get()), rv, "loadtmp");
        }
        llvm::Value* rightBool = rv;
        if (!rightBool->getType()->isIntegerTy(1)) {
            if (rightBool->getType()->isIntegerTy()) {
                rightBool = cg.builder.CreateICmpNE(rightBool, llvm::ConstantInt::get(rightBool->getType(), 0), "tobool");
            }
            else if (rightBool->getType()->isFloatingPointTy()) {
                rightBool = cg.builder.CreateFCmpONE(rightBool, llvm::ConstantFP::get(rightBool->getType(), 0.0), "tobool");
            }
            else {
                std::cerr << "Type error: Operator 'or' requires numeric or boolean operands" << std::endl;
                return nullptr;
            }
        }
        cg.builder.CreateStore(rightBool, resultPtr);
        if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
            cg.builder.CreateBr(mergeBB);
        }

        cg.builder.SetInsertPoint(mergeBB);
        return cg.builder.CreateLoad(llvm::Type::getInt1Ty(cg.context), resultPtr, "or_val");
    }

    llvm::Value* rightValue = right->codegen();
    if (!rightValue) return nullptr;

    if (rightValue->getType()->isPointerTy()) {
        auto& cg = CodeGenerator::getInstance();
        auto rightType = right->getType();
        if (!rightType) {
            std::cerr << "Type error: Cannot determine type of right operand" << std::endl;
            return nullptr;
        }
        const bool loadStringSlot = rightType->isStringTy() && llvm::isa<llvm::AllocaInst>(rightValue);
        if (!TensorRuntime::isTensorTypeName(rightType->getName()) &&
            (!rightType->isStringTy() || loadStringSlot) &&
            (rightType->getKind() != Type::Kind::CLASS || builtinValueStructFields(rightType->getName()))) {
            rightValue = cg.builder.CreateLoad(cg.getLLVMType(rightType.get()), rightValue, "loadtmp");
        }
    }

    // Struct type arithmetic (Complex, Rational, Quaternion)
    std::string leftTypeName = left->getType() ? left->getType()->getName() : "";
    std::string rightTypeName = right->getType() ? right->getType()->getName() : "";
    if (leftTypeName == rightTypeName) {
        if (leftTypeName == "Complex" && (op == "+" || op == "-" || op == "*" || op == "/")) {
            return codegenComplexOp(op, leftValue, rightValue);
        }
        if (leftTypeName == "Rational" && (op == "+" || op == "-" || op == "*" || op == "/")) {
            return codegenRationalOp(op, leftValue, rightValue);
        }
        if (leftTypeName == "Quaternion" && (op == "+" || op == "-" || op == "*")) {
            return codegenQuaternionOp(op, leftValue, rightValue);
        }
        if (leftTypeName == "Quaternion" && op == "/") {
            std::cerr << "Type error: Quaternion division is not supported (division is undefined for quaternions)" << std::endl;
            return nullptr;
        }
    }

    // Arbitrary-precision arithmetic/comparison when either operand is a Nat (an unbounded natural
    // number). Handles mixed Nat/integer operands (`n + 1`, `n < 1000`) by promoting the integer side
    // to a handle. Placed before string concat so `+` on a Nat is not misread as concatenation.
    {
        auto lt = left->getType();
        auto rt = right->getType();
        if (auto* nat = codegenNatScalarOp(op, leftValue, lt.get(), rightValue, rt.get())) {
            return nat;
        }
    }

    auto& cg = CodeGenerator::getInstance();

    // Two non-class pointer operands to `+` (or `==`/`!=`) are treated as strings — EXCEPT tensors,
    // which are also heap structs behind a pointer. Without the tensor exclusion, `tensorA + tensorB`
    // (e.g. a matmul result plus a bias) is mis-lowered to string concatenation, and the bogus
    // "string" pointer segfaults the next tensor op that reads it. Tensor `+` must fall through to
    // the tensor-arithmetic dispatch below.
    const bool pointerStringConcat =
        leftValue && rightValue &&
        leftValue->getType()->isPointerTy() && rightValue->getType()->isPointerTy() &&
        (!left->getType() || left->getType()->getKind() != Type::Kind::CLASS) &&
        (!right->getType() || right->getType()->getKind() != Type::Kind::CLASS) &&
        !TensorRuntime::isTensorTypeName(leftTypeName) &&
        !TensorRuntime::isTensorTypeName(rightTypeName);
    if (op == "+" && (leftTypeName == "String" || rightTypeName == "String" || pointerStringConcat)) {
        auto leftStaticType = left->getType();
        auto rightStaticType = right->getType();
        return codegenStringConcat(cg, leftValue, leftStaticType.get(), rightValue, rightStaticType.get());
    }

    if ((op == "==" || op == "!=") && (leftTypeName == "String" || rightTypeName == "String" || pointerStringConcat)) {
        // String comparison is by content, not pointer identity: two equal strings held in
        // distinct globals must compare equal. csec_string_equals returns a non-zero i32 when
        // the contents match.
        auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
        auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());
        auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
        auto leftStaticType = left->getType();
        auto rightStaticType = right->getType();
        llvm::Value* leftString = coerceValueForString(cg, leftValue, leftStaticType.get());
        llvm::Value* rightString = coerceValueForString(cg, rightValue, rightStaticType.get());
        llvm::Value* equals = cg.builder.CreateCall(
            getOrCreateRuntimeFunction("csec_string_equals", i32Ty, {i8PtrTy, i8PtrTy}),
            {leftString, rightString},
            "str.equals");
        llvm::Value* zero = llvm::ConstantInt::get(i32Ty, 0);
        return op == "=="
            ? cg.builder.CreateICmpNE(equals, zero, "str.eq")
            : cg.builder.CreateICmpEQ(equals, zero, "str.ne");
    }

    if (isLogicalOperator(op)) {
        llvm::Value* leftBool = coerceToBool(leftValue);
        llvm::Value* rightBool = coerceToBool(rightValue);
        if (!leftBool || !rightBool) {
            std::cerr << "Type error: Operator '" << op << "' requires boolean or numeric operands" << std::endl;
            return nullptr;
        }
        if (op == "implies") {
            return cg.builder.CreateOr(cg.builder.CreateNot(leftBool, "notlhs"), rightBool, "impliestmp");
        }
        return cg.builder.CreateICmpEQ(leftBool, rightBool, "ifftmp");
    }

    if (isSetOperator(op)) {
        llvm::Value* leftSet = coerceToI64(leftValue);
        llvm::Value* rightSet = coerceToI64(rightValue);
        if (!leftSet || !rightSet) {
            std::cerr << "Type error: Set operator '" << op << "' requires numeric bitset operands" << std::endl;
            return nullptr;
        }
        if (op == "union") return cg.builder.CreateOr(leftSet, rightSet, "setunion");
        if (op == "intersect") return cg.builder.CreateAnd(leftSet, rightSet, "setintersect");
        if (op == "symdiff") return cg.builder.CreateXor(leftSet, rightSet, "setsymdiff");
        return cg.builder.CreateAnd(leftSet, cg.builder.CreateNot(rightSet, "setnotrhs"), "setdiff");
    }

    if (isSetPredicateOperator(op)) {
        llvm::Value* leftSet = coerceToI64(leftValue);
        llvm::Value* rightSet = coerceToI64(rightValue);
        if (!leftSet || !rightSet) {
            std::cerr << "Type error: Set predicate '" << op << "' requires numeric bitset operands" << std::endl;
            return nullptr;
        }

        auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
        llvm::Value* zero = llvm::ConstantInt::get(i64Ty, 0);
        if (op == "in" || op == "notin") {
            llvm::Value* one = llvm::ConstantInt::get(i64Ty, 1);
            llvm::Value* bit = cg.builder.CreateShl(one, leftSet, "memberbit");
            llvm::Value* contains = cg.builder.CreateICmpNE(cg.builder.CreateAnd(rightSet, bit, "membermask"), zero, "contains");
            return op == "notin" ? cg.builder.CreateNot(contains, "notcontains") : contains;
        }

        llvm::Value* leftMinusRight = cg.builder.CreateAnd(leftSet, cg.builder.CreateNot(rightSet, "prednotrhs"), "leftminusright");
        llvm::Value* rightMinusLeft = cg.builder.CreateAnd(rightSet, cg.builder.CreateNot(leftSet, "prednotlhs"), "rightminusleft");
        llvm::Value* leftSubsetRight = cg.builder.CreateICmpEQ(leftMinusRight, zero, "subsettmp");
        llvm::Value* leftSupersetRight = cg.builder.CreateICmpEQ(rightMinusLeft, zero, "supersettmp");
        if (op == "subset") return leftSubsetRight;
        if (op == "superset") return leftSupersetRight;

        llvm::Value* notEqual = cg.builder.CreateICmpNE(leftSet, rightSet, "setnetmp");
        if (op == "properSubset") return cg.builder.CreateAnd(leftSubsetRight, notEqual, "propersubsettmp");
        return cg.builder.CreateAnd(leftSupersetRight, notEqual, "propersupersettmp");
    }

    const bool leftIsTensor = TensorRuntime::isTensorTypeName(leftTypeName);
    const bool rightIsTensor = TensorRuntime::isTensorTypeName(rightTypeName);
    if (isTensorArithmeticOperator(op) && (leftIsTensor || rightIsTensor)) {
        if (leftIsTensor && rightIsTensor) {
            return TensorRuntime::elementwiseTensorTensor(cg, leftValue, rightValue, op);
        }
        if (leftIsTensor && isNumericTypeName(rightTypeName)) {
            return TensorRuntime::elementwiseTensorScalar(cg, leftValue, rightValue, op, true);
        }
        if (rightIsTensor && isNumericTypeName(leftTypeName)) {
            return TensorRuntime::elementwiseTensorScalar(cg, rightValue, leftValue, op, false);
        }
        std::cerr << "Type error: Tensor arithmetic requires tensor or numeric operands" << std::endl;
        return nullptr;
    }

    if (isTensorOperator(op)) {
        if (!TensorRuntime::isTensorTypeName(leftTypeName) || !TensorRuntime::isTensorTypeName(rightTypeName)) {
            std::cerr << "Type error: Tensor operator '" << op << "' requires tensor operands" << std::endl;
            return nullptr;
        }
        if (op == "inner") {
            return TensorRuntime::innerProduct(cg, leftValue, rightValue);
        }
        if (op == "outer" || op == "tensor") {
            return TensorRuntime::outerProduct(cg, leftValue, rightValue);
        }
        if (op == "@") {
            return TensorRuntime::matrixProduct(cg, leftValue, rightValue);
        }
    }

    if (isArithmeticOperator(op) && isNumericTypeName(leftTypeName) && isNumericTypeName(rightTypeName) &&
        leftTypeName != rightTypeName) {
        auto* targetType = (leftValue->getType()->isDoubleTy() || rightValue->getType()->isDoubleTy() ||
                            leftTypeName == "Real" || rightTypeName == "Real")
            ? llvm::Type::getDoubleTy(cg.context)
            : (leftValue->getType()->isFloatTy() || rightValue->getType()->isFloatTy()
                ? llvm::Type::getFloatTy(cg.context)
                : llvm::Type::getInt64Ty(cg.context));
        leftValue = coerceNumericValue(leftValue, targetType);
        rightValue = coerceNumericValue(rightValue, targetType);
        if (op == "+") return targetType->isFloatingPointTy() ? cg.builder.CreateFAdd(leftValue, rightValue, "faddtmp") : cg.builder.CreateAdd(leftValue, rightValue, "addtmp");
        if (op == "-") return targetType->isFloatingPointTy() ? cg.builder.CreateFSub(leftValue, rightValue, "fsubtmp") : cg.builder.CreateSub(leftValue, rightValue, "subtmp");
        if (op == "*") return targetType->isFloatingPointTy() ? cg.builder.CreateFMul(leftValue, rightValue, "fmultmp") : cg.builder.CreateMul(leftValue, rightValue, "multmp");
        if (op == "/") return targetType->isFloatingPointTy() ? cg.builder.CreateFDiv(leftValue, rightValue, "fdivtmp") : cg.builder.CreateSDiv(leftValue, rightValue, "divtmp");
        if (op == "%") return targetType->isFloatingPointTy() ? cg.builder.CreateFRem(leftValue, rightValue, "fmodtmp") : cg.builder.CreateSRem(leftValue, rightValue, "modtmp");
        if (op == "mod") return cg.builder.CreateSRem(coerceToI64(leftValue), coerceToI64(rightValue), "imodtmp");
        if (op == "div") return cg.builder.CreateSDiv(coerceToI64(leftValue), coerceToI64(rightValue), "idivtmp");
        if (op == "**") {
            auto* doubleTy = llvm::Type::getDoubleTy(cg.context);
            llvm::Value* leftDouble = coerceNumericValue(leftValue, doubleTy);
            llvm::Value* rightDouble = coerceNumericValue(rightValue, doubleTy);
            return cg.builder.CreateCall(getOrCreateRuntimeDoubleBinary("csec_math_pow"), { leftDouble, rightDouble }, "powtmp");
        }
    }

    if (op == "+") {
        if (leftTypeName == rightTypeName) {
            if (left->getType()->isIntegerTy()) {
                return cg.builder.CreateAdd(leftValue, rightValue, "addtmp");
            }
            else if (left->getType()->isFloatTy() || left->getType()->isDoubleTy()) {
                return cg.builder.CreateFAdd(leftValue, rightValue, "faddtmp");
            }
            else {
                std::cerr << "Type error: Operator '" << op << "' not applicable to type '" << leftTypeName << "' and '" << rightTypeName << "'" << std::endl;
                return nullptr;
            }
        }
        else {
            std::cerr << "Type error: Left and right expressions have different types for operator '" << op << "'" << std::endl;
            return nullptr;
        }
    }
    else if (op == "-") {
        if (leftTypeName == rightTypeName) {
            if (left->getType()->isIntegerTy()) {
                return cg.builder.CreateSub(leftValue, rightValue, "subtmp");
            }
            else if (left->getType()->isFloatTy() || left->getType()->isDoubleTy()) {
                return cg.builder.CreateFSub(leftValue, rightValue, "fsubtmp");
            }
            else {
                std::cerr << "Type error: Operator '" << op << "' not applicable to type '" << leftTypeName << "' and '" << rightTypeName << "'" << std::endl;
                return nullptr;
            }
        }
        else {
            std::cerr << "Type error: Operator '" << op << "' not applicable to type '" << leftTypeName << "' and '" << rightTypeName << "'" << std::endl;
            return nullptr;
        }
    }
    else if (op == "*") {
        if (leftTypeName == rightTypeName) {
            if (left->getType()->isIntegerTy()) {
                return cg.builder.CreateMul(leftValue, rightValue, "multmp");
            }
            else if (left->getType()->isFloatTy() || left->getType()->isDoubleTy()) {
                return cg.builder.CreateFMul(leftValue, rightValue, "fmultmp");
            }
            else {
                std::cerr << "Type error: Operator '" << op << "' not applicable to type '" << leftTypeName << "' and '" << rightTypeName << "'" << std::endl;
                return nullptr;
            }
        }
        else {
            std::cerr << "Type error: Operator '" << op << "' not applicable to type '" << leftTypeName << "' and '" << rightTypeName << "'" << std::endl;
            return nullptr;
        }
    }
    else if (op == "/") {
        if (leftTypeName == rightTypeName) {
            if (left->getType()->isIntegerTy()) {
                return cg.builder.CreateSDiv(leftValue, rightValue, "divtmp");
            }
            else if (left->getType()->isFloatTy() || left->getType()->isDoubleTy()) {
                return cg.builder.CreateFDiv(leftValue, rightValue, "fdivtmp");
            }
            else {
                std::cerr << "Type error: Operator '" << op << "' not applicable to type '" << leftTypeName << "' and '" << rightTypeName << "'" << std::endl;
                return nullptr;
            }
        }
        else {
            std::cerr << "Type error: Operator '" << op << "' not applicable to type '" << leftTypeName << "' and '" << rightTypeName << "'" << std::endl;
            return nullptr;
        }
    }
    else if (op == "div") {
        if (leftTypeName == rightTypeName && isNumericTypeName(leftTypeName)) {
            return cg.builder.CreateSDiv(coerceToI64(leftValue), coerceToI64(rightValue), "idivtmp");
        }
        std::cerr << "Type error: Operator '" << op << "' requires numeric operands" << std::endl;
        return nullptr;
    }
    else if (op == "**") {
        if (isNumericTypeName(leftTypeName) && isNumericTypeName(rightTypeName)) {
            auto* doubleTy = llvm::Type::getDoubleTy(cg.context);
            llvm::Value* leftDouble = coerceNumericValue(leftValue, doubleTy);
            llvm::Value* rightDouble = coerceNumericValue(rightValue, doubleTy);
            return cg.builder.CreateCall(getOrCreateRuntimeDoubleBinary("csec_math_pow"), { leftDouble, rightDouble }, "powtmp");
        }
        std::cerr << "Type error: Operator '" << op << "' requires numeric operands" << std::endl;
        return nullptr;
    }
    else if (op == "mod") {
        if (leftTypeName == rightTypeName && isNumericTypeName(leftTypeName)) {
            return cg.builder.CreateSRem(coerceToI64(leftValue), coerceToI64(rightValue), "imodtmp");
        }
        std::cerr << "Type error: Operator '" << op << "' requires numeric operands" << std::endl;
        return nullptr;
    }
    else if (op == "%") {
        if (leftTypeName == rightTypeName) {
            if (left->getType()->isIntegerTy()) {
                return cg.builder.CreateSRem(leftValue, rightValue, "modtmp");
            }
            else if (left->getType()->isFloatTy() || left->getType()->isDoubleTy()) {
                return cg.builder.CreateFRem(leftValue, rightValue, "fmodtmp");
            }
            else {
                std::cerr << "Type error: Operator '" << op << "' not applicable to type '" << leftTypeName << "' and '" << rightTypeName << "'" << std::endl;
                return nullptr;
            }
        }
        else {
            std::cerr << "Type error: Operator '" << op << "' not applicable to type '" << leftTypeName << "' and '" << rightTypeName << "'" << std::endl;
            return nullptr;
        }
    }
    else if ((op == "&" || op == "|" || op == "^" || op == "xor") && leftTypeName != rightTypeName &&
             left->getType()->isIntegerTy() && right->getType()->isIntegerTy() &&
             leftValue->getType()->isIntegerTy() && rightValue->getType()->isIntegerTy()) {
        // Mixed integer widths (`longValue & 255`): sign-extend the narrower operand, matching the
        // wider result type BinaryExpressionNode::getType() reports.
        const unsigned leftBits = leftValue->getType()->getIntegerBitWidth();
        const unsigned rightBits = rightValue->getType()->getIntegerBitWidth();
        if (leftBits < rightBits) leftValue = cg.builder.CreateSExt(leftValue, rightValue->getType(), "bit.sext");
        else if (rightBits < leftBits) rightValue = cg.builder.CreateSExt(rightValue, leftValue->getType(), "bit.sext");
        if (op == "&") return cg.builder.CreateAnd(leftValue, rightValue, "andtmp");
        if (op == "|") return cg.builder.CreateOr(leftValue, rightValue, "ortmp");
        return cg.builder.CreateXor(leftValue, rightValue, "xortmp");
    }
    else if ((op == "<<" || op == ">>") && leftTypeName != rightTypeName &&
             left->getType()->isIntegerTy() && right->getType()->isIntegerTy() &&
             leftValue->getType()->isIntegerTy() && rightValue->getType()->isIntegerTy()) {
        // `longValue >> intCount`: the shift count takes the width of the shifted value, which is
        // also the result type.
        rightValue = cg.builder.CreateIntCast(rightValue, leftValue->getType(), false, "shift.count");
        if (op == "<<") return cg.builder.CreateShl(leftValue, rightValue, "shltmp");
        return cg.builder.CreateAShr(leftValue, rightValue, "ashrtmp");
    }
    else if (op == "&") {
        if (leftTypeName != rightTypeName || !left->getType()->isIntegerTy()) {
            std::cerr << "Type error: Operator '" << op << "' requires same integer operand types" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateAnd(leftValue, rightValue, "andtmp");
    }
    else if (op == "|") {
        if (leftTypeName != rightTypeName || !left->getType()->isIntegerTy()) {
            std::cerr << "Type error: Operator '" << op << "' requires same integer operand types" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateOr(leftValue, rightValue, "ortmp");
    }
    else if (op == "xor" || op == "^") {
        const bool isBooleanXor =
            (leftTypeName == "Boolean" || leftTypeName == "Bool") &&
            (rightTypeName == "Boolean" || rightTypeName == "Bool");
        if ((leftTypeName != rightTypeName && !isBooleanXor) ||
            (!isBooleanXor && !left->getType()->isIntegerTy())) {
            std::cerr << "Type error: Operator '" << op << "' requires same integer or boolean operand types" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateXor(leftValue, rightValue, "xortmp");
    }
    else if (op == "<<") {
        if (leftTypeName != rightTypeName || !left->getType()->isIntegerTy()) {
            std::cerr << "Type error: Operator '" << op << "' requires same integer operand types" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateShl(leftValue, rightValue, "shltmp");
    }
    else if (op == ">>") {
        if (leftTypeName != rightTypeName || !left->getType()->isIntegerTy()) {
            std::cerr << "Type error: Operator '" << op << "' requires same integer operand types" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateAShr(leftValue, rightValue, "ashrtmp");
    }
    else if (op == ">" || op == "<" || op == "==" || op == "!=" || op == ">=" || op == "<=") {
        if (isNumericTypeName(leftTypeName) && isNumericTypeName(rightTypeName) &&
            leftValue->getType() != rightValue->getType()) {
            llvm::Type* targetType =
                (leftValue->getType()->isDoubleTy() || rightValue->getType()->isDoubleTy() ||
                 leftTypeName == "Real" || rightTypeName == "Real" ||
                 leftTypeName == "Double" || rightTypeName == "Double")
                    ? llvm::Type::getDoubleTy(cg.context)
                    : (leftValue->getType()->isFloatTy() || rightValue->getType()->isFloatTy() ||
                       leftTypeName == "Float" || rightTypeName == "Float")
                        ? llvm::Type::getFloatTy(cg.context)
                        : llvm::Type::getInt64Ty(cg.context);
            leftValue = coerceNumericValue(leftValue, targetType);
            rightValue = coerceNumericValue(rightValue, targetType);
        }

        if (leftValue->getType() != rightValue->getType()) {
            llvm::Type* targetType = nullptr;
            if (leftValue->getType()->isIntegerTy() && rightValue->getType()->isIntegerTy()) {
                targetType = llvm::Type::getInt64Ty(cg.context);
            }
            else if (leftValue->getType()->isFloatingPointTy() && rightValue->getType()->isFloatingPointTy()) {
                targetType = (leftValue->getType()->isDoubleTy() || rightValue->getType()->isDoubleTy())
                    ? llvm::Type::getDoubleTy(cg.context)
                    : llvm::Type::getFloatTy(cg.context);
            }
            if (targetType) {
                leftValue = coerceNumericValue(leftValue, targetType);
                rightValue = coerceNumericValue(rightValue, targetType);
            }
        }
        if (leftValue->getType() != rightValue->getType()) {
            std::cerr << "Type error: Comparison operands have incompatible LLVM types" << std::endl;
            return nullptr;
        }

        const bool isFloatCompare = leftValue->getType()->isFloatingPointTy();
        if (isFloatCompare) {
            if (op == ">") return cg.builder.CreateFCmpOGT(leftValue, rightValue, "gttmp");
            if (op == "<") return cg.builder.CreateFCmpOLT(leftValue, rightValue, "lttmp");
            if (op == "==") return cg.builder.CreateFCmpOEQ(leftValue, rightValue, "eqtmp");
            if (op == "!=") return cg.builder.CreateFCmpONE(leftValue, rightValue, "netmp");
            if (op == ">=") return cg.builder.CreateFCmpOGE(leftValue, rightValue, "getmp");
            if (op == "<=") return cg.builder.CreateFCmpOLE(leftValue, rightValue, "letmp");
        }
        else {
            if (op == ">") return cg.builder.CreateICmpSGT(leftValue, rightValue, "gttmp");
            if (op == "<") return cg.builder.CreateICmpSLT(leftValue, rightValue, "lttmp");
            if (op == "==") return cg.builder.CreateICmpEQ(leftValue, rightValue, "eqtmp");
            if (op == "!=") return cg.builder.CreateICmpNE(leftValue, rightValue, "netmp");
            if (op == ">=") return cg.builder.CreateICmpSGE(leftValue, rightValue, "getmp");
            if (op == "<=") return cg.builder.CreateICmpSLE(leftValue, rightValue, "letmp");
        }
        std::cerr << "Unsupported comparison operator: " << op << std::endl;
        return nullptr;
    }

    if (isNumericTypeName(leftTypeName) && isNumericTypeName(rightTypeName)) {
        return llvm::ConstantFP::get(llvm::Type::getDoubleTy(cg.context), 0.0);
    }

    std::cerr << "Type error: unsupported operator '" << op << "'" << std::endl;
    return nullptr;
}

std::unique_ptr<Type> BinaryExpressionNode::getType() {
    if (type) return type->clone();

    auto leftType = left->getType();
    auto rightType = right->getType();

    if (!leftType || !rightType) {
        return std::make_unique<UnknownType>();
    }

    if ((op == "in" || op == "notin") && (isDictType(rightType.get()) || isSetType(rightType.get()))) {
        return std::make_unique<BasicType>("Boolean");
    }
    if (isSetType(leftType.get()) && isSetType(rightType.get())) {
        auto result = setBinaryOperatorType(op, leftType.get(), rightType.get());
        return result ? std::move(result) : std::make_unique<UnknownType>();
    }

    if (op == "+" && (leftType->getName() == "String" || rightType->getName() == "String")) {
        return std::make_unique<BasicType>("String");
    }

    // Nat (arbitrary-precision natural number): +/-/* with either operand a Nat -> Nat; comparisons ->
    // Boolean. Mixed Nat/integer promotes the integer side. A Nat + String is concatenation (handled
    // above by the String rule), so only fall here when the other operand is not a String.
    if (leftType->getName() == "Nat" || rightType->getName() == "Nat") {
        if (op == "+" || op == "-" || op == "*") {
            return std::make_unique<ClassType>("Nat");
        }
        if (op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=") {
            return std::make_unique<BasicType>("Boolean");
        }
    }

    if (leftType->getKind() == Type::Kind::CLASS) {
        auto* classSymbol = CodeGenerator::getInstance().symbolTable.lookupClass(leftType->getName());
        // Prefer the overload whose parameter matches the right operand, then any `operator<op>`.
        Symbol* methodSymbol = nullptr;
        if (classSymbol) {
            std::vector<std::unique_ptr<Type>> argTypes;
            argTypes.push_back(rightType->clone());
            methodSymbol = CodeGenerator::getInstance().symbolTable.lookupMethod(*classSymbol, "operator" + op, argTypes);
            if (!methodSymbol) methodSymbol = CodeGenerator::getInstance().symbolTable.lookupMethod(*classSymbol, "operator" + op);
        }
        if (methodSymbol) {
            auto* funcType = dynamic_cast<FunctionType*>(methodSymbol->type.get());
            if (funcType && funcType->returnType) {
                return funcType->returnType->clone();
            }
        }
    }

    if (!leftType->equals(rightType)) {
        if (isSetOperator(op)) {
            return std::make_unique<BasicType>("Long");
        }
        if (isSetPredicateOperator(op) || isLogicalOperator(op)) {
            return std::make_unique<BasicType>("Boolean");
        }
        const bool leftIsTensor = TensorRuntime::isTensorTypeName(leftType->getName());
        const bool rightIsTensor = TensorRuntime::isTensorTypeName(rightType->getName());
        if (isTensorArithmeticOperator(op) && (leftIsTensor || rightIsTensor)) {
            if (leftIsTensor) {
                return leftType->clone();
            }
            if (rightIsTensor) {
                return rightType->clone();
            }
        }
        if (isTensorOperator(op)) {
            if (op == "inner") {
                return std::make_unique<BasicType>("Real");
            }
            return leftType ? leftType->clone() : std::make_unique<UnknownType>();
        }
        if (op == ">" || op == "<" || op == "==" || op == "!=" || op == ">=" || op == "<=" ||
            op == "and" || op == "or") {
            return std::make_unique<BasicType>("Boolean");
        }
        if ((op == "&" || op == "|" || op == "^" || op == "xor" || op == "<<" || op == ">>") &&
            leftType->isIntegerTy() && rightType->isIntegerTy()) {
            if (op == "<<" || op == ">>") {
                return leftType->clone();
            }
            auto rank = [](const std::string& name) {
                if (name == "Byte") return 1;
                if (name == "Short") return 2;
                if (name == "Int") return 3;
                return 4;
            };
            return rank(leftType->getName()) >= rank(rightType->getName()) ? leftType->clone() : rightType->clone();
        }
        if (isArithmeticOperator(op) && isNumericTypeName(leftType->getName()) && isNumericTypeName(rightType->getName())) {
            if (op == "**") {
                return std::make_unique<BasicType>("Real");
            }
            if (op == "div" || op == "mod") {
                return std::make_unique<BasicType>("Long");
            }
            if (leftType->getName() == "Real" || rightType->getName() == "Real" ||
                leftType->getName() == "Double" || rightType->getName() == "Double") {
                return std::make_unique<BasicType>("Real");
            }
            if (leftType->getName() == "Float" || rightType->getName() == "Float") {
                return std::make_unique<BasicType>("Float");
            }
            return std::make_unique<BasicType>("Long");
        }
        return std::make_unique<UnknownType>();
    }

    if (isSetOperator(op)) {
        return std::make_unique<BasicType>("Long");
    }
    if (isSetPredicateOperator(op) || isLogicalOperator(op)) {
        return std::make_unique<BasicType>("Boolean");
    }

    if (isTensorOperator(op)) {
        if (op == "inner") {
            return std::make_unique<BasicType>("Real");
        }
        return left->getType();
    }
    if (isArithmeticOperator(op)) {
        if (TensorRuntime::isTensorTypeName(leftType->getName()) && isTensorArithmeticOperator(op)) {
            return left->getType();
        }
        if (op == "**") {
            if (isNumericTypeName(leftType->getName())) {
                return std::make_unique<BasicType>("Real");
            }
            return std::make_unique<UnknownType>();
        }
        if (op == "div" || op == "mod") {
            if (isNumericTypeName(leftType->getName())) {
                return std::make_unique<BasicType>("Long");
            }
            return std::make_unique<UnknownType>();
        }
        if (leftType->getName() == "Int" || leftType->getName() == "Float" || leftType->getName() == "Double"
            || leftType->getName() == "Long" || leftType->getName() == "Natural" || leftType->getName() == "Integer"
            || leftType->getName() == "Real" || leftType->getName() == "Complex" || leftType->getName() == "Rational"
            || leftType->getName() == "Quaternion") {
            return left->getType();
        }
        else {
            return std::make_unique<UnknownType>();
        }
    }
    else if (op == ">" || op == "<" || op == "==" || op == "!=" || op == ">=" || op == "<=") {
        return std::make_unique<BasicType>("Boolean");
    }
    else if (op == "and" || op == "or") {
        return std::make_unique<BasicType>("Boolean");
    }
    else if (op == "xor" || op == "^") {
        if (leftType->getName() == "Boolean" || leftType->getName() == "Bool") {
            return std::make_unique<BasicType>("Boolean");
        }
        if (!leftType->isIntegerTy()) {
            return std::make_unique<UnknownType>();
        }
        return left->getType();
    }
    else if (op == "&" || op == "|" || op == "<<" || op == ">>") {
        if (!leftType->isIntegerTy()) {
            return std::make_unique<UnknownType>();
        }
        return left->getType();
    }

    if (isNumericTypeName(leftType->getName()) && isNumericTypeName(rightType->getName())) {
        return std::make_unique<BasicType>("Real");
    }

    return std::make_unique<UnknownType>();
}



// Exported for MethodCallNode (overload arguments chosen through numeric widening).
llvm::Value* csecCoerceNumericValue(llvm::Value* value, llvm::Type* targetType) {
    return coerceNumericValue(value, targetType);
}
