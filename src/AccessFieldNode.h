#pragma once
#include "ast.h"

#include <functional>
#include "symbol.h"

class AccessFieldNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> base;
    std::unique_ptr<ASTNode> field;

    AccessFieldNode() {
        nodeType = ASTNodeType::ACCESS_FIELD;
    }
    AccessFieldNode(std::unique_ptr<ASTNode> base, std::unique_ptr<ASTNode> field) {
        nodeType = ASTNodeType::ACCESS_FIELD;
		this->base = std::move(base);
		this->field = std::move(field);
    }
    AccessFieldNode(const AccessFieldNode& other) {
        nodeType = ASTNodeType::ACCESS_FIELD;
        base = other.base ? other.base->clone() : nullptr;
        field = other.field ? other.field->clone() : nullptr;
	}
    AccessFieldNode& operator=(const AccessFieldNode& other) {
        if (this != &other) {
            nodeType = ASTNodeType::ACCESS_FIELD;
            base = other.base ? other.base->clone() : nullptr;
            field = other.field ? other.field->clone() : nullptr;
        }
        return *this;
	}

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;

    // Address of the field (an lvalue), used as an assignment target. codegen() reads through it.
    llvm::Value* codegenFieldPointer();
    // Field access on a base that is not a named value: binds the base to a hidden local first.
    llvm::Value* codegenThroughTemporary(const std::function<llvm::Value*()>& access);
    std::unique_ptr<Type> getType() override;
    std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<AccessFieldNode>(*this);
    }

    int findFieldIndex(ClassSymbol* classSymbol, const std::string& fieldName);
};
