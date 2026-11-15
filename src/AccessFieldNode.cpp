#include "codegen.h"
#include "AccessFieldNode.h"
#include "builtin_structs.h"
#include "ASTVisitor.h"
#include "utils.h"
#include "type_utils.h"
#include "TensorRuntime.h"

#include "IdentifierNode.h"
#include "VariableDeclarationNode.h"

#include <iostream>

namespace {
llvm::Function* getOrCreateRuntimeFunction(const std::string& name, llvm::Type* returnType, const std::vector<llvm::Type*>& paramTypes) {
    auto& cg = CodeGenerator::getInstance();
    if (auto* function = cg.module->getFunction(name)) return function;
    auto* functionTy = llvm::FunctionType::get(returnType, paramTypes, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, name, cg.module.get());
}

// A vector's named components: v.x/v.y/v.z/v.w address elements 0..3 of a tensor. Lets a rank-1 tensor
// read like the Vec3 it stands in for. Returns -1 for any other field name.
int tensorComponentIndex(const std::string& name) {
    if (name == "x") return 0;
    if (name == "y") return 1;
    if (name == "z") return 2;
    if (name == "w") return 3;
    return -1;
}
}

llvm::Value* AccessFieldNode::codegenThroughTemporary(const std::function<llvm::Value*()>& access) {
    static int counter = 0;
    auto baseType = this->base->getType();
    if (!baseType || baseType->getKind() == Type::Kind::UNKNOWN) {
        std::cerr << "Error: cannot determine the type of the value whose field is read" << std::endl;
        return nullptr;
    }
    const std::string name = "__field_base_" + std::to_string(counter++);
    VariableDeclarationNode temporary(name, baseType->clone(), std::move(this->base), false);
    temporary.hasExplicitType = true;
    temporary.codegen();
    this->base = std::make_unique<IdentifierNode>(name);
    llvm::Value* result = access();
    this->base = std::move(temporary.initializer);
    return result;
}

void AccessFieldNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* AccessFieldNode::codegen() {
    if (!this->base || !this->field) {
        return nullptr;
    }

    // Nested chain read (`a.b.c`): the base is an AccessFieldNode. Take the element address and load it.
    if (dynamic_cast<AccessFieldNode*>(this->base.get())) {
        auto& cg = CodeGenerator::getInstance();
        llvm::Value* fieldPtr = codegenFieldPointer();
        if (!fieldPtr) return nullptr;
        llvm::Type* fieldLLVMType = getABIStorageType(getType().get());
        if (!fieldLLVMType) return nullptr;
        auto* fid = dynamic_cast<IdentifierNode*>(this->field.get());
        return cg.builder.CreateLoad(fieldLLVMType, fieldPtr, (fid ? fid->value : "field") + ".value");
    }

    // Built-in value aggregates (Complex/Rational/Quaternion): any expression may be the base, e.g.
    // `conj(z).im`; the field is an extractvalue of the aggregate value.
    if (auto* builtinField = dynamic_cast<IdentifierNode*>(this->field.get())) {
        auto aggregateType = stripBorrow(this->base->getType());
        const int builtinIndex = aggregateType ? builtinValueStructFieldIndex(aggregateType->getName(), builtinField->value) : -1;
        if (builtinIndex >= 0) {
            auto& cg = CodeGenerator::getInstance();
            llvm::Value* aggregate = this->base->codegen();
            if (!aggregate) return nullptr;
            if (aggregate->getType()->isPointerTy()) {
                aggregate = cg.builder.CreateLoad(cg.getLLVMType(aggregateType.get()), aggregate, "aggregate.value");
            }
            return cg.builder.CreateExtractValue(aggregate, { static_cast<unsigned>(builtinIndex) }, builtinField->value);
        }
    }

    auto* baseIdentifier = dynamic_cast<IdentifierNode*>(this->base.get());
    if (!baseIdentifier) {
        // `f().x`, `(a + b).y`, `o.m().field`: evaluate the base once into a hidden local and read
        // the field through it, exactly as for a named value.
        return codegenThroughTemporary([this]() { return codegen(); });
    }

    auto* fieldIdentifier = dynamic_cast<IdentifierNode*>(this->field.get());
    if (!fieldIdentifier) {
        std::cerr << "Error: Field must be an identifier" << std::endl;
        return nullptr;
    }

    auto baseType = stripBorrow(baseIdentifier->getType());
    auto targetName = fieldIdentifier->value;
    if (baseType && baseType->isStringTy()) {
        llvm::Value* stringValue = baseIdentifier->codegen();
        if (!stringValue) {
            return nullptr;
        }
        if (targetName == "length" || targetName == "size" || targetName == "count") {
            auto& cg = CodeGenerator::getInstance();
            auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
            auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
            return cg.builder.CreateCall(
                getOrCreateRuntimeFunction("csec_string_length", i64Ty, {i8PtrTy}),
                {stringValue},
                "str.length");
        }
        if (targetName == "isEmpty") {
            auto& cg = CodeGenerator::getInstance();
            auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
            auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
            auto* result = cg.builder.CreateCall(
                getOrCreateRuntimeFunction("csec_string_is_empty", i32Ty, {i8PtrTy}),
                {stringValue},
                "str.empty.i32");
            return cg.builder.CreateICmpNE(result, llvm::ConstantInt::get(i32Ty, 0), "str.empty");
        }
        std::cerr << "Error: String property '" << targetName << "' not found" << std::endl;
        return nullptr;
    }

    // Named component of a vector (rank-1 tensor): v.x / v.y / v.z / v.w -> element 0..3.
    if (baseType && TensorRuntime::isTensorTypeName(baseType->getName())) {
        int comp = tensorComponentIndex(targetName);
        if (comp >= 0) {
            llvm::Value* tensorVal = baseIdentifier->codegen();
            if (!tensorVal) return nullptr;
            auto& cg = CodeGenerator::getInstance();
            auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
            llvm::Value* addr = TensorRuntime::elementPointer(cg, tensorVal, {llvm::ConstantInt::get(i64Ty, comp)});
            return cg.builder.CreateLoad(llvm::Type::getDoubleTy(cg.context), addr, targetName + ".comp");
        }
    }

    // Reading a field yields its value. The address is produced by codegenFieldPointer(); load it.
    llvm::Value* fieldPtr = codegenFieldPointer();
    if (!fieldPtr) {
        return nullptr;
    }
    auto& cg = CodeGenerator::getInstance();
    // Load with the field's ABI *storage* type, which is how it was laid out in the struct. For a
    // reference-typed field (a non-struct class such as BigInt) that is a `ptr` handle, not the pointee
    // struct that getLLVMType would return -- loading the struct-by-value from the slot would be wrong.
    llvm::Type* fieldLLVMType = getABIStorageType(getType().get());
    if (!fieldLLVMType) {
        std::cerr << "Error: Field '" << targetName << "' has an unsupported type" << std::endl;
        return nullptr;
    }
    return cg.builder.CreateLoad(fieldLLVMType, fieldPtr, targetName + ".value");
}

llvm::Value* AccessFieldNode::codegenFieldPointer() {
    auto* fieldIdentifier = dynamic_cast<IdentifierNode*>(this->field.get());
    if (!fieldIdentifier) {
        std::cerr << "Error: Field must be an identifier" << std::endl;
        return nullptr;
    }
    const auto targetName = fieldIdentifier->value;

    // Nested chain: `a.b.c` -- the base is itself an AccessFieldNode (`a.b`). Recurse to the pointer to
    // the base value-struct, then StructGEP the field on the base's declared class/struct type. (Only
    // inline value-struct fields chain this way; a reference field would need a load, not handled here.)
    if (auto* baseField = dynamic_cast<AccessFieldNode*>(this->base.get())) {
        auto& cg = CodeGenerator::getInstance();
        llvm::Value* basePtr = baseField->codegenFieldPointer();
        if (!basePtr) return nullptr;
        auto baseTy = stripBorrow(baseField->getType());
        if (!baseTy || baseTy->getKind() != Type::Kind::CLASS) {
            std::cerr << "Error: '" << targetName << "' base is not a struct/class" << std::endl;
            return nullptr;
        }
        auto* classSymbol = cg.symbolTable.lookupClass(baseTy->getName());
        auto* structType = classSymbol ? llvm::dyn_cast<llvm::StructType>(classSymbol->classType) : nullptr;
        if (!structType) {
            std::cerr << "Error: '" << baseTy->getName() << "' has no struct layout" << std::endl;
            return nullptr;
        }
        // A reference-class field is stored as a pointer in its containing struct.  Its field
        // address is therefore a pointer-to-pointer; load the referenced object before selecting
        // the next member.  Value structs remain inline and can use their field address directly.
        if (!isStructClassType(baseTy.get())) {
            llvm::Type* storageType = getABIStorageType(baseTy.get());
            if (!storageType || !storageType->isPointerTy()) {
                std::cerr << "Error: Invalid reference field storage for '" << baseTy->getName() << "'" << std::endl;
                return nullptr;
            }
            basePtr = cg.builder.CreateLoad(storageType, basePtr, "field.object");
        }
        int fieldIndex = findFieldIndex(classSymbol, targetName);
        if (fieldIndex == -1) {
            std::cerr << "Error: Field '" << targetName << "' not found in '" << baseTy->getName() << "'" << std::endl;
            return nullptr;
        }
        auto* targetPtrType = llvm::PointerType::getUnqual(structType->getContext());
        if (basePtr->getType() != targetPtrType) {
            basePtr = cg.builder.CreateBitCast(basePtr, targetPtrType, "obj.cast");
        }
        return cg.builder.CreateStructGEP(structType, basePtr, static_cast<unsigned>(fieldIndex), targetName);
    }

    auto* baseIdentifier = dynamic_cast<IdentifierNode*>(this->base.get());
    if (!baseIdentifier) {
        return codegenThroughTemporary([this]() { return codegenFieldPointer(); });
    }
    auto baseType = stripBorrow(baseIdentifier->getType());

    // Vector component as an assignment target: `nvec.x = ...` -> address of element 0..3.
    if (baseType && TensorRuntime::isTensorTypeName(baseType->getName())) {
        int comp = tensorComponentIndex(targetName);
        if (comp >= 0) {
            llvm::Value* tensorVal = baseIdentifier->codegen();
            if (!tensorVal) return nullptr;
            auto& cg = CodeGenerator::getInstance();
            auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
            return TensorRuntime::elementPointer(cg, tensorVal, {llvm::ConstantInt::get(i64Ty, comp)});
        }
    }

    if (!baseType || baseType->getKind() != Type::Kind::CLASS) {
        std::cerr << "Error: Base must be a class type" << std::endl;
        return nullptr;
    }

    auto* classSymbol = CodeGenerator::getInstance().symbolTable.lookupClass(baseType->getName());
    if (!classSymbol) {
        std::cerr << "Error: Class '" << baseType->getName() << "' not found" << std::endl;
        return nullptr;
    }

    auto* thisSymbol = CodeGenerator::getInstance().symbolTable.lookup(baseIdentifier->value);
    if (!thisSymbol) {
        std::cerr << "Error: Base object not found" << std::endl;
        return nullptr;
    }

    auto* structType = llvm::dyn_cast<llvm::StructType>(classSymbol->classType);
    if (!structType) {
        std::cerr << "Error: Class '" << baseType->getName() << "' has invalid LLVM struct type" << std::endl;
        return nullptr;
    }

    llvm::Value* basePtr = thisSymbol->value;
    if (!basePtr || !basePtr->getType()->isPointerTy()) {
        std::cerr << "Error: Base object value is not a pointer" << std::endl;
        return nullptr;
    }

    auto* targetPtrType = llvm::PointerType::getUnqual(structType->getContext());
    if (basePtr->getType() != targetPtrType) {
        basePtr = CodeGenerator::getInstance().builder.CreateBitCast(basePtr, targetPtrType, "obj.cast");
    }

    int fieldIndex = findFieldIndex(classSymbol, targetName);
    if (fieldIndex == -1) {
        std::cerr << "Error: Field '" << targetName << "' not found in class '"
            << baseType->getName() << "'" << std::endl;
        return nullptr;
    }

    return CodeGenerator::getInstance().builder.CreateStructGEP(
        structType,
        basePtr,
        static_cast<unsigned>(fieldIndex),
        targetName
    );
}

int AccessFieldNode::findFieldIndex(ClassSymbol* classSymbol, const std::string& fieldName) {
    int idx = 0;

    for (const auto& field : classSymbol->constructorParamOrder) {
        if (field == fieldName) {
            return idx;
        }
        idx++;
    }

    for (const auto& field : classSymbol->fieldOrder) {
        if (field == fieldName) {
            return idx;
        }
        idx++;
    }

    return -1;
}

std::unique_ptr<Type> AccessFieldNode::getType() {
    auto baseType = base ? stripBorrow(base->getType()) : nullptr;
    auto* fieldIdentifier = dynamic_cast<IdentifierNode*>(field.get());
    // A vector component (v.x/v.y/v.z/v.w) is a scalar tensor element.
    if (baseType && fieldIdentifier && TensorRuntime::isTensorTypeName(baseType->getName()) &&
        tensorComponentIndex(fieldIdentifier->value) >= 0) {
        return std::make_unique<BasicType>("Double");
    }
    if (baseType && fieldIdentifier && builtinValueStructFieldIndex(baseType->getName(), fieldIdentifier->value) >= 0) {
        return std::make_unique<BasicType>(builtinValueStructFieldType(baseType->getName()));
    }
    if (baseType && baseType->isStringTy() && fieldIdentifier) {
        const std::string& targetName = fieldIdentifier->value;
        if (targetName == "length" || targetName == "size" || targetName == "count") {
            return std::make_unique<BasicType>("Long");
        }
        if (targetName == "isEmpty") {
            return std::make_unique<BasicType>("Boolean");
        }
    }
    // A class/struct field has a declared type; resolve it from the class symbol. Falling back to the
    // field identifier's own getType() assumes Real (double) and mistypes non-Double fields -- e.g.
    // the i64 components of a Nat/Gaussian integer would be loaded as a double.
    if (baseType && baseType->getKind() == Type::Kind::CLASS && fieldIdentifier) {
        auto* classSymbol = CodeGenerator::getInstance().symbolTable.lookupClass(baseType->getName());
        if (classSymbol) {
            const std::string& fname = fieldIdentifier->value;
            auto cp = classSymbol->constructorParams.find(fname);
            if (cp != classSymbol->constructorParams.end() && cp->second && cp->second->type) {
                return cp->second->type->clone();
            }
            auto fp = classSymbol->fields.find(fname);
            if (fp != classSymbol->fields.end() && fp->second && fp->second->type) {
                return fp->second->type->clone();
            }
        }
    }
    if (!field) return std::make_unique<UnknownType>();
    return field->getType();
}
