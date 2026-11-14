// codegen.h
#pragma once

#include "symbol_table.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "llvm_compat.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Generates a template class instance registered during type checking (ClassInstanceCreationNode.cpp).
bool materializeTemplateClass(const std::string& mangledName);

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
    void emitCurrentScopeCleanups();
    void emitAllCleanups();
    void emitAllCleanupsExcept(llvm::Value* retainedPointer);

    llvm::Function* mallocFunction; // non-owning, owned by module
    llvm::Function* freeFunction;  // non-owning, owned by module

private:
    CodeGenerator();
    // Each cleanup is a (pointer, kind) pair: Free drops a single heap allocation, TensorFree also
    // frees the tensor's separate dims/data buffers, and SharedRelease/WeakRelease decrement a
    // Shared/Weak control block (freeing it only when both counts reach zero).
    enum class CleanupKind { Free = 0, TensorFree = 1, SharedRelease = 2, WeakRelease = 3 };
    std::vector<std::vector<std::pair<llvm::Value*, CleanupKind>>> cleanupScopes;
    void emitCleanupEntry(llvm::Value* pointer, CleanupKind kind, llvm::Value* retainedPointer);
};
