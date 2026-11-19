#include "codegen.h"
#include "MapStatementNode.h"
#include "ASTVisitor.h"
#include "ArrayLiteralNode.h"

#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>

namespace {
std::unique_ptr<Type> mapElementSourceType(const std::unique_ptr<Type>& type) {
    if (!type) return std::make_unique<UnknownType>();
    if (auto* arrType = dynamic_cast<ArrayType*>(type.get())) {
        return arrType->elementType ? arrType->elementType->clone() : std::make_unique<UnknownType>();
    }
    if (auto* genType = dynamic_cast<GenericType*>(type.get())) {
        if (genType->baseType && genType->baseType->getName() == "Array" && !genType->typeArguments.empty()) {
            return genType->typeArguments[0] ? genType->typeArguments[0]->clone() : std::make_unique<UnknownType>();
        }
    }
    return std::make_unique<UnknownType>();
}
}

void MapStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* MapStatementNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    llvm::Function* function = cg.builder.GetInsertBlock()->getParent();

    llvm::Value* arrayPtr = iterableExpr->codegen();
    if (!arrayPtr) return nullptr;

    auto iterType = iterableExpr->getType();
    if (!iterType) return nullptr;

    // Extract element type and array size
    llvm::Type* elementType = nullptr;
    int arraySize = 0;
    bool sizeKnown = false;

    if (auto* arrType = dynamic_cast<ArrayType*>(iterType.get())) {
        elementType = cg.getLLVMType(arrType->elementType.get());
        arraySize = arrType->size;
        sizeKnown = true;
    } else if (auto* genType = dynamic_cast<GenericType*>(iterType.get())) {
        if (genType->baseType && genType->baseType->getName() == "Array" && !genType->typeArguments.empty()) {
            elementType = cg.getLLVMType(genType->typeArguments[0].get());
            if (iterableExpr && iterableExpr->nodeType == ASTNodeType::ARRAY_LITERAL) {
                auto* arrayLiteral = static_cast<ArrayLiteralNode*>(iterableExpr.get());
                arraySize = static_cast<int>(arrayLiteral->elements.size());
                sizeKnown = true;
            }
        }
    }

    // Accept both array values and pointers-to-array by decaying to element pointer.
    if (auto* llvmArrayTy = llvm::dyn_cast<llvm::ArrayType>(arrayPtr->getType())) {
        if (!elementType) {
            elementType = llvmArrayTy->getElementType();
        }
        if (!sizeKnown) {
            arraySize = static_cast<int>(llvmArrayTy->getNumElements());
            sizeKnown = true;
        }
        llvm::Value* tmpArray = cg.builder.CreateAlloca(llvmArrayTy, nullptr, "map_arr_tmp");
        cg.builder.CreateStore(arrayPtr, tmpArray);
        llvm::Value* zero = cg.builder.getInt32(0);
        arrayPtr = cg.builder.CreateInBoundsGEP(llvmArrayTy, tmpArray, { zero, zero }, "map_arr_decay");
    }
    else if (arrayPtr->getType()->isPointerTy() && elementType && sizeKnown) {
        auto* llvmArrayTy = llvm::ArrayType::get(elementType, arraySize);
        llvm::Value* zero = cg.builder.getInt32(0);
        arrayPtr = cg.builder.CreateInBoundsGEP(llvmArrayTy, arrayPtr, { zero, zero }, "map_arr_decay");
    }

    if (!elementType || !sizeKnown || arraySize < 0) {
        std::cerr << "Error: Cannot determine array element type or size for map" << std::endl;
        return nullptr;
    }

    // Allocate loop counter and element variable before the loop
    llvm::Value* counterPtr = cg.createEntryAlloca(cg.builder.getInt32Ty(), "map_i");
    cg.builder.CreateStore(cg.builder.getInt32(0), counterPtr);
    llvm::Value* varPtr = cg.createEntryAlloca(elementType, variable + "_ptr");

    // Bind loop variable in scope (once, outside loop)
    cg.symbolTable.enterScope();
    std::unique_ptr<Type> varType;
    varType = mapElementSourceType(iterType);
    cg.symbolTable.addSymbol(variable, std::make_unique<Symbol>(
        variable, std::move(varType), varPtr, false, SymbolType::VARIABLE));

    // Determine result element type after loop variable binding (supports T -> U map)
    auto bodyType = body->getType();
    llvm::Type* resultElementType = bodyType ? cg.getLLVMType(bodyType.get()) : nullptr;
    if (!resultElementType) {
        std::cerr << "Type error: Cannot determine map result element type" << std::endl;
        cg.symbolTable.exitScope();
        return nullptr;
    }

    llvm::Value* resultPtr = cg.builder.CreateAlloca(resultElementType, cg.builder.getInt32(arraySize), "mapresult");

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(cg.context, "mapcond", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(cg.context, "mapbody", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(cg.context, "aftermap", function);

    cg.builder.CreateBr(condBB);

    // Condition
    cg.builder.SetInsertPoint(condBB);
    llvm::Value* counter = cg.builder.CreateLoad(cg.builder.getInt32Ty(), counterPtr, "i");
    llvm::Value* cond = cg.builder.CreateICmpSLT(counter, cg.builder.getInt32(arraySize), "mapcmp");
    cg.builder.CreateCondBr(cond, bodyBB, afterBB);

    // Body
    cg.builder.SetInsertPoint(bodyBB);
    llvm::Value* idx = cg.builder.CreateLoad(cg.builder.getInt32Ty(), counterPtr, "idx");

    // Load current element and store into loop variable
    llvm::Value* elemPtr = cg.builder.CreateGEP(elementType, arrayPtr, idx, "elemptr");
    llvm::Value* elemValue = cg.builder.CreateLoad(elementType, elemPtr, "elem");
    cg.builder.CreateStore(elemValue, varPtr);

    // Evaluate body
    llvm::Value* bodyResult = body->codegen();
    if (!bodyResult) {
        std::cerr << "Error: map body evaluation failed" << std::endl;
        cg.symbolTable.exitScope();
        return nullptr;
    }

    // Store result
    if (bodyResult->getType()->isPointerTy()) {
        bodyResult = cg.builder.CreateLoad(resultElementType, bodyResult, "map_body_load");
    }
    if (bodyResult->getType() != resultElementType) {
        std::cerr << "Type error: map body result type mismatch" << std::endl;
        cg.symbolTable.exitScope();
        return nullptr;
    }
    llvm::Value* resultElemPtr = cg.builder.CreateGEP(resultElementType, resultPtr, idx, "resultptr");
    cg.builder.CreateStore(bodyResult, resultElemPtr);

    // Increment counter
    llvm::Value* nextIdx = cg.builder.CreateAdd(idx, cg.builder.getInt32(1), "next_i");
    cg.builder.CreateStore(nextIdx, counterPtr);
    cg.builder.CreateBr(condBB);

    // After
    cg.builder.SetInsertPoint(afterBB);
    cg.symbolTable.exitScope();

    // Record the result length so a downstream collection op can iterate the right count even
    // though the bare pointer carries none. `map` preserves the source length.
    cg.arrayRuntimeLength[resultPtr] = cg.builder.getInt32(arraySize);

    return resultPtr;
}
