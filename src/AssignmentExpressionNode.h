#pragma once
#include "ast.h"


class AssignmentExpressionNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::string op = "=";
    bool rebindsStackTensor = false;
    // `f(name = value)`: a named call argument, resolved against the callee's parameter names before
    // type checking (default_arguments.cpp). Never evaluated as an assignment.
    bool isNamedArgument = false;

    AssignmentExpressionNode() {
        nodeType = ASTNodeType::ASSIGNMENT_EXPRESSION;
    }
    AssignmentExpressionNode(std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right, const std::string& op = "=") {
        this->left = std::move(left);
        this->right = std::move(right);
        this->op = op;
        nodeType = ASTNodeType::ASSIGNMENT_EXPRESSION;
    }
    AssignmentExpressionNode(const AssignmentExpressionNode& other)
        : left(other.left ? other.left->clone() : nullptr),
          right(other.right ? other.right->clone() : nullptr),
          op(other.op), rebindsStackTensor(other.rebindsStackTensor), isNamedArgument(other.isNamedArgument) {
        line = other.line;
        nodeType = ASTNodeType::ASSIGNMENT_EXPRESSION;
	}
    AssignmentExpressionNode& operator=(const AssignmentExpressionNode& other) {
        if (this != &other) {
            left = other.left ? other.left->clone() : nullptr;
            right = other.right ? other.right->clone() : nullptr;
            op = other.op;
            rebindsStackTensor = other.rebindsStackTensor;
            isNamedArgument = other.isNamedArgument;
            nodeType = ASTNodeType::ASSIGNMENT_EXPRESSION;
        }
        return *this;
    }

	void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;
    std::unique_ptr<Type> getType() override;
	std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<AssignmentExpressionNode>(*this);
	}
};
