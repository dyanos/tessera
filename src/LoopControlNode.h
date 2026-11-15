#pragma once
#include "ast.h"

// `break` / `continue` inside a `while` or `for` loop.
class LoopControlNode : public ASTNode {
public:
    bool isBreak = true;

    explicit LoopControlNode(bool isBreak = true) : isBreak(isBreak) {
        nodeType = ASTNodeType::LOOP_CONTROL;
    }
    LoopControlNode(const LoopControlNode& other) : isBreak(other.isBreak) {
        nodeType = ASTNodeType::LOOP_CONTROL;
        line = other.line;
    }

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;
    std::unique_ptr<Type> getType() override { return std::make_unique<BasicType>("Unit"); }
    std::unique_ptr<ASTNode> clone() override { return std::make_unique<LoopControlNode>(*this); }
};
