#include "codegen.h"
#include "debug_info.h"
#include "BlockNode.h"
#include "ASTVisitor.h"
#include "ast_walk.h"
#include <functional>
#include "enums.h"
#include "type_checker.h"
#include "type_utils.h"
#include "default_arguments.h"

namespace {
// Whether every use of `name` in `statements[from..]` leaves it the only owner of its object:
// a method call or field access on it, a borrow, a `<-` move or free() (both clear its drop flag),
// returning it, comparing it, or printing it. Anything else -- a capture, an array element, a value
// stored without `<-` -- could keep a second reference, so such a local is not dropped.
bool onlyOwningUses(const std::string& name, const std::string& className,
                    std::vector<std::unique_ptr<ASTNode>>& statements, size_t from) {
    bool safe = true;
    std::function<void(ASTNode*, ASTNode*)> visit = [&](ASTNode* node, ASTNode* parent) {
        if (!node || !safe) return;
        if (auto* id = dynamic_cast<IdentifierNode*>(node)) {
            if (id->value.rfind(name + ".", 0) == 0) return;  // `x.field`
            if (id->value != name) return;
            if (auto* call = dynamic_cast<MethodCallNode*>(parent)) {
                // A method that returns or stores `this` shares the object.
                if (call->object.get() == node && !methodMayKeepThis(className, call->methodName)) return;
            }
            if (auto* access = dynamic_cast<AccessFieldNode*>(parent)) { if (access->base.get() == node) return; }
            if (auto* prefix = dynamic_cast<PrefixExpressionNode*>(parent)) {
                if (prefix->op == "<-" || prefix->op == "&" || prefix->op == "&mut") return;
            }
            if (auto* unary = dynamic_cast<UnaryExpressionNode*>(parent)) {
                if (unary->op == "&" || unary->op == "&mut") return;
            }
            if (auto* call = dynamic_cast<FunctionCallNode*>(parent)) {
                if (call->functionName == "free" || call->functionName == "print" || call->functionName == "println") return;
            }
            if (dynamic_cast<ReturnStatementNode*>(parent)) return;
            if (auto* binary = dynamic_cast<BinaryExpressionNode*>(parent)) {
                if (binary->op == "==" || binary->op == "!=") return;
            }
            safe = false;
            return;
        }
        if (dynamic_cast<LambdaExpressionNode*>(node)) {
            // A capture of the name in a lambda shares the object.
            ast_walk::walk(node, nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
                if (auto* id = dynamic_cast<IdentifierNode*>(n)) {
                    if (id->value == name || id->value.rfind(name + ".", 0) == 0) safe = false;
                }
                return safe;
            });
            return;
        }
        ast_walk::forEachChild(node, [&](ASTNode* child, std::unique_ptr<ASTNode>*) { visit(child, node); });
    };
    for (size_t i = from; i < statements.size() && safe; ++i) visit(statements[i].get(), nullptr);
    // The block's value is its last statement: a bare `x` there hands the object out.
    if (safe && !statements.empty()) {
        if (auto* id = dynamic_cast<IdentifierNode*>(statements.back().get())) {
            if (id->value == name) safe = false;
        }
    }
    return safe;
}
}  // namespace


void BlockNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* BlockNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    cg.symbolTable.enterScope();
    cg.enterCleanupScope();

    llvm::Value* last = nullptr;
    for (size_t i = 0; i < statements.size(); ++i) {
        auto& stmt = statements[i];
        // Under strict ownership a local made by `new` owns its object: drop it at scope exit when
        // it is never shared.
        if (auto* decl = dynamic_cast<VariableDeclarationNode*>(stmt.get())) {
            if (isStrictOwnership() && dynamic_cast<ClassInstanceCreationNode*>(decl->initializer.get())) {
                auto declType = decl->getType();
                if (declType && declType->getKind() == Type::Kind::CLASS && !isStructClassType(declType.get()) &&
                    !enumOfType(declType.get())) {
                    std::string className = declType->getName();
                    std::string templateName;
                    if (templateInstanceArguments(className, &templateName, nullptr)) className = templateName;
                    decl->autoDrop = onlyOwningUses(decl->name, className, statements, i + 1);
                }
            }
        }
        if (stmt->line > 0) debuginfo::setLine(stmt->line);
        cg.enterTemporaryFrame();
        last = stmt->codegen();
        cg.exitTemporaryFrame();
    }

    auto* currentBlock = cg.builder.GetInsertBlock();
    if (currentBlock && !csec_llvm::terminatorOrNull(currentBlock)) {
        cg.emitCurrentScopeCleanups();
    }
    cg.exitCleanupScope();
    cg.symbolTable.exitScope();

    return last;
}
