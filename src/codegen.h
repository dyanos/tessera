// codegen.h
#pragma once

#include "symbol_table.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "llvm_compat.h"
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Generates a template class instance registered during type checking (ClassInstanceCreationNode.cpp).
bool materializeTemplateClass(const std::string& mangledName);
// Turns applied template types written in signatures and declarations (`Opt<Int>`) into their
// instance class types (`Opt$Int`), instantiating them. Template bodies are left alone.
void normalizeAppliedTemplateTypes(ASTNode* root);
// The template and arguments an instance class (`Opt$Int`) was created from.
bool templateInstanceArguments(const std::string& instanceName, std::string* templateName,
                               std::vector<std::unique_ptr<Type>>* arguments);
// Instantiates (registers, and generates once code generation runs) template class `templateName`.
struct ClassSymbol;
ClassSymbol* instantiateTemplateClass(const std::string& templateName, const std::vector<std::unique_ptr<Type>>& arguments);
std::string templateInstanceName(const std::string& templateName, const std::vector<std::unique_ptr<Type>>& arguments);

class CodeGenerator {
public:
    static CodeGenerator& getInstance() {
        static CodeGenerator instance; // Guaranteed to be destroyed
        return instance;          // Instantiated on first use
    }

    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    llvm::Function* mainFunction; // non-owning, owned by module

	// 현재 스코프 체인: namespace 및 클래스 스코프 추적용 -> body가 시작될때 push, 끝날때 pop
    std::vector<std::string> scopes;
    std::vector<std::string> externalLinkLibraries;
    std::vector<std::string> externalLinkPaths;

    SymbolTable symbolTable;
    // 현재 namespace 또는 클래스, function(or method), block 등을 의미(상위 심볼의 정보도 검색되어야 하는 조건이 있음), nullptr이면 전역 스코프
	Symbol* currentSymbol; // non-owning, owned by SymbolTable

    // Runtime element counts for collection results whose length is not encoded in their static
    // type. `filter` produces a buffer over-allocated to the source size but valid only for the
    // surviving-element count; `map` produces a statically sized buffer. Keyed by the result
    // pointer so a downstream `preduce`/`map`/`filter` iterating that pointer can use the real
    // length instead of the (unknown or over-allocated) static size. Non-owning LLVM values.
    std::unordered_map<llvm::Value*, llvm::Value*> arrayRuntimeLength;

    // Classes whose field initializers are currently being emitted. A class that eagerly
    // constructs its own type in a field default (`var next: Node = new Node(0)`) would recurse
    // forever; this set lets construction detect the cycle and emit a diagnostic instead of
    // overflowing the compiler's stack.
    std::unordered_set<std::string> classesUnderConstruction;
    // False while the program is type-checked, true once code generation starts. Template classes
    // used during type checking are only registered then and generated when this becomes true.
    bool generatingCode = false;

    llvm::LLVMContext& getContext() { return context; }

    // A code-generation error that must fail the compilation (printed now, counted for main).
    int errorCount = 0;
    void reportError(const std::string& message);
    // reportError of the concatenated parts (strings, numbers, ...).
    template <class... Parts>
    void error(const Parts&... parts) {
        std::ostringstream text;
        (text << ... << parts);
        reportError(text.str());
    }
    // A stack slot in the entry block of the current function, so code in a loop does not allocate
    // a new one on every iteration.
    llvm::AllocaInst* createEntryAlloca(llvm::Type* type, const std::string& name);

    void addExternalLinkLibrary(const std::string& library);
    void addExternalLinkPath(const std::string& path);
    void requireSystemNative();

    void dumpIR();
    llvm::Type* getLLVMType(const Type* type);
    void enterCleanupScope();
    void exitCleanupScope();
    void registerCleanup(llvm::Value* pointer);
    // Tensor storage consists of the header plus separately allocated dims/data buffers.
    void registerTensorCleanup(llvm::Value* tensor);
    // A manual free consumes a local allocation; remove its pending lexical drop.
    void forgetCleanup(llvm::Value* pointer);
    // A Shared<T> control block: at scope exit its strong count is decremented; the block is freed
    // only when both strong and weak reach zero (reverse-order, like every other cleanup).
    void registerSharedCleanup(llvm::Value* controlBlock);
    // A Weak<T> handle: decrements the weak count; frees the block when strong and weak are both zero.
    void registerWeakCleanup(llvm::Value* controlBlock);
    // An owned class instance dropped at scope exit unless it was moved away first. Its drop flag is
    // cleared by noteMoved (a `<-` move or `free`), so a value moved on some paths only is freed
    // exactly once.
    void registerFlaggedCleanup(llvm::Value* pointer);
    void noteMoved(llvm::Value* pointer);
    bool hasDropFlag(llvm::Value* pointer) const { return dropFlagInCurrentFunction(pointer) != nullptr; }
    llvm::Value* dropFlagInCurrentFunction(llvm::Value* pointer) const;
    void emitCurrentScopeCleanups();
    void emitAllCleanups();
    void emitAllCleanupsExcept(llvm::Value* retainedPointer);
    // Number of open cleanup scopes, and the drops of every scope opened at or above `depth`
    // (innermost first, entries kept) -- used by break/continue to leave a loop body early.
    size_t cleanupDepth() const { return cleanupScopes.size(); }
    void emitCleanupsAbove(size_t depth);

    // Enclosing loops of the code being generated: where `break` and `continue` jump.
    struct LoopTarget {
        llvm::BasicBlock* breakBB;
        llvm::BasicBlock* continueBB;
        size_t cleanupDepth;
    };
    std::vector<LoopTarget> loopTargets;

    void markFreshTensor(llvm::Value* tensor);
    bool isFreshTensor(llvm::Value* tensor) const;
    static void clearFreshTensor(llvm::Value* tensor);
    void stripFreshTensorMarks();
    void dropTemporary(llvm::Value* tensor, llvm::Value* result = nullptr);
    void enterTemporaryFrame();
    void exitTemporaryFrame();
    void emitTemporaryDropsAbove(size_t depth);

    llvm::Function* mallocFunction; // non-owning, owned by module
    llvm::Function* freeFunction;  // non-owning, owned by module

private:
    CodeGenerator();
    // Each cleanup is a (pointer, kind) pair: Free drops a single heap allocation, TensorFree also
    // frees the tensor's separate dims/data buffers, and SharedRelease/WeakRelease decrement a
    // Shared/Weak control block (freeing it only when both counts reach zero).
    enum class CleanupKind { Free = 0, TensorFree = 1, SharedRelease = 2, WeakRelease = 3, FlaggedFree = 4 };
    std::unordered_map<llvm::Value*, llvm::Value*> dropFlags;  // owned pointer -> its i1 drop flag
    std::vector<std::vector<std::pair<llvm::Value*, CleanupKind>>> cleanupScopes;
    void emitCleanupEntry(llvm::Value* pointer, CleanupKind kind, llvm::Value* retainedPointer);

    struct TemporaryFrame {
        llvm::Function* function;
        size_t cleanupDepth;
        std::vector<llvm::AllocaInst*> slots;
    };
    std::vector<TemporaryFrame> temporaryFrames;
    llvm::Function* getTensorDropFunction();
    void emitTemporarySlotDrop(llvm::AllocaInst* slot);
};
