#pragma once
#include "ast.h"

class ParameterNode : public ASTNode {
public:
    ParameterNode() {
        nodeType = ASTNodeType::PARAMETER;
    }
    ParameterNode(const std::string& name, std::unique_ptr<Type> type)
        : name(name) {
        nodeType = ASTNodeType::PARAMETER;
        this->type = std::move(type);
    }
    ParameterNode(const ParameterNode& other) {
        nodeType = ASTNodeType::PARAMETER;
        name = other.name;
        type = other.type ? other.type->clone() : nullptr;
        defaultValue = other.defaultValue ? other.defaultValue->clone() : nullptr;
        line = other.line;
    }
    ParameterNode& operator=(const ParameterNode& other) {
        if (this != &other) {
            nodeType = ASTNodeType::PARAMETER;
            name = other.name;
            type = other.type ? other.type->clone() : nullptr;
            defaultValue = other.defaultValue ? other.defaultValue->clone() : nullptr;
            line = other.line;
        }
        return *this;
    }
    ~ParameterNode() = default;

    std::string name;
    std::unique_ptr<Type> type;
    // `name: T = expr`: used for a call that omits this argument. The expression is copied to and
    // evaluated at each such call site (see default_arguments.cpp).
    std::unique_ptr<ASTNode> defaultValue;

    void accept(ASTVisitor& visitor) override;

    llvm::Value* codegen() override;

    std::unique_ptr<Type> getType() override {
        return type->clone();
    }

    std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<ParameterNode>(*this);
    }
};
