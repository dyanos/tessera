#include "codegen.h"
#include "debug_info.h"
#include "LambdaExpressionNode.h"
#include "ASTVisitor.h"
#include "ParameterNode.h"
#include "IdentifierNode.h"
#include "BinaryExpressionNode.h"
#include "UnaryExpressionNode.h"
#include "ReturnStatementNode.h"
#include "BlockNode.h"
#include "IfStatementNode.h"
#include "WhileStatementNode.h"
#include "CallExpressionNode.h"
#include "FunctionCallNode.h"
#include "AssignmentExpressionNode.h"
#include "AccessFieldNode.h"
#include "ArrayAccessNode.h"
#include "VariableDeclarationNode.h"
#include "PrefixExpressionNode.h"
#include "PostfixExpressionNode.h"
#include "MethodCallNode.h"
#include "AssignmentNode.h"
#include "ForStatementNode.h"
#include "ClassInstanceCreationNode.h"
#include "ArrayLiteralNode.h"
#include "DictLiteralNode.h"
#include "ArrayCreationExpressionNode.h"
#include "CastingExpressionNode.h"
#include "TupleExpressionNode.h"
#include "MatchExpressionNode.h"
#include "RangeExpressionNode.h"
#include "DestructuringAssignmentNode.h"
#include "type_utils.h"
#include "ast_walk.h"
#include "source_map.h"

#include <iostream>
#include <set>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>

namespace {
std::unique_ptr<Type> usableLambdaType(std::unique_ptr<Type> type) {
    if (!type || type->getKind() == Type::Kind::UNKNOWN) {
        return std::make_unique<BasicType>("Int");
    }
    return type;
}

// Collect the identifier names referenced anywhere in a lambda body. Used to detect free
// variables (captures). Handles the expression and statement nodes that appear in lambda bodies;
// an unrecognized node contributes nothing, which can only under-report a capture.
void collectReferencedIdentifiers(ASTNode* node, std::set<std::string>& names) {
    ast_walk::walk(node, nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
        if (auto* ident = dynamic_cast<IdentifierNode*>(n)) {
            names.insert(ident->value);
            const auto dot = ident->value.find('.');
            if (dot != std::string::npos) names.insert(ident->value.substr(0, dot));
        }
        else if (auto* assignment = dynamic_cast<AssignmentNode*>(n)) {
            names.insert(assignment->name);
        }
        else if (auto* access = dynamic_cast<AccessFieldNode*>(n)) {
            // The field name is not a reference to a variable.
            collectReferencedIdentifiers(access->base.get(), names);
            return false;
        }
        return true;
    });
}

llvm::Value* defaultReturnValue(llvm::Type* type) {
    auto& cg = CodeGenerator::getInstance();
    if (!type || type->isVoidTy()) {
        return nullptr;
    }
    if (type->isIntegerTy()) {
        return llvm::ConstantInt::get(type, 0, true);
    }
    if (type->isFloatingPointTy()) {
        return llvm::ConstantFP::get(type, 0.0);
    }
    if (type->isPointerTy()) {
        return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
    }
    return llvm::Constant::getNullValue(type);
}
}

namespace {
// Names a function body returns (`return x`, or `x` as its last statement).
void collectReturnedNames(ASTNode* body, std::set<std::string>& names) {
    ast_walk::walk(body, nullptr, [&](ASTNode* node, std::unique_ptr<ASTNode>*) {
        if (dynamic_cast<LambdaExpressionNode*>(node) && node != body) return false;
        if (auto* ret = dynamic_cast<ReturnStatementNode*>(node)) {
            if (auto* id = dynamic_cast<IdentifierNode*>(ret->expression.get())) names.insert(id->value);
        }
        return true;
    });
    if (auto* block = dynamic_cast<BlockNode*>(body)) {
        if (!block->statements.empty()) {
            if (auto* id = dynamic_cast<IdentifierNode*>(block->statements.back().get())) names.insert(id->value);
        }
    }
}

struct EscapeMarker {
    int errors = 0;

    void mark(ASTNode* candidate, int line) {
        if (auto* named = dynamic_cast<AssignmentExpressionNode*>(candidate)) {
            if (named->isNamedArgument) candidate = named->right.get();  // `f(callback = [=](...) -> ...)`
        }
        auto* lambda = dynamic_cast<LambdaExpressionNode*>(candidate);
        if (!lambda || lambda->escapes) return;
        lambda->escapes = true;
        if (lambda->capturesByReference) {
            const SourceLocation location = sourceLocationOf(line);
            std::cerr << "Error" << (location.file.empty() ? std::string() : " (" + location.file + ":" + std::to_string(location.line) + ")")
                      << ": a lambda that captures by reference ([&]) cannot be returned or stored: the variables "
                         "it refers to end with the function; capture by value with [=]" << std::endl;
            ++errors;
        }
    }

    // One function (or lambda) body.
    void visitBody(ASTNode* body) {
        if (!body) return;
        std::set<std::string> returned;
        collectReturnedNames(body, returned);
        if (auto* block = dynamic_cast<BlockNode*>(body)) {
            if (!block->statements.empty()) mark(block->statements.back().get(), block->statements.back()->line);
        }
        else {
            mark(body, body->line);
        }
        int line = 0;
        ast_walk::walk(body, nullptr, [&](ASTNode* node, std::unique_ptr<ASTNode>*) {
            if (node->line > 0) line = node->line;
            if (auto* lambda = dynamic_cast<LambdaExpressionNode*>(node)) {
                if (node != body) {
                    visitBody(lambda->body.get());
                    return false;
                }
            }
            if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(node)) {
                visitBody(fn->body.get());
                return false;
            }
            if (auto* ret = dynamic_cast<ReturnStatementNode*>(node)) mark(ret->expression.get(), line);
            else if (auto* decl = dynamic_cast<VariableDeclarationNode*>(node)) {
                if (returned.count(decl->name)) mark(decl->initializer.get(), line);
            }
            else if (auto* assign = dynamic_cast<AssignmentExpressionNode*>(node)) {
                auto* target = dynamic_cast<IdentifierNode*>(assign->left.get());
                if (!target || returned.count(target->value) || target->value.find('.') != std::string::npos) {
                    mark(assign->right.get(), line);  // a field, an element, or a returned variable
                }
            }
            else if (auto* creation = dynamic_cast<ClassInstanceCreationNode*>(node)) {
                for (auto& a : creation->arguments) mark(a.get(), line);
            }
            else if (auto* literal = dynamic_cast<ArrayLiteralNode*>(node)) {
                for (auto& e : literal->elements) mark(e.get(), line);
            }
            else if (auto* dict = dynamic_cast<DictLiteralNode*>(node)) {
                for (auto& v : dict->values) mark(v.get(), line);
            }
            else if (auto* tuple = dynamic_cast<TupleExpressionNode*>(node)) {
                for (auto& e : tuple->elements) mark(e.get(), line);
            }
            else if (auto* construct = dynamic_cast<EnumConstructNode*>(node)) {
                for (auto& a : construct->arguments) mark(a.get(), line);
            }
            return true;
        });
    }
};
}  // namespace

bool markEscapingLambdas(ASTNode* program) {
    EscapeMarker marker;
    ast_walk::walk(program, nullptr, [&](ASTNode* node, std::unique_ptr<ASTNode>*) {
        if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(node)) {
            marker.visitBody(fn->body.get());
            return false;
        }
        return true;
    });
    return marker.errors == 0;
}

void LambdaExpressionNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* LambdaExpressionNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    auto* opaquePtr = llvm::PointerType::getUnqual(cg.context);

    // Determine the captured variables at the enclosing scope (before entering the lambda scope).
    // An explicit capture list is used verbatim; `[=]`/`[&]` capture every free variable of the
    // body that resolves to an outer local.
    std::set<std::string> paramNames;
    for (auto& arg : arguments) {
        if (auto* paramNode = dynamic_cast<ParameterNode*>(arg.get())) paramNames.insert(paramNode->name);
    }
    std::vector<std::string> capturedNames;
    {
        std::set<std::string> candidates;
        if (!captureVariables.empty()) {
            for (const auto& capture : captureVariables) candidates.insert(capture);
        } else {
            std::set<std::string> referenced;
            collectReferencedIdentifiers(body.get(), referenced);
            for (const auto& name : referenced) {
                if (paramNames.count(name)) continue;
                auto* symbol = cg.symbolTable.lookup(name);
                if (symbol && (symbol->symbolType == SymbolType::VARIABLE || symbol->symbolType == SymbolType::FIELD)) {
                    candidates.insert(name);
                }
            }
        }
        for (const auto& name : candidates) capturedNames.push_back(name);
    }

    struct Captured {
        std::string name;
        llvm::Value* outerValue;
        llvm::Type* fieldType;
        std::unique_ptr<Type> semanticType;
    };
    std::vector<Captured> captured;
    std::vector<llvm::Type*> envFieldTypes;
    for (const auto& name : capturedNames) {
        auto* symbol = cg.symbolTable.lookup(name);
        if (!symbol || !symbol->value) continue;
        llvm::Type* fieldType = capturesByReference ? opaquePtr : getABIStorageType(symbol->type.get());
        if (!fieldType) fieldType = llvm::Type::getInt32Ty(cg.context);
        Captured entry;
        entry.name = name;
        entry.outerValue = symbol->value;
        entry.fieldType = fieldType;
        entry.semanticType = symbol->type ? symbol->type->clone() : std::make_unique<UnknownType>();
        captured.push_back(std::move(entry));
        envFieldTypes.push_back(fieldType);
    }
    // Every lambda is represented uniformly as a { code, env } closure whose function takes the
    // environment pointer first, even when it captures nothing (empty environment). This gives
    // lambda values one representation, so a function-type parameter can be called the same way
    // whether or not the lambda it received captures anything.
    llvm::StructType* envType = llvm::StructType::get(cg.context, envFieldTypes);
    environmentType = envType;

    std::vector<llvm::Type*> paramTypes;
    paramTypes.push_back(opaquePtr);
    for (auto& arg : arguments) {
        auto paramType = usableLambdaType(arg ? arg->getType() : nullptr);
        auto* argType = cg.getLLVMType(paramType.get());
        if (!argType) {
            std::cerr << "Error: Invalid lambda parameter type" << std::endl;
            return nullptr;
        }
        paramTypes.push_back(argType);
    }

    auto bodyType = usableLambdaType(body ? body->getType() : nullptr);
    llvm::Type* returnType = bodyType ? cg.getLLVMType(bodyType.get()) : nullptr;
    if (!returnType) returnType = llvm::Type::getInt32Ty(cg.context);

    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
    static int lambdaCount = 0;
    std::string lambdaName = "__lambda_" + std::to_string(lambdaCount++);
    llvm::Function* lambdaFunc = llvm::Function::Create(
        funcType, llvm::Function::InternalLinkage, lambdaName, cg.module.get());

    llvm::BasicBlock* savedBB = cg.builder.GetInsertBlock();
    auto savedPoint = cg.builder.GetInsertPoint();
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(cg.context, "entry", lambdaFunc);
    cg.builder.SetInsertPoint(entry);
    debuginfo::beginFunction(lambdaFunc, "lambda", line);
    cg.symbolTable.enterScope();

    // Bind captured variables from the environment argument.
    {
        llvm::Value* envArg = lambdaFunc->getArg(0);
        for (size_t i = 0; i < captured.size(); ++i) {
            llvm::Value* slot = cg.builder.CreateStructGEP(envType, envArg, static_cast<unsigned>(i), captured[i].name + ".cap");
            if (capturesByReference) {
                // The slot holds a pointer to the outer variable; bind the name to that storage.
                llvm::Value* storage = cg.builder.CreateLoad(opaquePtr, slot, captured[i].name + ".ref");
                cg.symbolTable.addSymbol(captured[i].name, std::make_unique<Symbol>(
                    captured[i].name, captured[i].semanticType->clone(), storage, true, SymbolType::VARIABLE));
            } else {
                // The slot holds the captured value; copy it into a local so the name is an lvalue.
                llvm::Value* localCopy = cg.builder.CreateAlloca(captured[i].fieldType, nullptr, captured[i].name);
                llvm::Value* value = cg.builder.CreateLoad(captured[i].fieldType, slot, captured[i].name + ".val");
                cg.builder.CreateStore(value, localCopy);
                cg.symbolTable.addSymbol(captured[i].name, std::make_unique<Symbol>(
                    captured[i].name, captured[i].semanticType->clone(), localCopy, false, SymbolType::VARIABLE));
            }
        }
    }

    // Bind declared parameters (after the environment argument).
    const unsigned argBase = 1u;
    for (size_t i = 0; i < arguments.size(); ++i) {
        auto* paramNode = static_cast<ParameterNode*>(arguments[i].get());
        auto paramType = usableLambdaType(arguments[i] ? arguments[i]->getType() : nullptr);
        cg.symbolTable.addSymbol(paramNode->name, std::make_unique<Symbol>(
            paramNode->name, std::move(paramType), lambdaFunc->getArg(argBase + static_cast<unsigned>(i)), false, SymbolType::VARIABLE));
    }

    llvm::Value* bodyValue = body ? body->codegen() : nullptr;
    if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
        if (bodyValue && !returnType->isVoidTy()) cg.builder.CreateRet(bodyValue);
        else if (!returnType->isVoidTy()) cg.builder.CreateRet(defaultReturnValue(returnType));
        else cg.builder.CreateRetVoid();
    }

    cg.symbolTable.exitScope();
    cg.builder.SetInsertPoint(savedBB, savedPoint);

    // Build the { code, env } closure. Fill the environment with the captured values (by value)
    // or the outer variables' addresses (by reference); an empty environment for a lambda that
    // captures nothing.
    // A closure that outlives this frame lives on the heap; any other one on the stack.
    auto heapAllocate = [&](llvm::Type* type, const char* name) -> llvm::Value* {
        auto* size = llvm::ConstantExpr::getSizeOf(type);
        llvm::Value* bytes = size->getType() == llvm::Type::getInt64Ty(cg.context)
            ? static_cast<llvm::Value*>(size)
            : cg.builder.CreateZExtOrTrunc(size, llvm::Type::getInt64Ty(cg.context));
        return cg.builder.CreateCall(cg.mallocFunction, bytes, name);
    };
    llvm::Value* environment = escapes ? heapAllocate(envType, "lambda.env.heap")
                                       : cg.createEntryAlloca(envType, "lambda.env");
    for (size_t i = 0; i < captured.size(); ++i) {
        llvm::Value* slot = cg.builder.CreateStructGEP(envType, environment, static_cast<unsigned>(i), captured[i].name + ".store");
        // A function parameter is bound to its SSA llvm::Argument rather than to storage: capture
        // its value directly, or spill it to a slot so a by-reference capture has an address.
        const bool isSsaParameter = llvm::isa<llvm::Argument>(captured[i].outerValue);
        if (capturesByReference) {
            llvm::Value* storage = captured[i].outerValue;
            if (isSsaParameter) {
                storage = cg.builder.CreateAlloca(storage->getType(), nullptr, captured[i].name + ".spill");
                cg.builder.CreateStore(captured[i].outerValue, storage);
            }
            cg.builder.CreateStore(storage, slot);
        } else {
            llvm::Value* value = captured[i].outerValue;
            if (!isSsaParameter) {
                IdentifierNode reference(captured[i].name);
                value = reference.codegen();
            }
            if (value && value->getType() != captured[i].fieldType && value->getType()->isPointerTy()) {
                value = cg.builder.CreateLoad(captured[i].fieldType, value, captured[i].name + ".take");
            }
            if (!value || value->getType() != captured[i].fieldType) {
                value = llvm::Constant::getNullValue(captured[i].fieldType);
            }
            if (escapes) {
                value = cg.copyStackBufferToHeap(value);
                cg.noteMoved(captured[i].outerValue);
            }
            if (captured[i].semanticType && captured[i].semanticType->getName() == "String") value = cg.takeOwnedString(value);
            cg.builder.CreateStore(value, slot);
        }
    }
    llvm::StructType* closureType = llvm::StructType::get(cg.context, { opaquePtr, opaquePtr });
    llvm::Value* closure = escapes ? heapAllocate(closureType, "lambda.closure.heap")
                                   : cg.createEntryAlloca(closureType, "lambda.closure");
    cg.builder.CreateStore(lambdaFunc, cg.builder.CreateStructGEP(closureType, closure, 0, "closure.code"));
    cg.builder.CreateStore(environment, cg.builder.CreateStructGEP(closureType, closure, 1, "closure.env"));
    return closure;
}
