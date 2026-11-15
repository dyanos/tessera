// ast_walk.h -- generic traversal of the AST's child nodes.
//
// The AST has no uniform child list: every node class keeps its children in its own named members.
// forEachChild() enumerates them in source order so that whole-program passes (default/named argument
// expansion, enum desugaring, template-type normalisation, ...) do not each need their own copy of a
// per-node switch. Keep it in sync when a node class gains a child member.
#pragma once

#include "all_ast.h"

#include <memory>

namespace ast_walk {

// Calls f(child, slot) for each direct child of `node`. `slot` points at the owning
// std::unique_ptr<ASTNode> so the callback may replace the child; it is null when the child is held
// through a typed pointer (std::unique_ptr<BlockNode>) and can only be inspected or edited in place.
template <class F>
void forEachChild(ASTNode* node, F&& f) {
    if (!node) return;
    auto one = [&](std::unique_ptr<ASTNode>& slot) { if (slot) f(slot.get(), &slot); };
    auto many = [&](std::vector<std::unique_ptr<ASTNode>>& slots) { for (auto& s : slots) one(s); };
    auto block = [&](std::unique_ptr<BlockNode>& b) { if (b) f(static_cast<ASTNode*>(b.get()), nullptr); };

    if (node->contracts) {
        for (auto& c : node->contracts->requiresClauses) one(c.expression);
        for (auto& c : node->contracts->ensuresClauses) one(c.expression);
        for (auto& c : node->contracts->invariants) one(c.expression);
        for (auto& c : node->contracts->decreases) one(c.expression);
    }

    if (auto* n = dynamic_cast<ProgramNode*>(node)) { many(n->statements); return; }
    if (auto* n = dynamic_cast<BlockNode*>(node)) { many(n->statements); return; }
    if (auto* n = dynamic_cast<AttributeNode*>(node)) { one(n->expr); one(n->target); return; }
    if (auto* n = dynamic_cast<ClassDeclarationNode*>(node)) { many(n->constructorParams); one(n->body); return; }
    if (auto* n = dynamic_cast<ClassBodyNode*>(node)) { many(n->fields); many(n->methods); return; }
    if (auto* n = dynamic_cast<ObjectDeclarationNode*>(node)) { one(n->body); return; }
    if (auto* n = dynamic_cast<FunctionDeclarationNode*>(node)) {
        many(n->parameters);
        for (auto& p : n->parameters) {
            if (auto* param = dynamic_cast<ParameterNode*>(p.get())) one(param->defaultValue);
        }
        block(n->body);
        return;
    }
    if (auto* n = dynamic_cast<TemplateDeclarationNode*>(node)) { one(n->declaration); return; }
    if (auto* n = dynamic_cast<VariableDeclarationNode*>(node)) { one(n->initializer); return; }
    if (auto* n = dynamic_cast<CallExpressionNode*>(node)) { one(n->callee); many(n->arguments); return; }
    if (auto* n = dynamic_cast<MethodCallNode*>(node)) { one(n->object); many(n->arguments); return; }
    if (auto* n = dynamic_cast<FunctionCallNode*>(node)) { many(n->arguments); return; }
    if (auto* n = dynamic_cast<IfStatementNode*>(node)) { one(n->condition); block(n->thenBlock); block(n->elseBlock); return; }
    if (auto* n = dynamic_cast<ForStatementNode*>(node)) { one(n->iterableExpr); one(n->tokenStream); block(n->body); return; }
    if (auto* n = dynamic_cast<WhileStatementNode*>(node)) { one(n->condition); block(n->body); return; }
    if (auto* n = dynamic_cast<MapStatementNode*>(node)) { one(n->iterableExpr); one(n->body); return; }
    if (auto* n = dynamic_cast<PMapStatementNode*>(node)) { one(n->iterableExpr); one(n->body); return; }
    if (auto* n = dynamic_cast<ReduceStatementNode*>(node)) { one(n->iterableExpr); one(n->initialValue); one(n->body); return; }
    if (auto* n = dynamic_cast<FilterStatementNode*>(node)) { one(n->iterableExpr); one(n->body); return; }
    if (auto* n = dynamic_cast<LambdaExpressionNode*>(node)) { many(n->arguments); one(n->body); return; }
    if (auto* n = dynamic_cast<MatchExpressionNode*>(node)) {
        one(n->expression);
        for (auto& c : n->cases) { one(c.first); one(c.second); }
        return;
    }
    if (auto* n = dynamic_cast<RangeExpressionNode*>(node)) { one(n->startExpr); one(n->endExpr); return; }
    if (auto* n = dynamic_cast<BinaryExpressionNode*>(node)) { one(n->left); one(n->right); return; }
    if (auto* n = dynamic_cast<AssignmentNode*>(node)) { one(n->expression); return; }
    if (auto* n = dynamic_cast<ReturnStatementNode*>(node)) { one(n->expression); return; }
    if (auto* n = dynamic_cast<ClassInstanceCreationNode*>(node)) { many(n->arguments); return; }
    if (auto* n = dynamic_cast<ArrayCreationExpressionNode*>(node)) { many(n->sizes); return; }
    if (auto* n = dynamic_cast<AssignmentExpressionNode*>(node)) { one(n->left); one(n->right); return; }
    if (auto* n = dynamic_cast<UnaryExpressionNode*>(node)) { one(n->expression); return; }
    if (auto* n = dynamic_cast<CastingExpressionNode*>(node)) { one(n->expression); one(n->typeNode); return; }
    if (auto* n = dynamic_cast<PostfixExpressionNode*>(node)) { one(n->expression); return; }
    if (auto* n = dynamic_cast<PrefixExpressionNode*>(node)) { one(n->expression); return; }
    if (auto* n = dynamic_cast<ArrayLiteralNode*>(node)) { many(n->elements); return; }
    if (auto* n = dynamic_cast<DictLiteralNode*>(node)) { many(n->keys); many(n->values); return; }
    if (auto* n = dynamic_cast<TupleExpressionNode*>(node)) { many(n->elements); return; }
    if (auto* n = dynamic_cast<DestructuringAssignmentNode*>(node)) { one(n->value); return; }
    if (auto* n = dynamic_cast<ArrayCreationNode*>(node)) { many(n->elements); return; }
    if (auto* n = dynamic_cast<ArrayAccessNode*>(node)) {
        one(n->array);
        one(n->index);
        for (auto& spec : n->indices) {
            if (!spec) continue;
            one(spec->index); one(spec->start); one(spec->end); one(spec->step);
        }
        return;
    }
    if (auto* n = dynamic_cast<AccessFieldNode*>(node)) { one(n->base); one(n->field); return; }
    // IdentifierNode, ValueNode, UnitNode, ImportNode, ParameterNode, ExpressionNode: leaves.
}

// Pre-order walk of the whole subtree. f(node, slot) as in forEachChild; the root gets slot `rootSlot`.
// When f returns false the node's children are skipped.
template <class F>
void walk(ASTNode* node, std::unique_ptr<ASTNode>* rootSlot, F&& f) {
    if (!node) return;
    if (!f(node, rootSlot)) return;
    // The callback may have replaced the node through its slot; continue with the replacement.
    ASTNode* current = rootSlot && rootSlot->get() ? rootSlot->get() : node;
    forEachChild(current, [&](ASTNode* child, std::unique_ptr<ASTNode>* slot) { walk(child, slot, f); });
}

}  // namespace ast_walk
