#include "codegen.h"
#include "debug_info.h"
#include "BlockNode.h"
#include "ASTVisitor.h"
#include "ast_walk.h"
#include <functional>
#include <set>
#include "ArrayAccessNode.h"
#include "enums.h"
#include "type_checker.h"
#include "type_utils.h"
#include "default_arguments.h"
#include "all_ast.h"

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
                if (binary->left.get() == node && !methodMayKeepThis(className, "operator" + binary->op)) return;
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
bool isTensorBorrowingBuiltin(const std::string& functionName) {
    static const std::set<std::string> names = {
        "transpose", "conjugate", "adjoint", "relu", "softmax", "sum", "mean", "norm", "mse",
        "innerProduct", "outerProduct", "tensorProduct", "argmax", "argmin", "min", "max", "prod",
        "zerosLike", "onesLike", "fullLike", "print", "println", "free"};
    if (!names.count(functionName)) return false;
    auto& cg = CodeGenerator::getInstance();
    if (cg.module->getFunction(functionName)) return false;
    return cg.symbolTable.lookup(functionName) == nullptr;
}

bool isIntegerText(const std::string& text) {
    if (text.empty()) return false;
    size_t start = text[0] == '-' ? 1 : 0;
    if (start == text.size()) return false;
    for (size_t i = start; i < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
    }
    return true;
}

bool isTensorOrNumericType(const Type* type) {
    if (!type) return false;
    static const std::set<std::string> numeric = {
        "Int", "Long", "Short", "Byte", "Double", "Float", "Real", "Natural", "Integer"};
    return type->getName() == "Tensor" || numeric.count(type->getName()) != 0;
}

std::vector<VariableDeclarationNode*> laterDeclarations(const std::string& name,
                                                        std::vector<std::unique_ptr<ASTNode>>& statements, size_t from) {
    std::vector<VariableDeclarationNode*> found;
    for (size_t i = from; i < statements.size(); ++i) {
        ast_walk::walk(statements[i].get(), nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
            if (auto* decl = dynamic_cast<VariableDeclarationNode*>(n)) {
                if (decl->name == name) found.push_back(decl);
            }
            return true;
        });
    }
    return found;
}

bool identifiersResolve(ASTNode* node) {
    bool resolved = true;
    auto& cg = CodeGenerator::getInstance();
    ast_walk::walk(node, nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
        if (auto* id = dynamic_cast<IdentifierNode*>(n)) {
            const std::string base = id->value.substr(0, id->value.find('.'));
            if (base != "true" && base != "false" && !isIntegerText(base) && !cg.symbolTable.lookup(base)) {
                resolved = false;
            }
        }
        return resolved;
    });
    return resolved;
}

bool cannotDispatchClassOperator(ASTNode* left, std::vector<std::unique_ptr<ASTNode>>& statements, size_t from) {
    if (!left) return false;
    if (auto* id = dynamic_cast<IdentifierNode*>(left)) {
        if (id->value.find('.') == std::string::npos) {
            Symbol* symbol = CodeGenerator::getInstance().symbolTable.lookup(id->value);
            auto declarations = laterDeclarations(id->value, statements, from);
            if (!symbol && declarations.empty()) return isIntegerText(id->value);
            if (symbol && !isTensorOrNumericType(symbol->type.get())) return false;
            for (auto* decl : declarations) {
                auto declType = decl->getType();
                if (!isTensorOrNumericType(declType.get())) return false;
            }
            return true;
        }
    }
    if (!identifiersResolve(left)) return false;
    auto leftType = left->getType();
    return isTensorOrNumericType(leftType.get());
}

bool onlyBorrowingTensorUses(const std::string& name, std::vector<std::unique_ptr<ASTNode>>& statements, size_t from) {
    bool safe = true;
    std::function<void(ASTNode*, ASTNode*)> visit = [&](ASTNode* node, ASTNode* parent) {
        if (!node || !safe) return;
        if (auto* id = dynamic_cast<IdentifierNode*>(node)) {
            if (id->value.rfind(name + ".", 0) == 0) {
                const std::string component = id->value.substr(name.size() + 1);
                if (component != "x" && component != "y" && component != "z" && component != "w") safe = false;
                return;
            }
            if (id->value != name) return;
            if (auto* binary = dynamic_cast<BinaryExpressionNode*>(parent)) {
                if (binary->left.get() == node) return;
                if (binary->right.get() == node && cannotDispatchClassOperator(binary->left.get(), statements, from)) return;
            }
            if (auto* call = dynamic_cast<FunctionCallNode*>(parent)) {
                if (isTensorBorrowingBuiltin(call->functionName) &&
                    laterDeclarations(call->functionName, statements, from).empty()) return;
            }
            if (auto* access = dynamic_cast<ArrayAccessNode*>(parent)) {
                if (access->array.get() == node) return;
            }
            if (auto* ret = dynamic_cast<ReturnStatementNode*>(parent)) {
                if (ret->expression.get() == node) return;
            }
            safe = false;
            return;
        }
        if (dynamic_cast<LambdaExpressionNode*>(node)) {
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
    return safe;
}
bool initializerOwnsResult(ASTNode* initializer) {
    if (dynamic_cast<ClassInstanceCreationNode*>(initializer)) return true;
    if (auto* prefix = dynamic_cast<PrefixExpressionNode*>(initializer)) return prefix->op == "<-";
    if (auto* call = dynamic_cast<FunctionCallNode*>(initializer)) {
        return CodeGenerator::getInstance().symbolTable.lookupFunction(call->functionName, {}) != nullptr;
    }
    if (auto* call = dynamic_cast<MethodCallNode*>(initializer)) {
        auto receiverType = call->object ? call->object->getType() : nullptr;
        if (!receiverType || receiverType->getKind() != Type::Kind::CLASS) return false;
        std::string className = receiverType->getName();
        std::string templateName;
        if (templateInstanceArguments(className, &templateName, nullptr)) className = templateName;
        return !methodMayKeepThis(className, call->methodName);
    }
    return false;
}

bool yieldsValue(ASTNode* statement) {
    return dynamic_cast<IdentifierNode*>(statement) || dynamic_cast<PrefixExpressionNode*>(statement) ||
           dynamic_cast<TupleExpressionNode*>(statement) || dynamic_cast<IfStatementNode*>(statement) ||
           dynamic_cast<MatchExpressionNode*>(statement) || dynamic_cast<BlockNode*>(statement) ||
           dynamic_cast<CastingExpressionNode*>(statement);
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
            auto declType = decl->initializer ? decl->getType() : nullptr;
            if (declType && declType->getName() == "Tensor") {
                decl->autoDrop = !decl->isMutable &&
                    (isStrictOwnership() || onlyBorrowingTensorUses(decl->name, statements, i + 1));
            }
            else if (isStrictOwnership() && initializerOwnsResult(decl->initializer.get())) {
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
        if (i + 1 == statements.size() && last && yieldsValue(stmt.get())) cg.promoteTemporaryString(last);
        cg.exitTemporaryFrame();
    }

    auto* currentBlock = cg.builder.GetInsertBlock();
    if (currentBlock && !csec_llvm::terminatorOrNull(currentBlock)) {
        if (last && yieldsValue(statements.back().get())) {
            cg.releaseFromCurrentScope(last);
            if (dynamic_cast<IdentifierNode*>(statements.back().get()) && cg.hasDropFlag(last)) cg.noteMoved(last);
        }
        cg.emitCurrentScopeCleanups();
    }
    cg.exitCleanupScope();
    cg.symbolTable.exitScope();

    return last;
}
