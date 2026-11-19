#include "codegen.h"
#include "ArrayCreationExpressionNode.h"
#include "ASTVisitor.h"

#include <iostream>
#include "utils.h"

void ArrayCreationExpressionNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* ArrayCreationExpressionNode::codegen() {
    // sizes로 배열 크기를 받아서 처리하는데 이들은 4차원이 최대이므로, 이들을 곱한다.
    llvm::Value* array_size = sizes[0]->codegen();
    for (int i = 1; i < sizes.size(); i++) {
        array_size = CodeGenerator::getInstance().builder.CreateMul(array_size, sizes[i]->codegen());
    }

    auto& cg = CodeGenerator::getInstance();
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);

    // Arrays are heap allocated, not stack allocated, so a `new T[n]` value can escape the
    // function that created it (for example by being returned). A stack `alloca` would leave the
    // caller holding a pointer into a freed frame.
    auto heapAllocate = [&](llvm::Type* elementType) -> llvm::Value* {
        const llvm::DataLayout& dl = cg.module->getDataLayout();
        uint64_t elementBytes = dl.getTypeAllocSize(elementType);
        llvm::Value* count = cg.builder.CreateZExtOrTrunc(array_size, i64Ty, "array.count");
        llvm::Value* bytes = cg.builder.CreateMul(
            count, llvm::ConstantInt::get(i64Ty, elementBytes), "array.bytes");
        return cg.builder.CreateCall(cg.mallocFunction, bytes, typeName);
    };

    // primitive type인지, class type인지 확인, template type인지 확인
    if (isPrimitiveType(typeName)) {
		BasicType basicType(typeName);
        llvm::Type* type = cg.getLLVMType(&basicType);
        llvm::Value* memory = heapAllocate(type);
        if (memory && type && type->isPointerTy() && cg.isRefcountedType(&basicType)) {
            llvm::Value* count = cg.builder.CreateZExtOrTrunc(array_size, i64Ty, "array.count");
            llvm::Value* bytes = cg.builder.CreateMul(
                count, llvm::ConstantInt::get(i64Ty, cg.module->getDataLayout().getTypeAllocSize(type)), "array.bytes");
            cg.builder.CreateMemSet(memory, cg.builder.getInt8(0), bytes, llvm::MaybeAlign(8));
            cg.trackArrayElements(memory, count);
        }
        return memory;
    }

    // Template Class인지 확인은 나중에 코드 추가되면...

    // 클래스 심볼 찾기
    auto classSymbolOpt = CodeGenerator::getInstance().symbolTable.lookupClass(typeName);
    if (!classSymbolOpt) {
        std::cerr << "Error: Class '" << typeName << "' not found" << std::endl;
        return nullptr;
    }

    auto* classSymbol = classSymbolOpt;

    // 클래스 타입 가져오기
    auto* classType = static_cast<llvm::StructType*>(classSymbol->classType);

    // 메모리 할당 (heap, so the array can escape via return)
    llvm::Value* allocatedMemory = heapAllocate(classType);

    // 객체 초기화 (생성자 호출)
    std::string constructorName = typeName + "_constructor";
    llvm::Function* constructorFunc = CodeGenerator::getInstance().module->getFunction(constructorName);
    if (constructorFunc) {
        std::vector<llvm::Value*> constructorArgs = { allocatedMemory };
        for (auto& arg : sizes) {
            constructorArgs.push_back(arg->codegen());
        }
        CodeGenerator::getInstance().builder.CreateCall(constructorFunc, constructorArgs);
    }
    else {
        std::cerr << "Warning: Constructor for class '" << typeName << "' not found" << std::endl;
    }

    return allocatedMemory;
}

std::unique_ptr<Type> ArrayCreationExpressionNode::getType() {
    std::vector<std::unique_ptr<Type>> typeArgs;
    typeArgs.push_back(std::make_unique<BasicType>(typeName));
    std::unique_ptr<Type> baseType = std::make_unique<ClassType>("Array");
    return std::make_unique<GenericType>(baseType, typeArgs);
}
