#include "codegen.h"

#include "WhileStatementNode.h"
#include "ASTVisitor.h"

#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>

void WhileStatementNode::accept(ASTVisitor& visitor) {
	visitor.visit(*this);
}

llvm::Value* WhileStatementNode::codegen() {
	auto& cg = CodeGenerator::getInstance();
	llvm::Function* function = cg.builder.GetInsertBlock()->getParent();

	llvm::BasicBlock* condBB = llvm::BasicBlock::Create(cg.context, "whilecond", function);
	llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(cg.context, "whilebody", function);
	llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(cg.context, "afterwhile", function);

	cg.builder.CreateBr(condBB);

	// Condition block
	cg.builder.SetInsertPoint(condBB);
	llvm::Value* condValue = this->condition->codegen();
	if (!condValue) return nullptr;

	// Convert to i1 if not already boolean
	if (!condValue->getType()->isIntegerTy(1)) {
		if (condValue->getType()->isIntegerTy()) {
			condValue = cg.builder.CreateICmpNE(
				condValue,
				llvm::ConstantInt::get(condValue->getType(), 0),
				"whilecond");
		}
		else if (condValue->getType()->isFloatingPointTy()) {
			condValue = cg.builder.CreateFCmpONE(
				condValue,
				llvm::ConstantFP::get(condValue->getType(), 0.0),
				"whilecond");
		}
		else {
			std::cerr << "Type error: while condition must be numeric or boolean" << std::endl;
			return nullptr;
		}
	}

	cg.builder.CreateCondBr(condValue, loopBB, afterBB);

	// Loop body
	cg.builder.SetInsertPoint(loopBB);
	cg.loopTargets.push_back({afterBB, condBB, cg.cleanupDepth()});
	this->body->codegen();
	cg.loopTargets.pop_back();

	// Branch back to condition if no terminator
	if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
		cg.builder.CreateBr(condBB);
	}

	// After loop
	cg.builder.SetInsertPoint(afterBB);

	return nullptr;
}

std::unique_ptr<Type> WhileStatementNode::getType() {
	return std::make_unique<UnknownType>();
}
