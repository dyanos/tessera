#include "ProgramNode.h"
#include "ASTVisitor.h"
#include "FunctionDeclarationNode.h"
#include "ClassDeclarationNode.h"
#include "AttributeNode.h"

void ProgramNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* ProgramNode::codegen() {
    // Lay out every class and struct first, so prototypes and methods can name them in any order
    // (value structs are passed by value and need a sized type). Retry while layouts resolve.
    {
        std::vector<ClassDeclarationNode*> pending;
        for (auto& stmt : statements) {
            if (auto* cls = dynamic_cast<ClassDeclarationNode*>(stmt.get())) {
                pending.push_back(cls);
            }
        }
        bool progress = true;
        while (progress && !pending.empty()) {
            progress = false;
            for (auto it = pending.begin(); it != pending.end();) {
                if ((*it)->predeclareLayout()) {
                    it = pending.erase(it);
                    progress = true;
                } else {
                    ++it;
                }
            }
        }
    }

    // Declare every top-level function prototype before emitting any body, so a function can call
    // another that is declared later in the file (forward references and mutual recursion).
    for (auto& stmt : statements) {
        if (auto* function = dynamic_cast<FunctionDeclarationNode*>(stmt.get())) {
            function->declarePrototype();
            continue;
        }
        // `[@Inline] def f(...)` too; an attributed `external def` gets its symbol from the attribute.
        ASTNode* target = stmt.get();
        while (auto* attribute = dynamic_cast<AttributeNode*>(target)) target = attribute->target.get();
        if (auto* function = dynamic_cast<FunctionDeclarationNode*>(target)) {
            if (!function->isExternal) function->declarePrototype();
        }
    }

    llvm::Value* last = nullptr;
    for (auto& stmt : statements) {
        last = stmt->codegen();
    }
    return last;
}
