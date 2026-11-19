#include "codegen.h"

#include "ReturnStatementNode.h"
#include "ASTVisitor.h"
#include "type_utils.h"

#include <iostream>


void ReturnStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* ReturnStatementNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    llvm::Value* returnValue = nullptr;
    if (expression) {
        returnValue = expression->codegen();
    }

    llvm::Function* currentFunction = cg.builder.GetInsertBlock()->getParent();
    llvm::Type* returnType = currentFunction->getReturnType();

    if (returnType->isVoidTy()) {
        cg.emitAllCleanups();
        cg.builder.CreateRetVoid();
    }
    else {
        if (!returnValue) {
            std::cerr << "Error: Return statement with no value in function returning non-void type" << std::endl;
            return nullptr;
        }

        // Returning an integer where the function's return slot is a pointer means a Nat return
        // (`def f(): Nat = 5`): promote it to a handle. No other pointer-return type accepts an integer.
        if (returnValue && returnValue->getType()->isIntegerTy() && returnType->isPointerTy()) {
            auto natType = std::make_unique<ClassType>("Nat");
            returnValue = coerceToNat(returnValue, natType.get(), expression.get());
        }

        returnValue = coerceValueToLLVMType(returnValue, returnType);
        if (!returnValue || returnValue->getType() != returnType) {
            std::cerr << "Error: Return value type does not match function return type" << std::endl;
            return nullptr;
        }

        if (returnType->isPointerTy()) returnValue = cg.copyStackBufferToHeap(returnValue);
        cg.emitAllCleanupsExcept(returnValue);
        cg.builder.CreateRet(returnValue);
    }

    return nullptr;
}
