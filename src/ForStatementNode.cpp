#include "codegen.h"

#include "ForStatementNode.h"
#include "ASTVisitor.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>

#include "RangeExpressionNode.h"
#include "ArrayLiteralNode.h"
#include "dict_support.h"
#include "symbol.h"

#include <iostream>

namespace {
llvm::Value* coerceRangeBound(llvm::Value* value, llvm::Type* targetType) {
    if (!value || !targetType) {
        return value;
    }

    auto& cg = CodeGenerator::getInstance();
    llvm::Type* sourceType = value->getType();
    if (sourceType == targetType) {
        return value;
    }

    if (sourceType->isIntegerTy() && targetType->isIntegerTy()) {
        unsigned srcBits = sourceType->getIntegerBitWidth();
        unsigned dstBits = targetType->getIntegerBitWidth();
        if (srcBits < dstBits) {
            return cg.builder.CreateSExt(value, targetType, "range.sext");
        }
        return cg.builder.CreateTrunc(value, targetType, "range.trunc");
    }

    if (sourceType->isFloatingPointTy() && targetType->isIntegerTy()) {
        return cg.builder.CreateFPToSI(value, targetType, "range.fptosi");
    }

    if (sourceType->isIntegerTy() && targetType->isFloatingPointTy()) {
        return cg.builder.CreateSIToFP(value, targetType, "range.sitofp");
    }

    return value;
}

// Bind a loop variable of `type` to `value`: arrays and closures are bound to their pointer, every
// other value to a fresh slot (so identifier reads load it like any local).
void bindLoopVariable(const std::string& name, const Type* type, llvm::Value* value) {
    auto& cg = CodeGenerator::getInstance();
    llvm::Value* bound = value;
    const bool direct = type->getKind() == Type::Kind::ARRAY || type->getKind() == Type::Kind::FUNCTION ||
        (type->getKind() == Type::Kind::GENERIC && (type->getName() == "Array" || type->getName() == "Vector"));
    if (!direct) {
        auto* slot = cg.builder.CreateAlloca(value->getType(), nullptr, name + "_ptr");
        cg.builder.CreateStore(value, slot);
        bound = slot;
    }
    auto boundType = cloneType(type);
    cg.symbolTable.addSymbol(name, std::make_unique<Symbol>(name, std::move(boundType), bound, false, SymbolType::VARIABLE));
}
}

// `for (k <- d) { ... }` visits the keys of a dictionary in insertion order; `for (k, v <- d)` also
// binds each value. The size is re-read every iteration, so removing the current entry ends early
// rather than reading past the end.
llvm::Value* ForStatementNode::codegenDictLoop() {
    auto& cg = CodeGenerator::getInstance();
    auto dictType = iterableExpr->getType();
    // A set iterates its elements, which are the keys of the underlying dictionary.
    const bool isSet = isSetType(dictType.get());
    const Type* keyType = isSet ? setElementType(dictType.get()) : dictKeyType(dictType.get());
    const Type* valueType = isSet ? keyType : dictValueType(dictType.get());
    llvm::Value* handle = codegenDictHandle(iterableExpr.get());
    if (!handle || !keyType || !valueType) return nullptr;

    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* ptrTy = llvm::PointerType::getUnqual(cg.context);
    auto runtime = [&](const char* name, llvm::Type* ret, std::vector<llvm::Type*> params) {
        if (auto* fn = cg.module->getFunction(name)) return fn;
        return llvm::Function::Create(llvm::FunctionType::get(ret, params, false),
                                      llvm::Function::ExternalLinkage, name, cg.module.get());
    };

    llvm::Function* function = cg.builder.GetInsertBlock()->getParent();
    auto* indexSlot = cg.builder.CreateAlloca(i64Ty, nullptr, "dict.iter.index");
    cg.builder.CreateStore(llvm::ConstantInt::get(i64Ty, 0), indexSlot);

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(cg.context, "dict.iter.cond", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(cg.context, "dict.iter.body", function);
    llvm::BasicBlock* incBB = llvm::BasicBlock::Create(cg.context, "dict.iter.inc", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(cg.context, "dict.iter.end", function);
    cg.builder.CreateBr(condBB);

    cg.builder.SetInsertPoint(condBB);
    llvm::Value* index = cg.builder.CreateLoad(i64Ty, indexSlot, "dict.iter.i");
    llvm::Value* size = cg.builder.CreateCall(runtime("csec_dict_size", i64Ty, {ptrTy}), {handle}, "dict.iter.size");
    cg.builder.CreateCondBr(cg.builder.CreateICmpSLT(index, size, "dict.iter.more"), bodyBB, afterBB);

    cg.builder.SetInsertPoint(bodyBB);
    cg.symbolTable.enterScope();
    const std::string keyName = (tokenValue.empty() || isSet) ? variable : tokenValue;
    llvm::Value* keyPayload = cg.builder.CreateCall(runtime("csec_dict_key_at", i64Ty, {ptrTy, i64Ty}), {handle, index}, "dict.iter.key");
    llvm::Value* key = dictValueFromPayload(keyPayload, keyType);
    if (!key) {
        cg.symbolTable.exitScope();
        return nullptr;
    }
    bindLoopVariable(keyName, keyType, key);
    if (!tokenValue.empty() && !isSet) {
        llvm::Value* valuePayload = cg.builder.CreateCall(runtime("csec_dict_value_at", i64Ty, {ptrTy, i64Ty}), {handle, index}, "dict.iter.value");
        llvm::Value* value = dictValueFromPayload(valuePayload, valueType);
        if (!value) {
            cg.symbolTable.exitScope();
            return nullptr;
        }
        bindLoopVariable(variable, valueType, value);
    }
    cg.loopTargets.push_back({afterBB, incBB, cg.cleanupDepth()});
    body->codegen();
    cg.loopTargets.pop_back();
    if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
        cg.builder.CreateBr(incBB);
    }
    cg.builder.SetInsertPoint(incBB);
    llvm::Value* current = cg.builder.CreateLoad(i64Ty, indexSlot, "dict.iter.i");
    cg.builder.CreateStore(cg.builder.CreateAdd(current, llvm::ConstantInt::get(i64Ty, 1), "dict.iter.next"), indexSlot);
    cg.builder.CreateBr(condBB);
    cg.symbolTable.exitScope();
    cg.builder.SetInsertPoint(afterBB);
    return nullptr;
}


// `for (x <- xs) { ... }` over an array: a literal, a statically sized array, or a `map`/`filter`
// result whose element count was recorded at runtime.
llvm::Value* ForStatementNode::codegenArrayLoop() {
    auto& cg = CodeGenerator::getInstance();
    auto iterType = iterableExpr->getType();
    llvm::Value* arrayPtr = iterableExpr->codegen();
    if (!arrayPtr || !iterType) return nullptr;
    llvm::Value* srcPtr = arrayPtr;

    std::unique_ptr<Type> elementSourceType;
    llvm::Type* elementType = nullptr;
    int arraySize = 0;
    bool sizeKnown = false;
    if (auto* arrType = dynamic_cast<ArrayType*>(iterType.get())) {
        if (arrType->elementType) elementSourceType = arrType->elementType->clone();
        arraySize = arrType->size;
        sizeKnown = arraySize > 0;
    }
    else if (auto* genType = dynamic_cast<GenericType*>(iterType.get())) {
        const std::string base = genType->baseType ? genType->baseType->getName() : "";
        if ((base == "Array" || base == "Vector") && !genType->typeArguments.empty() && genType->typeArguments[0]) {
            elementSourceType = genType->typeArguments[0]->clone();
        }
    }
    if (auto* literal = dynamic_cast<ArrayLiteralNode*>(iterableExpr.get())) {
        arraySize = static_cast<int>(literal->elements.size());
        sizeKnown = true;
    }
    if (elementSourceType) elementType = cg.getLLVMType(elementSourceType.get());

    if (auto* llvmArrayTy = llvm::dyn_cast<llvm::ArrayType>(arrayPtr->getType())) {
        if (!elementType) elementType = llvmArrayTy->getElementType();
        if (!sizeKnown) {
            arraySize = static_cast<int>(llvmArrayTy->getNumElements());
            sizeKnown = true;
        }
        llvm::Value* tmpArray = cg.builder.CreateAlloca(llvmArrayTy, nullptr, "for.arr.tmp");
        cg.builder.CreateStore(arrayPtr, tmpArray);
        llvm::Value* zero = cg.builder.getInt32(0);
        arrayPtr = cg.builder.CreateInBoundsGEP(llvmArrayTy, tmpArray, { zero, zero }, "for.arr.decay");
    }

    llvm::Value* runtimeLen = nullptr;
    auto lenIt = cg.arrayRuntimeLength.find(srcPtr);
    if (lenIt != cg.arrayRuntimeLength.end()) runtimeLen = lenIt->second;

    if (!elementType || !elementSourceType || !arrayPtr->getType()->isPointerTy() || (!sizeKnown && !runtimeLen)) {
        std::cerr << "Error: for (" << variable << " <- ...) needs an array whose length is known here "
                  << "(a literal, a fixed-size array, or a map/filter result); iterate an index range instead"
                  << std::endl;
        return nullptr;
    }

    auto* i32Ty = cg.builder.getInt32Ty();
    llvm::Value* bound = runtimeLen ? runtimeLen : llvm::ConstantInt::get(i32Ty, arraySize);
    if (bound->getType() != i32Ty) bound = cg.builder.CreateIntCast(bound, i32Ty, true, "for.len");

    llvm::Function* function = cg.builder.GetInsertBlock()->getParent();
    llvm::Value* indexSlot = cg.builder.CreateAlloca(i32Ty, nullptr, "for.index");
    cg.builder.CreateStore(llvm::ConstantInt::get(i32Ty, 0), indexSlot);

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(cg.context, "for.each.cond", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(cg.context, "for.each.body", function);
    llvm::BasicBlock* incBB = llvm::BasicBlock::Create(cg.context, "for.each.inc", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(cg.context, "for.each.end", function);
    cg.builder.CreateBr(condBB);

    cg.builder.SetInsertPoint(condBB);
    llvm::Value* index = cg.builder.CreateLoad(i32Ty, indexSlot, "for.i");
    cg.builder.CreateCondBr(cg.builder.CreateICmpSLT(index, bound, "for.more"), bodyBB, afterBB);

    cg.builder.SetInsertPoint(bodyBB);
    cg.symbolTable.enterScope();
    llvm::Value* elementPtr = cg.builder.CreateInBoundsGEP(elementType, arrayPtr, index, "for.elem.ptr");
    llvm::Value* element = cg.builder.CreateLoad(elementType, elementPtr, variable);
    bindLoopVariable(variable, elementSourceType.get(), element);
    cg.loopTargets.push_back({afterBB, incBB, cg.cleanupDepth()});
    body->codegen();
    cg.loopTargets.pop_back();
    if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
        cg.builder.CreateBr(incBB);
    }
    cg.symbolTable.exitScope();

    cg.builder.SetInsertPoint(incBB);
    llvm::Value* current = cg.builder.CreateLoad(i32Ty, indexSlot, "for.i");
    cg.builder.CreateStore(cg.builder.CreateAdd(current, llvm::ConstantInt::get(i32Ty, 1), "for.next"), indexSlot);
    cg.builder.CreateBr(condBB);

    cg.builder.SetInsertPoint(afterBB);
    return nullptr;
}

// ForStatementNode
void ForStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* ForStatementNode::codegen() {
    auto& cg = CodeGenerator::getInstance();

    if (!isRange && !tokenStream && iterableExpr) {
        auto iterableType = iterableExpr->getType();
        if (isDictType(iterableType.get()) || isSetType(iterableType.get())) {
            return codegenDictLoop();
        }
        return codegenArrayLoop();
    }

    llvm::Function* function = cg.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* beforeLoopBB = cg.builder.GetInsertBlock();

    llvm::Value* startValue;
    llvm::Value* endValue;

    llvm::Value* value_ptr = nullptr;

    cg.symbolTable.enterScope();

    if (this->isRange) {
        auto* rangeExpr = dynamic_cast<RangeExpressionNode*>(this->iterableExpr.get());
        if (!rangeExpr) {
            cg.symbolTable.exitScope();
            return nullptr;
        }
        startValue = rangeExpr->startExpr->codegen();
        endValue = rangeExpr->endExpr->codegen();
        if (!startValue || !endValue) {
            cg.symbolTable.exitScope();
            return nullptr;
        }

        llvm::Type* rangeValueType = startValue->getType()->isIntegerTy()
            ? startValue->getType()
            : llvm::Type::getInt32Ty(cg.context);
        startValue = coerceRangeBound(startValue, rangeValueType);
        endValue = coerceRangeBound(endValue, rangeValueType);

        if (!rangeExpr->isInclusive) {
            endValue = cg.builder.CreateSub(endValue, llvm::ConstantInt::get(rangeValueType, 1, true), "untilEnd");
        }

        value_ptr = cg.builder.CreateAlloca(rangeValueType, nullptr, this->variable + "_ptr");
        cg.builder.CreateStore(startValue, value_ptr);

        auto rangeType = rangeExpr->startExpr->getType();
        cg.symbolTable.addSymbol(
            this->variable,
            std::make_unique<Symbol>(
                this->variable,
                std::move(rangeType),
                value_ptr,
                false,
                SymbolType::VARIABLE));
    }
    else {
        std::cerr << "Error: non-range for-each loops are not yet implemented." << std::endl;
        cg.symbolTable.exitScope();
        return nullptr;
    }

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(cg.context, "forcond", function);
    llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(cg.context, "loop", function);
    llvm::BasicBlock* loopBodyBB = tokenStream ? llvm::BasicBlock::Create(cg.context, "token.body", function) : loopBB;
    llvm::BasicBlock* incrementBB = llvm::BasicBlock::Create(cg.context, tokenStream ? "token.next" : "for.inc", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(cg.context, "afterloop", function);

    if (beforeLoopBB && !csec_llvm::terminatorOrNull(beforeLoopBB)) {
        cg.builder.CreateBr(condBB);
    }

    // Condition block: check i <= endValue before each iteration
    cg.builder.SetInsertPoint(condBB);
    auto* condValue = cg.builder.CreateLoad(startValue->getType(), value_ptr, this->variable.c_str());
    if (!condValue->getType()->isIntegerTy()) {
        std::cerr << "Type error: range bounds in for-loop must be the same integer type" << std::endl;
        cg.symbolTable.exitScope();
        return nullptr;
    }
    auto* cond = cg.builder.CreateICmpSLE(condValue, endValue, "cond");
    cg.builder.CreateCondBr(cond, loopBB, afterBB);

    // Loop body
    cg.builder.SetInsertPoint(loopBB);
    if (tokenStream) {
        auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
        auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());
        auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
        llvm::Value* stream = tokenStream->codegen();
        if (!stream || !stream->getType()->isPointerTy()) {
            cg.symbolTable.exitScope();
            return nullptr;
        }
        llvm::Value* ordinal = cg.builder.CreateLoad(startValue->getType(), value_ptr, variable + ".ordinal");
        if (ordinal->getType() != i32Ty) ordinal = cg.builder.CreateIntCast(ordinal, i32Ty, true, "token.ordinal.i32");
        if (skipTrivia) {
            auto kindFn = cg.module->getOrInsertFunction("csec_token_kind_at", llvm::FunctionType::get(i8Ty, {i8PtrTy, i32Ty}, false));
            llvm::Value* kind = cg.builder.CreateCall(kindFn, {stream, ordinal}, "token.kind");
            llvm::Value* isComment = cg.builder.CreateICmpEQ(kind, llvm::ConstantInt::get(i8Ty, 'M'));
            cg.builder.CreateCondBr(isComment, incrementBB, loopBodyBB);
        }
        else {
            cg.builder.CreateBr(loopBodyBB);
        }
        cg.builder.SetInsertPoint(loopBodyBB);
        auto textFn = cg.module->getOrInsertFunction("csec_token_text_at", llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i32Ty}, false));
        llvm::Value* text = cg.builder.CreateCall(textFn, {stream, ordinal}, tokenValue + ".text");
        auto* textSlot = cg.builder.CreateAlloca(i8PtrTy, nullptr, tokenValue + "_ptr");
        cg.builder.CreateStore(text, textSlot);
        cg.symbolTable.addSymbol(tokenValue, std::make_unique<Symbol>(tokenValue, std::make_unique<BasicType>("String"), textSlot, false, SymbolType::VARIABLE));
    }
    cg.loopTargets.push_back({afterBB, incrementBB, cg.cleanupDepth()});
    this->body->codegen();
    cg.loopTargets.pop_back();

    // Increment and branch back to condition (`continue` jumps to the increment too).
    if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
        cg.builder.CreateBr(incrementBB);
    }
    cg.builder.SetInsertPoint(incrementBB);
    {
        auto* value = cg.builder.CreateLoad(startValue->getType(), value_ptr, this->variable.c_str());
        auto* one = llvm::ConstantInt::get(startValue->getType(), 1, true);
        cg.builder.CreateStore(cg.builder.CreateAdd(value, one, "next_i"), value_ptr);
        cg.builder.CreateBr(condBB);
    }

    cg.builder.SetInsertPoint(afterBB);
    cg.symbolTable.exitScope();

    return nullptr;
}

