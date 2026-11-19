#include "codegen.h"
#include "ArrayLiteralNode.h"
#include "ASTVisitor.h"
#include "type_utils.h"
#include "ast_walk.h"

#include <map>
#include <set>

#include <iostream>
#include <llvm/IR/Constants.h>

void ArrayLiteralNode::accept(ASTVisitor& visitor) {
	visitor.visit(*this);
}

llvm::Value* ArrayLiteralNode::codegen() {
	if (elements.empty()) return nullptr;

	auto& cg = CodeGenerator::getInstance();

	// Get element type from first element
	llvm::Value* firstElem = elements[0]->codegen();
	if (!firstElem) return nullptr;

	llvm::Type* elementType = firstElem->getType();
	// Store each element in its type's storage form: a struct value (an enum, a `struct`) is copied into
	// the array, not referenced through the temporary that built it.
	if (auto declared = getType()) {
		if (auto* arrayType = dynamic_cast<ArrayType*>(declared.get())) {
			if (llvm::Type* storage = arrayType->elementType ? getABIStorageType(arrayType->elementType.get()) : nullptr) {
				if (storage->isStructTy()) elementType = storage;
			}
		}
	}
	firstElem = coerceValueToLLVMType(firstElem, elementType);

	// Allocate array on stack
	llvm::Value* arraySize = cg.builder.getInt32(elements.size());
	llvm::Value* arrayPtr = hoistable ? cg.createEntryAlloca(elementType, "arrlit", arraySize)
	                                  : cg.builder.CreateAlloca(elementType, arraySize, "arrlit");

	// Store first element
	llvm::Value* idx0 = llvm::ConstantInt::get(cg.builder.getInt32Ty(), 0);
	llvm::Value* elemPtr0 = cg.builder.CreateGEP(elementType, arrayPtr, idx0);
	{ auto firstType = elements[0]->getType(); firstElem = cg.takeOwned(firstElem, firstType.get()); }
	cg.builder.CreateStore(firstElem, elemPtr0);

	// Store remaining elements
	for (size_t i = 1; i < elements.size(); ++i) {
		llvm::Value* elemValue = elements[i]->codegen();
		if (!elemValue) return nullptr;
		if (elementType->isStructTy()) elemValue = coerceValueToLLVMType(elemValue, elementType);
		if (elemValue->getType() != elementType) {
			std::cerr << "Type error: Array literal elements must have the same type" << std::endl;
			return nullptr;
		}
		llvm::Value* index = llvm::ConstantInt::get(cg.builder.getInt32Ty(), i);
		llvm::Value* elemPtr = cg.builder.CreateGEP(elementType, arrayPtr, index);
		{ auto elemType = elements[i]->getType(); elemValue = cg.takeOwned(elemValue, elemType.get()); }
		cg.builder.CreateStore(elemValue, elemPtr);
	}

	// The element count, for loops over the literal once it is bound to a name.
	cg.arrayRuntimeLength[arrayPtr] = cg.builder.getInt32(static_cast<uint32_t>(elements.size()));
	return arrayPtr;
}

std::unique_ptr<Type> ArrayLiteralNode::getType() {
	if (type) return type->clone();
	if (elements.empty()) return std::make_unique<UnknownType>();

	auto firstType = elements[0]->getType();
	if (!firstType) return std::make_unique<UnknownType>();
	for (size_t i = 1; i < elements.size(); ++i) {
		auto elemType = elements[i]->getType();
		if (!elemType || !firstType->equals(elemType)) {
			return std::make_unique<UnknownType>();
		}
	}
	return std::make_unique<ArrayType>(firstType, static_cast<int>(elements.size()));
}

namespace {

bool borrowsOrReads(ASTNode* parent, ASTNode* child) {
    if (auto* access = dynamic_cast<ArrayAccessNode*>(parent)) return access->array.get() == child;
    if (auto* call = dynamic_cast<MethodCallNode*>(parent)) return call->object.get() == child;
    if (auto* field = dynamic_cast<AccessFieldNode*>(parent)) return field->base.get() == child;
    if (auto* prefix = dynamic_cast<PrefixExpressionNode*>(parent)) return prefix->op == "&" || prefix->op == "&mut";
    if (auto* loop = dynamic_cast<ForStatementNode*>(parent)) return loop->iterableExpr.get() == child;
    return false;
}

}

std::set<std::string> ArrayLiteralNode::movedNames(ASTNode* functionBody) {
    std::set<std::string> names;
    auto visit = [&](auto& self, ASTNode* node) -> void {
        ASTNode* moved = nullptr;
        if (auto* prefix = dynamic_cast<PrefixExpressionNode*>(node); prefix && prefix->op == "<-") moved = prefix->expression.get();
        if (auto* assignment = dynamic_cast<AssignmentExpressionNode*>(node); assignment && assignment->op == "<-") moved = assignment->right.get();
        if (auto* identifier = dynamic_cast<IdentifierNode*>(moved)) names.insert(identifier->value);
        ast_walk::forEachChild(node, [&](ASTNode* child, std::unique_ptr<ASTNode>*) { self(self, child); });
    };
    if (functionBody) visit(visit, functionBody);
    return names;
}

void ArrayLiteralNode::markHoistable(ASTNode* functionBody) {
    if (!functionBody) return;
    std::map<std::string, std::vector<ArrayLiteralNode*>> bound;
    std::set<std::string> escaping;
    auto visit = [&](auto& self, ASTNode* node, ASTNode* parent, int lambdaDepth) -> void {
        if (auto* literal = dynamic_cast<ArrayLiteralNode*>(node)) {
            auto* declaration = dynamic_cast<VariableDeclarationNode*>(parent);
            if (declaration && declaration->initializer.get() == node) bound[declaration->name].push_back(literal);
            else if (parent && lambdaDepth == 0 && borrowsOrReads(parent, node)) literal->hoistable = true;
        }
        if (auto* identifier = dynamic_cast<IdentifierNode*>(node)) {
            if (lambdaDepth > 0 || !parent || !borrowsOrReads(parent, node)) {
                escaping.insert(identifier->value);
            }
        }
        const int depth = lambdaDepth + (dynamic_cast<LambdaExpressionNode*>(node) ? 1 : 0);
        ast_walk::forEachChild(node, [&](ASTNode* child, std::unique_ptr<ASTNode>*) { self(self, child, node, depth); });
    };
    visit(visit, functionBody, nullptr, 0);
    for (auto& [name, literals] : bound) {
        if (escaping.count(name)) continue;
        for (ArrayLiteralNode* literal : literals) literal->hoistable = true;
    }
}
