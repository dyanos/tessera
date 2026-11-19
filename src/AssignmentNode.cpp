#include "codegen.h"
#include "AssignmentNode.h"
#include "ASTVisitor.h"
#include "type_utils.h"

#include <iostream>


void AssignmentNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* AssignmentNode::codegen() {
    // C/C++의 '=' 연산자는 오른쪽 값을 왼쪽에 대입하는 연산자이다.
    // 왼쪽 값은 변수이어야 한다.
    // 오른쪽 값은 변수이거나 메서드 호출 결과이어야 한다.
    auto symbolOpt = CodeGenerator::getInstance().symbolTable.lookup(name);
    if (!symbolOpt) {
        std::cerr << "Undefined variable: " << name << std::endl;
        return nullptr;
    }

    auto* symbol = symbolOpt;
    if (!symbol->isMutable) {
        std::cerr << "Cannot assign to immutable variable: " << name << std::endl;
        return nullptr;
    }

    llvm::Value* exprValue = expression->codegen();
    if (!exprValue) {
        return nullptr;
    }

    if (isStructClassType(symbol->type.get())) {
        llvm::Type* targetType = CodeGenerator::getInstance().getLLVMType(symbol->type.get());
        exprValue = coerceValueToLLVMType(exprValue, targetType);
        if (!exprValue || exprValue->getType() != targetType) {
            std::cerr << "Type error: Assignment value does not match struct variable '" << name << "'" << std::endl;
            return nullptr;
        }
    }

    auto& cg = CodeGenerator::getInstance();
    if (symbol->type && symbol->type->getName() == "String" && llvm::isa<llvm::AllocaInst>(symbol->value) &&
        exprValue->getType()->isPointerTy()) {
        exprValue = cg.takeOwnedString(exprValue);
        llvm::Value* previous = cg.builder.CreateLoad(exprValue->getType(), symbol->value, name + ".previous");
        cg.builder.CreateStore(exprValue, symbol->value);
        cg.releaseString(previous);
        return exprValue;
    }
    CodeGenerator::getInstance().builder.CreateStore(exprValue, symbol->value);
    CodeGenerator::clearFreshTensor(exprValue);

    return exprValue;
}
