#include "codegen.h"

#include "LoopControlNode.h"
#include "ASTVisitor.h"

#include <iostream>
#include <sstream>
#include <llvm/IR/Function.h>

void LoopControlNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* LoopControlNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    if (cg.loopTargets.empty()) {
        { std::ostringstream os_; os_ << "'" << (isBreak ? "break" : "continue") << "' outside a loop"; CodeGenerator::getInstance().reportError(os_.str()); }
        return nullptr;
    }
    auto* block = cg.builder.GetInsertBlock();
    if (!block || csec_llvm::terminatorOrNull(block)) return nullptr;
    const auto& target = cg.loopTargets.back();
    // Leaving the loop body (or the rest of this iteration) drops the owned locals of every scope
    // opened since the loop started, exactly as a `return` drops all of them.
    cg.emitCleanupsAbove(target.cleanupDepth);
    cg.builder.CreateBr(isBreak ? target.breakBB : target.continueBB);
    // Statements after break/continue in the same block are unreachable; give them a block of their
    // own so the IR stays well formed.
    cg.builder.SetInsertPoint(llvm::BasicBlock::Create(cg.context, isBreak ? "after.break" : "after.continue",
                                                       block->getParent()));
    return nullptr;
}
