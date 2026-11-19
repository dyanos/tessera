#include "codegen.h"
#include "FilterStatementNode.h"
#include "ASTVisitor.h"
#include "ArrayLiteralNode.h"

#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>

namespace {
std::unique_ptr<Type> filterElementSourceType(const std::unique_ptr<Type>& type) {
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

void FilterStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* FilterStatementNode::codegen() {
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
        llvm::Value* tmpArray = cg.builder.CreateAlloca(llvmArrayTy, nullptr, "filter_arr_tmp");
        cg.builder.CreateStore(arrayPtr, tmpArray);
        llvm::Value* zero = cg.builder.getInt32(0);
        arrayPtr = cg.builder.CreateInBoundsGEP(llvmArrayTy, tmpArray, { zero, zero }, "filter_arr_decay");
    }
    else if (arrayPtr->getType()->isPointerTy() && elementType && sizeKnown) {
        auto* llvmArrayTy = llvm::ArrayType::get(elementType, arraySize);
        llvm::Value* zero = cg.builder.getInt32(0);
        arrayPtr = cg.builder.CreateInBoundsGEP(llvmArrayTy, arrayPtr, { zero, zero }, "filter_arr_decay");
    }

    if (!elementType || !sizeKnown || arraySize < 0) {
        std::cerr << "Error: Cannot determine array element type or size for filter" << std::endl;
        return nullptr;
    }

    // Allocate all variables before the loop
    llvm::Value* resultPtr = cg.builder.CreateAlloca(elementType, cg.builder.getInt32(arraySize), "filterresult");
    llvm::Value* nullElem = llvm::Constant::getNullValue(elementType);
    for (int i = 0; i < arraySize; ++i) {
        llvm::Value* initIdx = cg.builder.getInt32(i);
        llvm::Value* initElemPtr = cg.builder.CreateGEP(elementType, resultPtr, initIdx, "filter_init_ptr");
        cg.builder.CreateStore(nullElem, initElemPtr);
    }
    llvm::Value* countPtr = cg.createEntryAlloca(cg.builder.getInt32Ty(), "filter_count");
    cg.builder.CreateStore(cg.builder.getInt32(0), countPtr);
    llvm::Value* counterPtr = cg.createEntryAlloca(cg.builder.getInt32Ty(), "filter_i");
    cg.builder.CreateStore(cg.builder.getInt32(0), counterPtr);
    llvm::Value* varPtr = cg.createEntryAlloca(elementType, variable + "_ptr");

    // Bind loop variable in scope (once, outside loop)
    cg.symbolTable.enterScope();
    std::unique_ptr<Type> varType;
    varType = filterElementSourceType(iterType);
    cg.symbolTable.addSymbol(variable, std::make_unique<Symbol>(
        variable, std::move(varType), varPtr, false, SymbolType::VARIABLE));

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(cg.context, "filtercond", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(cg.context, "filterbody", function);
    llvm::BasicBlock* storeBB = llvm::BasicBlock::Create(cg.context, "filterstore", function);
    llvm::BasicBlock* nextBB = llvm::BasicBlock::Create(cg.context, "filternext", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(cg.context, "afterfilter", function);

    cg.builder.CreateBr(condBB);

    // Condition
    cg.builder.SetInsertPoint(condBB);
    llvm::Value* counter = cg.builder.CreateLoad(cg.builder.getInt32Ty(), counterPtr, "i");
    llvm::Value* cond = cg.builder.CreateICmpSLT(counter, cg.builder.getInt32(arraySize), "filtercmp");
    cg.builder.CreateCondBr(cond, bodyBB, afterBB);

    // Body: evaluate predicate
    cg.builder.SetInsertPoint(bodyBB);
    llvm::Value* idx = cg.builder.CreateLoad(cg.builder.getInt32Ty(), counterPtr, "idx");

    // Load current element and store into loop variable
    llvm::Value* elemPtr = cg.builder.CreateGEP(elementType, arrayPtr, idx, "elemptr");
    llvm::Value* elemValue = cg.builder.CreateLoad(elementType, elemPtr, "elem");
    cg.builder.CreateStore(elemValue, varPtr);

    // Evaluate predicate
    llvm::Value* predResult = body->codegen();
    if (!predResult) {
        std::cerr << "Error: filter predicate evaluation failed" << std::endl;
        cg.symbolTable.exitScope();
        return nullptr;
    }

    // Convert to i1 if needed
    if (!predResult->getType()->isIntegerTy(1)) {
        if (predResult->getType()->isIntegerTy()) {
            predResult = cg.builder.CreateICmpNE(
                predResult, llvm::ConstantInt::get(predResult->getType(), 0), "pred");
        }
        else if (predResult->getType()->isFloatingPointTy()) {
            predResult = cg.builder.CreateFCmpONE(
                predResult, llvm::ConstantFP::get(predResult->getType(), 0.0), "pred");
        }
        else {
            std::cerr << "Type error: filter predicate must be numeric or boolean" << std::endl;
            cg.symbolTable.exitScope();
            return nullptr;
        }
    }

    cg.builder.CreateCondBr(predResult, storeBB, nextBB);

    // Store: copy element to result array
    cg.builder.SetInsertPoint(storeBB);
    llvm::Value* count = cg.builder.CreateLoad(cg.builder.getInt32Ty(), countPtr, "count");
    llvm::Value* resultElemPtr = cg.builder.CreateGEP(elementType, resultPtr, count, "resultptr");
    llvm::Value* elemReload = cg.builder.CreateLoad(elementType, varPtr, "elem_reload");
    cg.builder.CreateStore(elemReload, resultElemPtr);
    llvm::Value* nextCount = cg.builder.CreateAdd(count, cg.builder.getInt32(1), "next_count");
    cg.builder.CreateStore(nextCount, countPtr);
    cg.builder.CreateBr(nextBB);

    // Next: increment counter
    cg.builder.SetInsertPoint(nextBB);
    llvm::Value* idxReload = cg.builder.CreateLoad(cg.builder.getInt32Ty(), counterPtr, "idx_reload");
    llvm::Value* nextIdx = cg.builder.CreateAdd(idxReload, cg.builder.getInt32(1), "next_i");
    cg.builder.CreateStore(nextIdx, counterPtr);
    cg.builder.CreateBr(condBB);

    // After
    cg.builder.SetInsertPoint(afterBB);
    cg.symbolTable.exitScope();

    // The result buffer is over-allocated to the source size but only the first `count` elements
    // survived the predicate. Record that runtime count so a downstream collection op iterates the
    // surviving elements rather than the padded source length.
    llvm::Value* survivingCount = cg.builder.CreateLoad(cg.builder.getInt32Ty(), countPtr, "filter_len");
    cg.arrayRuntimeLength[resultPtr] = survivingCount;

    return resultPtr;
}
