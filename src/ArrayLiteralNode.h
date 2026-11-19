#pragma once

#include <vector>
#include <memory>
#include "ast.h"

class ArrayLiteralNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> elements;
    bool hoistable = false;

    ArrayLiteralNode() {
        elements.clear();
        nodeType = ASTNodeType::ARRAY_LITERAL;
    }
    ArrayLiteralNode(const ArrayLiteralNode& other) {
        for (const auto& elem : other.elements) {
            this->elements.push_back(elem ? elem->clone() : nullptr);
        }
        nodeType = ASTNodeType::ARRAY_LITERAL;
    }
    ArrayLiteralNode& operator=(const ArrayLiteralNode& other) {
        if (this != &other) {
            elements.clear();
            elements.reserve(other.elements.size());
            for (const auto& elem : other.elements) {
                elements.push_back(elem ? elem->clone() : nullptr);
            }
            nodeType = ASTNodeType::ARRAY_LITERAL;
        }
        return *this;
    }
    virtual ~ArrayLiteralNode() = default;

    static void markHoistable(ASTNode* functionBody);

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;
    std::unique_ptr<Type> getType() override;
    std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<ArrayLiteralNode>(*this);
    }
};
