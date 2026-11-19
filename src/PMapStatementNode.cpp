#include "codegen.h"
#include "PMapStatementNode.h"
#include "ASTVisitor.h"
#include "ArrayLiteralNode.h"
#include "all_ast.h"

#include <iostream>
#include <set>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/DerivedTypes.h>

namespace {
int nextPMapId() {
    static int id = 0;
    return ++id;
}

struct PMapCapture {
    std::string name;
    std::unique_ptr<Type> type;
    llvm::Type* llvmType = nullptr;
    llvm::Value* value = nullptr;
};

std::unique_ptr<Type> pmapElementSourceType(const std::unique_ptr<Type>& type) {
    if (!type) return std::make_unique<UnknownType>();
    if (auto* arrType = dynamic_cast<ArrayType*>(type.get())) {
        return arrType->elementType ? arrType->elementType->clone() : std::make_unique<UnknownType>();
    }
    if (auto* genType = dynamic_cast<GenericType*>(type.get())) {
        if (genType->baseType && genType->baseType->getName() == "Array" && !genType->typeArguments.empty()) {
            return genType->typeArguments[0] ? genType->typeArguments[0]->clone() : std::make_unique<UnknownType>();
        }
    }
    return std::make_unique<UnknownType>();
}

void attachParallelLoopMetadata(llvm::LLVMContext& context, llvm::Instruction* latch, const std::string& backend) {
    if (!latch || backend == "cpu") return;

    auto temp = llvm::MDNode::getTemporary(context, {});
    llvm::Metadata* vectorizeOperands[] = {
        llvm::MDString::get(context, "llvm.loop.vectorize.enable"),
        llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), true))
    };
    llvm::Metadata* operands[] = {
        temp.get(),
        llvm::MDNode::get(context, vectorizeOperands),
        llvm::MDNode::get(context, llvm::MDString::get(context, "csec.pmap.backend." + backend))
    };
    auto* loopId = llvm::MDNode::getDistinct(context, operands);
    loopId->replaceOperandWith(0, loopId);
    latch->setMetadata(llvm::LLVMContext::MD_loop, loopId);
}

llvm::Function* getOrCreateParallelForI32(CodeGenerator& cg) {
    cg.requireSystemNative();
    if (auto* function = cg.module->getFunction("csec_parallel_for_i32")) {
        return function;
    }

    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* voidTy = llvm::Type::getVoidTy(cg.context);
    auto* ptrTy = llvm::PointerType::getUnqual(cg.context);
    auto* callbackTy = llvm::FunctionType::get(voidTy, { ptrTy, i32Ty }, false);
    auto* callbackPtrTy = llvm::PointerType::getUnqual(callbackTy->getContext());
    auto* functionTy = llvm::FunctionType::get(voidTy, { i32Ty, i32Ty, ptrTy, callbackPtrTy }, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "csec_parallel_for_i32", cg.module.get());
}

bool isCapturedIdentifierCandidate(const std::string& name) {
    return !name.empty() && name != "true" && name != "false";
}

void addCaptureName(
    const std::string& name,
    const std::string& loopVariable,
    const std::set<std::string>& locals,
    std::vector<std::string>& captures,
    std::set<std::string>& seen) {
    if (!isCapturedIdentifierCandidate(name) || name == loopVariable || locals.count(name) > 0 || seen.count(name) > 0) {
        return;
    }

    auto& cg = CodeGenerator::getInstance();
    Symbol* symbol = cg.symbolTable.lookup(name);
    if (!symbol || !symbol->value || !symbol->type) {
        return;
    }
    if (symbol->symbolType != SymbolType::VARIABLE && symbol->symbolType != SymbolType::FIELD) {
        return;
    }

    seen.insert(name);
    captures.push_back(name);
}

void collectPMapCaptures(
    ASTNode* ast,
    const std::string& loopVariable,
    std::set<std::string> locals,
    std::vector<std::string>& captures,
    std::set<std::string>& seen,
    std::set<std::string>& writes);

void collectAssignmentTargetWrites(
    ASTNode* ast,
    const std::string& loopVariable,
    const std::set<std::string>& locals,
    std::set<std::string>& writes) {
    if (!ast) return;

    if (auto* ident = dynamic_cast<IdentifierNode*>(ast)) {
        if (ident->value != loopVariable && locals.count(ident->value) == 0 && isCapturedIdentifierCandidate(ident->value)) {
            auto& cg = CodeGenerator::getInstance();
            Symbol* symbol = cg.symbolTable.lookup(ident->value);
            if (symbol && symbol->value && symbol->symbolType == SymbolType::VARIABLE) {
                writes.insert(ident->value);
            }
        }
        return;
    }

    if (auto* access = dynamic_cast<ArrayAccessNode*>(ast)) {
        collectAssignmentTargetWrites(access->array.get(), loopVariable, locals, writes);
        return;
    }

    if (auto* field = dynamic_cast<AccessFieldNode*>(ast)) {
        collectAssignmentTargetWrites(field->base.get(), loopVariable, locals, writes);
    }
}

void collectPMapCaptures(
    ASTNode* ast,
    const std::string& loopVariable,
    std::set<std::string> locals,
    std::vector<std::string>& captures,
    std::set<std::string>& seen,
    std::set<std::string>& writes) {
    if (!ast) return;

    if (auto* ident = dynamic_cast<IdentifierNode*>(ast)) {
        addCaptureName(ident->value, loopVariable, locals, captures, seen);
        return;
    }
    if (auto* block = dynamic_cast<BlockNode*>(ast)) {
        for (const auto& stmt : block->statements) {
            if (auto* decl = dynamic_cast<VariableDeclarationNode*>(stmt.get())) {
                collectPMapCaptures(decl->initializer.get(), loopVariable, locals, captures, seen, writes);
                locals.insert(decl->name);
            } else {
                collectPMapCaptures(stmt.get(), loopVariable, locals, captures, seen, writes);
            }
        }
        return;
    }
    if (auto* binary = dynamic_cast<BinaryExpressionNode*>(ast)) {
        collectPMapCaptures(binary->left.get(), loopVariable, locals, captures, seen, writes);
        collectPMapCaptures(binary->right.get(), loopVariable, locals, captures, seen, writes);
        return;
    }
    if (auto* assign = dynamic_cast<AssignmentExpressionNode*>(ast)) {
        collectAssignmentTargetWrites(assign->left.get(), loopVariable, locals, writes);
        collectPMapCaptures(assign->right.get(), loopVariable, locals, captures, seen, writes);
        return;
    }
    if (auto* call = dynamic_cast<FunctionCallNode*>(ast)) {
        for (const auto& arg : call->arguments) {
            collectPMapCaptures(arg.get(), loopVariable, locals, captures, seen, writes);
        }
        return;
    }
    if (auto* method = dynamic_cast<MethodCallNode*>(ast)) {
        collectPMapCaptures(method->object.get(), loopVariable, locals, captures, seen, writes);
        for (const auto& arg : method->arguments) {
            collectPMapCaptures(arg.get(), loopVariable, locals, captures, seen, writes);
        }
        return;
    }
    if (auto* access = dynamic_cast<ArrayAccessNode*>(ast)) {
        collectPMapCaptures(access->array.get(), loopVariable, locals, captures, seen, writes);
        collectPMapCaptures(access->index.get(), loopVariable, locals, captures, seen, writes);
        for (const auto& spec : access->indices) {
            if (!spec) continue;
            collectPMapCaptures(spec->index.get(), loopVariable, locals, captures, seen, writes);
            collectPMapCaptures(spec->start.get(), loopVariable, locals, captures, seen, writes);
            collectPMapCaptures(spec->end.get(), loopVariable, locals, captures, seen, writes);
            collectPMapCaptures(spec->step.get(), loopVariable, locals, captures, seen, writes);
        }
        return;
    }
    if (auto* field = dynamic_cast<AccessFieldNode*>(ast)) {
        collectPMapCaptures(field->base.get(), loopVariable, locals, captures, seen, writes);
        return;
    }
    if (auto* prefix = dynamic_cast<PrefixExpressionNode*>(ast)) {
        if (prefix->op == "++" || prefix->op == "--") {
            collectAssignmentTargetWrites(prefix->expression.get(), loopVariable, locals, writes);
        } else {
            collectPMapCaptures(prefix->expression.get(), loopVariable, locals, captures, seen, writes);
        }
        return;
    }
    if (auto* postfix = dynamic_cast<PostfixExpressionNode*>(ast)) {
        if (postfix->op == "++" || postfix->op == "--") {
            collectAssignmentTargetWrites(postfix->expression.get(), loopVariable, locals, writes);
        } else {
            collectPMapCaptures(postfix->expression.get(), loopVariable, locals, captures, seen, writes);
        }
        return;
    }
    if (auto* unary = dynamic_cast<UnaryExpressionNode*>(ast)) {
        collectPMapCaptures(unary->expression.get(), loopVariable, locals, captures, seen, writes);
        return;
    }
    if (auto* ifStmt = dynamic_cast<IfStatementNode*>(ast)) {
        collectPMapCaptures(ifStmt->condition.get(), loopVariable, locals, captures, seen, writes);
        collectPMapCaptures(ifStmt->thenBlock.get(), loopVariable, locals, captures, seen, writes);
        collectPMapCaptures(ifStmt->elseBlock.get(), loopVariable, locals, captures, seen, writes);
        return;
    }
    if (auto* ret = dynamic_cast<ReturnStatementNode*>(ast)) {
        collectPMapCaptures(ret->expression.get(), loopVariable, locals, captures, seen, writes);
        return;
    }
    if (auto* literal = dynamic_cast<ArrayLiteralNode*>(ast)) {
        for (const auto& elem : literal->elements) {
            collectPMapCaptures(elem.get(), loopVariable, locals, captures, seen, writes);
        }
        return;
    }
    // A nested pmap: its iterable and body reference variables that the enclosing pmap must
    // capture too. Walk both, treating the inner loop variable as a local of the inner scope.
    if (auto* nested = dynamic_cast<PMapStatementNode*>(ast)) {
        collectPMapCaptures(nested->iterableExpr.get(), loopVariable, locals, captures, seen, writes);
        std::set<std::string> innerLocals = locals;
        innerLocals.insert(nested->variable);
        collectPMapCaptures(nested->body.get(), loopVariable, innerLocals, captures, seen, writes);
        return;
    }
    if (auto* match = dynamic_cast<MatchExpressionNode*>(ast)) {
        collectPMapCaptures(match->expression.get(), loopVariable, locals, captures, seen, writes);
        for (const auto& item : match->cases) {
            collectPMapCaptures(item.first.get(), loopVariable, locals, captures, seen, writes);
            collectPMapCaptures(item.second.get(), loopVariable, locals, captures, seen, writes);
        }
    }
}

llvm::Value* loadCapturedValue(CodeGenerator& cg, Symbol* symbol, llvm::Type* llvmType, const std::string& name) {
    if (!symbol || !symbol->value || !llvmType) return nullptr;
    IdentifierNode reference(name);
    llvm::Value* value = reference.codegen();
    if (value && value->getType() != llvmType && value->getType()->isPointerTy()) {
        value = cg.builder.CreateLoad(llvmType, value, name + ".pmap.capture");
    }
    return value;
}

llvm::Value* codegenOutlinedParallelPMap(
    PMapStatementNode& node,
    llvm::Value* arrayPtr,
    const std::unique_ptr<Type>& iterType,
    llvm::Type* elementType,
    int arraySize,
    llvm::Type* resultElementType) {
    auto& cg = CodeGenerator::getInstance();
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* voidTy = llvm::Type::getVoidTy(cg.context);
    auto* ptrTy = llvm::PointerType::getUnqual(cg.context);

    llvm::Value* resultPtr = cg.builder.CreateAlloca(resultElementType, cg.builder.getInt32(arraySize), "pmapresult");

    std::vector<std::string> captureNames;
    std::set<std::string> seenCaptures;
    std::set<std::string> capturedWrites;
    collectPMapCaptures(node.body.get(), node.variable, {}, captureNames, seenCaptures, capturedWrites);
    if (!capturedWrites.empty()) {
        std::cerr << "Error: pmap(openmp) does not allow writes to captured variable '"
                  << *capturedWrites.begin() << "'" << std::endl;
        return nullptr;
    }

    std::vector<PMapCapture> captures;
    captures.reserve(captureNames.size());
    for (const auto& name : captureNames) {
        Symbol* symbol = cg.symbolTable.lookup(name);
        if (!symbol || !symbol->type || !symbol->value) {
            continue;
        }
        llvm::Type* captureType = cg.getLLVMType(symbol->type.get());
        llvm::Value* captureValue = loadCapturedValue(cg, symbol, captureType, name);
        if (!captureType || !captureValue || captureValue->getType() != captureType) {
            std::cerr << "Error: unsupported pmap(openmp) capture '" << name << "'" << std::endl;
            return nullptr;
        }
        captures.push_back(PMapCapture{ name, symbol->type->clone(), captureType, captureValue });
    }

    const int id = nextPMapId();
    auto* ctxTy = llvm::StructType::create(cg.context, "csec.pmap.ctx." + std::to_string(id));
    std::vector<llvm::Type*> ctxFields = { arrayPtr->getType(), resultPtr->getType() };
    for (const auto& capture : captures) {
        ctxFields.push_back(capture.llvmType);
    }
    ctxTy->setBody(ctxFields);

    llvm::Value* ctx = cg.builder.CreateAlloca(ctxTy, nullptr, "pmap.ctx");
    cg.builder.CreateStore(arrayPtr, cg.builder.CreateStructGEP(ctxTy, ctx, 0, "pmap.ctx.input.slot"));
    cg.builder.CreateStore(resultPtr, cg.builder.CreateStructGEP(ctxTy, ctx, 1, "pmap.ctx.result.slot"));
    for (size_t i = 0; i < captures.size(); ++i) {
        cg.builder.CreateStore(
            captures[i].value,
            cg.builder.CreateStructGEP(ctxTy, ctx, static_cast<unsigned>(i + 2), "pmap.ctx.capture.slot"));
    }

    auto* callbackTy = llvm::FunctionType::get(voidTy, { ptrTy, i32Ty }, false);
    auto* callback = llvm::Function::Create(
        callbackTy,
        llvm::Function::InternalLinkage,
        "__csec_pmap_body_" + std::to_string(id),
        cg.module.get());

    auto savedIP = cg.builder.saveIP();
    auto* entryBB = llvm::BasicBlock::Create(cg.context, "entry", callback);
    cg.builder.SetInsertPoint(entryBB);

    auto argIt = callback->arg_begin();
    llvm::Value* callbackCtx = &*argIt++;
    callbackCtx->setName("ctx");
    llvm::Value* idx = &*argIt;
    idx->setName("idx");

    llvm::Value* inputBase = cg.builder.CreateLoad(
        arrayPtr->getType(),
        cg.builder.CreateStructGEP(ctxTy, callbackCtx, 0, "pmap.cb.input.slot"),
        "pmap.cb.input");
    llvm::Value* outputBase = cg.builder.CreateLoad(
        resultPtr->getType(),
        cg.builder.CreateStructGEP(ctxTy, callbackCtx, 1, "pmap.cb.result.slot"),
        "pmap.cb.result");

    llvm::Value* elemPtr = cg.builder.CreateGEP(elementType, inputBase, idx, "pmap.cb.elem.ptr");
    llvm::Value* elemValue = cg.builder.CreateLoad(elementType, elemPtr, "pmap.cb.elem");
    llvm::Value* varPtr = cg.builder.CreateAlloca(elementType, nullptr, node.variable + "_ptr");
    cg.builder.CreateStore(elemValue, varPtr);

    cg.symbolTable.enterScope();
    std::unique_ptr<Type> varType = pmapElementSourceType(iterType);
    cg.symbolTable.addSymbol(node.variable, std::make_unique<Symbol>(
        node.variable, std::move(varType), varPtr, false, SymbolType::VARIABLE));
    for (size_t i = 0; i < captures.size(); ++i) {
        const auto& capture = captures[i];
        llvm::Value* captureValue = cg.builder.CreateLoad(
            capture.llvmType,
            cg.builder.CreateStructGEP(ctxTy, callbackCtx, static_cast<unsigned>(i + 2), capture.name + ".capture.slot"),
            capture.name + ".capture");
        llvm::Value* capturePtr = cg.builder.CreateAlloca(capture.llvmType, nullptr, capture.name + "_capture_ptr");
        cg.builder.CreateStore(captureValue, capturePtr);
        cg.symbolTable.addSymbol(capture.name, std::make_unique<Symbol>(
            capture.name, capture.type->clone(), capturePtr, false, SymbolType::VARIABLE));
    }

    llvm::Value* bodyResult = node.body->codegen();
    cg.symbolTable.exitScope();
    if (!bodyResult) {
        cg.builder.CreateRetVoid();
        cg.builder.restoreIP(savedIP);
        return nullptr;
    }
    if (bodyResult->getType()->isPointerTy()) {
        bodyResult = cg.builder.CreateLoad(resultElementType, bodyResult, "pmap.cb.body.load");
    }
    if (bodyResult->getType() != resultElementType) {
        std::cerr << "Type error: pmap body result type mismatch" << std::endl;
        cg.builder.CreateRetVoid();
        cg.builder.restoreIP(savedIP);
        return nullptr;
    }

    llvm::Value* resultElemPtr = cg.builder.CreateGEP(resultElementType, outputBase, idx, "pmap.cb.result.ptr");
    cg.builder.CreateStore(bodyResult, resultElemPtr);
    cg.builder.CreateRetVoid();

    cg.builder.restoreIP(savedIP);
    cg.builder.CreateCall(
        getOrCreateParallelForI32(cg),
        { cg.builder.getInt32(0), cg.builder.getInt32(arraySize), ctx, callback });

    return resultPtr;
}
}

void PMapStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* PMapStatementNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    llvm::Function* function = cg.builder.GetInsertBlock()->getParent();

    // The `gpu` backend has no device code generator yet, so it falls back to CPU execution: the
    // sequential loop below produces the same results (the backend is a performance hint, not a
    // semantic change). The loop is still tagged with `csec.pmap.backend.gpu` metadata so a future
    // GPU lowering pass can find it.

    llvm::Value* arrayPtr = iterableExpr->codegen();
    if (!arrayPtr) return nullptr;

    auto iterType = iterableExpr->getType();
    if (!iterType) return nullptr;

    // Extract element type and array size
    llvm::Type* elementType = nullptr;
    int arraySize = 0;
    bool sizeKnown = false;

    if (auto* arrType = dynamic_cast<ArrayType*>(iterType.get())) {
        elementType = cg.getLLVMType(arrType->elementType.get());
        arraySize = arrType->size;
        sizeKnown = true;
    } else if (auto* genType = dynamic_cast<GenericType*>(iterType.get())) {
        if (genType->baseType && genType->baseType->getName() == "Array" && !genType->typeArguments.empty()) {
            elementType = cg.getLLVMType(genType->typeArguments[0].get());
            if (iterableExpr && iterableExpr->nodeType == ASTNodeType::ARRAY_LITERAL) {
                auto* arrayLiteral = static_cast<ArrayLiteralNode*>(iterableExpr.get());
                arraySize = static_cast<int>(arrayLiteral->elements.size());
                sizeKnown = true;
            }
        }
    }

    // Accept both array values and pointers-to-array by decaying to element pointer.
    if (auto* llvmArrayTy = llvm::dyn_cast<llvm::ArrayType>(arrayPtr->getType())) {
        if (!elementType) {
            elementType = llvmArrayTy->getElementType();
        }
        if (!sizeKnown) {
            arraySize = static_cast<int>(llvmArrayTy->getNumElements());
            sizeKnown = true;
        }
        llvm::Value* tmpArray = cg.builder.CreateAlloca(llvmArrayTy, nullptr, "pmap_arr_tmp");
        cg.builder.CreateStore(arrayPtr, tmpArray);
        llvm::Value* zero = cg.builder.getInt32(0);
        arrayPtr = cg.builder.CreateInBoundsGEP(llvmArrayTy, tmpArray, { zero, zero }, "pmap_arr_decay");
    }
    else if (arrayPtr->getType()->isPointerTy() && elementType && sizeKnown) {
        auto* llvmArrayTy = llvm::ArrayType::get(elementType, arraySize);
        llvm::Value* zero = cg.builder.getInt32(0);
        arrayPtr = cg.builder.CreateInBoundsGEP(llvmArrayTy, arrayPtr, { zero, zero }, "pmap_arr_decay");
    }

    if (!elementType || !sizeKnown || arraySize < 0) {
        std::cerr << "Error: Cannot determine array element type or size for pmap" << std::endl;
        return nullptr;
    }

    cg.symbolTable.enterScope();
    std::unique_ptr<Type> preflightVarType = pmapElementSourceType(iterType);
    llvm::Value* preflightVarPtr = cg.builder.CreateAlloca(elementType, nullptr, variable + "_typecheck_ptr");
    cg.symbolTable.addSymbol(variable, std::make_unique<Symbol>(
        variable, std::move(preflightVarType), preflightVarPtr, false, SymbolType::VARIABLE));
    auto bodyType = body->getType();
    llvm::Type* resultElementType = bodyType ? cg.getLLVMType(bodyType.get()) : nullptr;
    cg.symbolTable.exitScope();
    if (!resultElementType) {
        std::cerr << "Type error: Cannot determine pmap result element type" << std::endl;
        return nullptr;
    }

    if (backend == "openmp") {
        return codegenOutlinedParallelPMap(*this, arrayPtr, iterType, elementType, arraySize, resultElementType);
    }

    // Allocate loop counter and element variable before the loop
    llvm::Value* counterPtr = cg.builder.CreateAlloca(cg.builder.getInt32Ty(), nullptr, "pmap_i");
    cg.builder.CreateStore(cg.builder.getInt32(0), counterPtr);
    llvm::Value* varPtr = cg.builder.CreateAlloca(elementType, nullptr, variable + "_ptr");

    // Bind loop variable in scope (once, outside loop)
    cg.symbolTable.enterScope();
    std::unique_ptr<Type> varType;
    varType = pmapElementSourceType(iterType);
    cg.symbolTable.addSymbol(variable, std::make_unique<Symbol>(
        variable, std::move(varType), varPtr, false, SymbolType::VARIABLE));

    llvm::Value* resultPtr = cg.builder.CreateAlloca(resultElementType, cg.builder.getInt32(arraySize), "pmapresult");

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(cg.context, "pmapcond", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(cg.context, "pmapbody", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(cg.context, "afterpmap", function);

    cg.builder.CreateBr(condBB);

    // Condition
    cg.builder.SetInsertPoint(condBB);
    llvm::Value* counter = cg.builder.CreateLoad(cg.builder.getInt32Ty(), counterPtr, "i");
    llvm::Value* cond = cg.builder.CreateICmpSLT(counter, cg.builder.getInt32(arraySize), "pmapcmp");
    cg.builder.CreateCondBr(cond, bodyBB, afterBB);

    // Body
    cg.builder.SetInsertPoint(bodyBB);
    llvm::Value* idx = cg.builder.CreateLoad(cg.builder.getInt32Ty(), counterPtr, "idx");

    // Load current element and store into loop variable
    llvm::Value* elemPtr = cg.builder.CreateGEP(elementType, arrayPtr, idx, "elemptr");
    llvm::Value* elemValue = cg.builder.CreateLoad(elementType, elemPtr, "elem");
    cg.builder.CreateStore(elemValue, varPtr);

    // Evaluate body
    llvm::Value* bodyResult = body->codegen();
    if (!bodyResult) {
        std::cerr << "Error: pmap body evaluation failed" << std::endl;
        cg.symbolTable.exitScope();
        return nullptr;
    }

    // Store result
    if (bodyResult->getType()->isPointerTy()) {
        bodyResult = cg.builder.CreateLoad(resultElementType, bodyResult, "pmap_body_load");
    }
    if (bodyResult->getType() != resultElementType) {
        std::cerr << "Type error: pmap body result type mismatch" << std::endl;
        cg.symbolTable.exitScope();
        return nullptr;
    }
    llvm::Value* resultElemPtr = cg.builder.CreateGEP(resultElementType, resultPtr, idx, "resultptr");
    cg.builder.CreateStore(bodyResult, resultElemPtr);

    // Increment counter
    llvm::Value* nextIdx = cg.builder.CreateAdd(idx, cg.builder.getInt32(1), "next_i");
    cg.builder.CreateStore(nextIdx, counterPtr);
    auto* latch = cg.builder.CreateBr(condBB);
    attachParallelLoopMetadata(cg.context, latch, backend);

    // After
    cg.builder.SetInsertPoint(afterBB);
    cg.symbolTable.exitScope();

    return resultPtr;
}
