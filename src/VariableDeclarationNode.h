#pragma once
#include "ast.h"

class VariableDeclarationNode : public ASTNode {
public:
    VariableDeclarationNode() {
        this->nodeType = ASTNodeType::VARIABLE_DECLARATION;
    }
    VariableDeclarationNode(const std::string& name,
                            std::unique_ptr<Type> type,
                            std::unique_ptr<ASTNode> initializer,
                            bool isMutable = false,
                            bool isField = false)
        : name(name),
          type(std::move(type)),
          initializer(std::move(initializer)),
          isMutable(isMutable),
          isField(isField) {
        this->nodeType = ASTNodeType::VARIABLE_DECLARATION;
    }
    VariableDeclarationNode(const VariableDeclarationNode& other) {
        this->nodeType = ASTNodeType::VARIABLE_DECLARATION;
        this->name = other.name;
        this->type = other.type ? other.type->clone() : nullptr;
        this->initializer = other.initializer ? other.initializer->clone() : nullptr;
        this->isMutable = other.isMutable;
        this->isField = other.isField;
        this->isConstexpr = other.isConstexpr;
        this->hasExplicitType = other.hasExplicitType;
    }
    VariableDeclarationNode& operator=(const VariableDeclarationNode& other) {
        if (this != &other) {
            this->nodeType = ASTNodeType::VARIABLE_DECLARATION;
            this->name = other.name;
            this->type = other.type ? other.type->clone() : nullptr;
            this->initializer = other.initializer ? other.initializer->clone() : nullptr;
            this->isMutable = other.isMutable;
            this->isField = other.isField;
            this->isConstexpr = other.isConstexpr;
            this->hasExplicitType = other.hasExplicitType;
        }
        return *this;
    }

    std::string name;
    std::unique_ptr<Type> type;
    std::unique_ptr<ASTNode> initializer;
    bool isMutable = false;
    bool isField = false;
    bool isConstexpr = false;
    bool hasExplicitType = false;
    // Free the instance at scope exit (set by BlockNode for a `new` local used only in ways that
    // keep it the sole owner of the allocation).
    bool autoDrop = false;

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;

    std::unique_ptr<Type> getType() override {
        if (type) {
            return type->clone();
        }
        return std::make_unique<UnknownType>();
    }
    std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<VariableDeclarationNode>(*this);
    }
};
