#include "codegen.h"

#include "MatchExpressionNode.h"
#include "ASTVisitor.h"
#include "UnitNode.h"
#include "ValueNode.h"
#include "AccessFieldNode.h"
#include "BinaryExpressionNode.h"
#include "IdentifierNode.h"
#include "VariableDeclarationNode.h"
#include "enum_nodes.h"
#include "enums.h"
#include "type_utils.h"

#include <functional>
#include <iostream>
#include <sstream>

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

namespace {
llvm::Function* getOrCreateStringMatchRuntime(
    CodeGenerator& cg,
    const std::string& name,
    llvm::Type* i32Ty,
    llvm::Type* i8PtrTy) {
    cg.requireSystemNative();
    if (auto* function = cg.module->getFunction(name)) {
        return function;
    }

    return llvm::Function::Create(
        llvm::FunctionType::get(i32Ty, { i8PtrTy, i8PtrTy }, false),
        llvm::Function::ExternalLinkage,
        name,
        cg.module.get());
}

llvm::Value* asCStringPointer(CodeGenerator& cg, llvm::Value* value) {
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    if (!value) {
        return llvm::ConstantPointerNull::get(i8PtrTy);
    }
    if (value->getType() == i8PtrTy) {
        return value;
    }
    if (value->getType()->isPointerTy()) {
        return cg.builder.CreateBitCast(value, i8PtrTy, "match.str.ptr");
    }
    return llvm::ConstantPointerNull::get(i8PtrTy);
}
}

void MatchExpressionNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

namespace {
using ValueRef = std::function<std::unique_ptr<ASTNode>()>;

llvm::Value* toCondition(CodeGenerator& cg, llvm::Value* value) {
    if (!value) return nullptr;
    if (value->getType()->isIntegerTy(1)) return value;
    if (value->getType()->isIntegerTy()) {
        return cg.builder.CreateICmpNE(value, llvm::ConstantInt::get(value->getType(), 0), "match.bool");
    }
    return nullptr;
}

ValueRef fieldOf(const ValueRef& base, const std::string& field) {
    return [base, field]() -> std::unique_ptr<ASTNode> {
        return std::make_unique<AccessFieldNode>(base(), std::make_unique<IdentifierNode>(field));
    };
}

// Emits the test of `pattern` against the value `ref` denotes. Branches to `fail` on a mismatch and
// leaves the insertion point in the block reached on success.
bool emitPatternTest(CodeGenerator& cg, PatternNode& pattern, const ValueRef& ref, llvm::BasicBlock* fail) {
    llvm::Function* function = cg.builder.GetInsertBlock()->getParent();
    auto branchOn = [&](llvm::Value* condition) {
        auto* next = llvm::BasicBlock::Create(cg.context, "match.test", function);
        cg.builder.CreateCondBr(condition, next, fail);
        cg.builder.SetInsertPoint(next);
    };
    switch (pattern.kind) {
        case PatternNode::Kind::Wildcard:
        case PatternNode::Kind::Binding:
            return true;
        case PatternNode::Kind::Literal: {
            auto* literal = dynamic_cast<ValueNode*>(pattern.literal.get());
            if (literal && literal->valueType == TokenType::REGEX_LITERAL) {
                auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
                auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
                llvm::Value* subject = ref()->codegen();
                llvm::Value* regex = literal->codegen();
                if (!subject || !regex) return false;
                cg.requireSystemNative();
                auto fn = cg.module->getOrInsertFunction("csec_string_regex_match", llvm::FunctionType::get(i32Ty, {i8PtrTy, i8PtrTy}, false));
                llvm::Value* raw = cg.builder.CreateCall(fn, {subject, regex}, "match.regex");
                branchOn(cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Ty, 0), "match.cond"));
                return true;
            }
            BinaryExpressionNode equal(ref(), "==", pattern.literal->clone());
            llvm::Value* condition = toCondition(cg, equal.codegen());
            if (!condition) {
                CodeGenerator::getInstance().error("cannot compare the matched value with a pattern");
                return false;
            }
            branchOn(condition);
            return true;
        }
        case PatternNode::Kind::Variant: {
            AccessFieldNode tagAccess(ref(), std::make_unique<IdentifierNode>(enumTagField()));
            llvm::Value* tag = tagAccess.codegen();
            if (!tag) return false;
            branchOn(cg.builder.CreateICmpEQ(tag, llvm::ConstantInt::get(tag->getType(), pattern.tag), "match.variant"));
            for (size_t k = 0; k < pattern.subpatterns.size() && k < pattern.fieldNames.size(); ++k) {
                if (!pattern.subpatterns[k] || pattern.subpatterns[k]->isIrrefutable()) continue;
                if (!emitPatternTest(cg, *pattern.subpatterns[k], fieldOf(ref, pattern.fieldNames[k]), fail)) return false;
            }
            return true;
        }
    }
    return true;
}

// Declares the variables `pattern` binds, reading them from the value `ref` denotes.
bool emitPatternBindings(PatternNode& pattern, const ValueRef& ref) {
    if (pattern.kind == PatternNode::Kind::Binding) {
        VariableDeclarationNode binding(pattern.name, pattern.valueType ? pattern.valueType->clone() : std::make_unique<UnknownType>(),
                                        ref(), false);
        binding.hasExplicitType = pattern.valueType && pattern.valueType->getKind() != Type::Kind::UNKNOWN;
        binding.borrowsValue = true;
        binding.codegen();
        return true;
    }
    if (pattern.kind == PatternNode::Kind::Variant) {
        for (size_t k = 0; k < pattern.subpatterns.size() && k < pattern.fieldNames.size(); ++k) {
            if (pattern.subpatterns[k] && !emitPatternBindings(*pattern.subpatterns[k], fieldOf(ref, pattern.fieldNames[k]))) {
                return false;
            }
        }
    }
    return true;
}
}  // namespace

// Matching with resolved patterns: the value is bound once to a hidden local; each case tests its
// patterns in turn (a variant's tag, then its sub-patterns), binds its variables, checks its guard
// and evaluates its result. A value no case matches stops the program.
llvm::Value* MatchExpressionNode::codegenResolved() {
    auto& cg = CodeGenerator::getInstance();
    static int counter = 0;
    auto valueType = expression ? stripBorrow(expression->getType()) : nullptr;
    if (!valueType || valueType->getKind() == Type::Kind::UNKNOWN) {
        CodeGenerator::getInstance().error("cannot determine the type of the matched value");
        return nullptr;
    }
    const std::string name = "__match_" + std::to_string(counter++);
    VariableDeclarationNode temporary(name, valueType->clone(), std::move(expression), false);
    temporary.hasExplicitType = true;
    temporary.codegen();
    expression = std::move(temporary.initializer);
    const ValueRef ref = [name]() -> std::unique_ptr<ASTNode> { return std::make_unique<IdentifierNode>(name); };

    llvm::Function* function = cg.builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(cg.context, "match.end", function);
    llvm::Type* resultLLVMType = nullptr;
    if (type && type->getKind() != Type::Kind::UNKNOWN && !type->isVoidTy() && type->getName() != "Unit") {
        resultLLVMType = getABIStorageType(type.get());
        if (resultLLVMType && resultLLVMType->isVoidTy()) resultLLVMType = nullptr;
    }
    std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>> incoming;

    for (size_t i = 0; i < cases.size(); ++i) {
        auto* first = dynamic_cast<PatternNode*>(cases[i].first.get());
        if (!first) return nullptr;
        llvm::BasicBlock* nextCase = llvm::BasicBlock::Create(cg.context, "match.next", function);
        llvm::BasicBlock* matched = llvm::BasicBlock::Create(cg.context, "match.case", function);
        std::vector<PatternNode*> patterns{first};
        for (auto& alternative : alternatives[i]) patterns.push_back(static_cast<PatternNode*>(alternative.get()));
        for (size_t a = 0; a < patterns.size(); ++a) {
            const bool last = a + 1 == patterns.size();
            llvm::BasicBlock* altFail = last ? nextCase : llvm::BasicBlock::Create(cg.context, "match.alt", function);
            if (!emitPatternTest(cg, *patterns[a], ref, altFail)) return nullptr;
            cg.builder.CreateBr(matched);
            if (!last) cg.builder.SetInsertPoint(altFail);
        }

        cg.builder.SetInsertPoint(matched);
        cg.symbolTable.enterScope();
        // Bindings alias parts of the matched value, which keeps ownership of them: no drops here.
        cg.enterCleanupScope();
        if (!emitPatternBindings(*first, ref)) return nullptr;
        cg.exitCleanupScope();
        if (guards[i]) {
            llvm::Value* condition = toCondition(cg, guards[i]->codegen());
            if (!condition) {
                CodeGenerator::getInstance().error("a match guard must be a Boolean");
                cg.symbolTable.exitScope();
                return nullptr;
            }
            auto* guarded = llvm::BasicBlock::Create(cg.context, "match.guarded", function);
            cg.builder.CreateCondBr(condition, guarded, nextCase);
            cg.builder.SetInsertPoint(guarded);
        }
        llvm::Value* result = cases[i].second ? cases[i].second->codegen() : nullptr;
        if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
            if (resultLLVMType) {
                result = coerceValueToLLVMType(result, resultLLVMType);
                if (!result || result->getType() != resultLLVMType) {
                    CodeGenerator::getInstance().error("match case result has an unexpected type");
                    cg.symbolTable.exitScope();
                    return nullptr;
                }
                incoming.push_back({result, cg.builder.GetInsertBlock()});
            }
            cg.builder.CreateBr(mergeBB);
        }
        cg.symbolTable.exitScope();
        cg.builder.SetInsertPoint(nextCase);
    }

    // No case matched.
    cg.requireSystemNative();
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto failFn = cg.module->getOrInsertFunction("csec_assert_fail",
        llvm::FunctionType::get(llvm::Type::getVoidTy(cg.context), {i8PtrTy, i8PtrTy}, false));
    cg.builder.CreateCall(failFn, {llvm::ConstantPointerNull::get(i8PtrTy),
                                   cg.builder.CreateGlobalString("no case of the match matched the value")});
    cg.builder.CreateUnreachable();

    cg.builder.SetInsertPoint(mergeBB);
    if (!resultLLVMType) return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(cg.context));
    if (incoming.empty()) {
        cg.builder.CreateUnreachable();
        cg.builder.SetInsertPoint(llvm::BasicBlock::Create(cg.context, "match.dead", function));
        return llvm::UndefValue::get(resultLLVMType);
    }
    auto* phi = cg.builder.CreatePHI(resultLLVMType, static_cast<unsigned>(incoming.size()), "match.result");
    for (const auto& in : incoming) phi->addIncoming(in.first, in.second);
    return phi;
}

llvm::Value* MatchExpressionNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    if (patternsResolved) {
        return codegenResolved();
    }

    llvm::Value* matchValue = expression ? expression->codegen() : nullptr;
    if (!matchValue) {
        return nullptr;
    }

    auto* currentBlock = cg.builder.GetInsertBlock();
    if (!currentBlock) {
        return nullptr;
    }

    llvm::Function* function = currentBlock->getParent();
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(cg.context, "match.end", function);

    llvm::Type* resultLLVMType = nullptr;
    if (type && type->getKind() != Type::Kind::UNKNOWN) {
        resultLLVMType = cg.getLLVMType(type.get());
    }

    llvm::PHINode* phi = nullptr;
    if (resultLLVMType && !resultLLVMType->isVoidTy()) {
        phi = llvm::PHINode::Create(resultLLVMType, static_cast<unsigned>(cases.size()), "match.result", mergeBB);
    }

    llvm::BasicBlock* nextTestBB = currentBlock;

    for (size_t i = 0; i < cases.size(); ++i) {
        auto& casePair = cases[i];
        auto* caseBB = llvm::BasicBlock::Create(cg.context, "match.case", function);
        const bool isWildcard = dynamic_cast<UnitNode*>(casePair.first.get()) != nullptr;
        llvm::BasicBlock* nextBB = (i + 1 < cases.size())
            ? llvm::BasicBlock::Create(cg.context, "match.next", function)
            : mergeBB;

        cg.builder.SetInsertPoint(nextTestBB);
        if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
            if (isWildcard) {
                cg.builder.CreateBr(caseBB);
            }
            else {
                llvm::Value* caseValue = casePair.first ? casePair.first->codegen() : nullptr;
                if (!caseValue) {
                    return nullptr;
                }

                llvm::Value* condition = nullptr;
                auto* patternValue = dynamic_cast<ValueNode*>(casePair.first.get());
                const bool isStringPattern = patternValue &&
                    (patternValue->valueType == TokenType::STRING_LITERAL ||
                     patternValue->valueType == TokenType::REGEX_LITERAL);
                if (isStringPattern) {
                    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
                    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
                    const char* runtimeName = patternValue->valueType == TokenType::REGEX_LITERAL
                        ? "csec_string_regex_match"
                        : "csec_string_equals";
                    llvm::Value* raw = cg.builder.CreateCall(
                        getOrCreateStringMatchRuntime(cg, runtimeName, i32Ty, i8PtrTy),
                        { asCStringPointer(cg, matchValue), asCStringPointer(cg, caseValue) },
                        "match.str.raw");
                    condition = cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Ty, 0), "match.cond");
                }
                else {
                    condition = cg.builder.CreateICmpEQ(matchValue, caseValue, "match.cond");
                }
                cg.builder.CreateCondBr(condition, caseBB, nextBB);
            }
        }

        cg.builder.SetInsertPoint(caseBB);
        llvm::Value* caseResult = casePair.second ? casePair.second->codegen() : nullptr;
        if (!caseResult && phi) {
            return nullptr;
        }

        auto* finishedCaseBB = cg.builder.GetInsertBlock();
        if (!csec_llvm::terminatorOrNull(finishedCaseBB)) {
            if (phi) {
                phi->addIncoming(caseResult, finishedCaseBB);
            }
            cg.builder.CreateBr(mergeBB);
        }

        nextTestBB = nextBB;
    }

    cg.builder.SetInsertPoint(mergeBB);
    if (phi) {
        return phi;
    }

    return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(cg.context));
}
