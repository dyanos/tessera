// codegen.h
#pragma once

#include "symbol_table.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueHandle.h>
#include "llvm_compat.h"
#include <sstream>
#include <vector>
#include <set>
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
    llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderCallbackInserter> builder;
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
    std::set<std::string> movedNames;
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
    llvm::AllocaInst* createEntryAlloca(llvm::Type* type, const std::string& name, llvm::Value* count = nullptr);

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
    bool forgetCleanup(llvm::Value* pointer);
    bool hasCleanup(llvm::Value* pointer) const;
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
    void releaseFromCurrentScope(llvm::Value* value);
    llvm::Value* copyStackBufferToHeap(llvm::Value* value);
    llvm::Function* classDropFunction(const Type* type);
    llvm::Function* arrayDropFunction(const Type* type);
    llvm::Function* sharedPayloadDropFunction(const Type* type, llvm::Type* payloadType);
    llvm::Function* sharedStrongDropFunction();
    void trackArrayElements(llvm::Value* array, llvm::Value* count);
    void emitOwnedValueDrop(llvm::Value* value, const Type* type);
    void registerOwnedValueCleanup(llvm::Value* value, const Type* type);
    void registerStructCleanup(llvm::Value* storage, llvm::Function* release);
    void markStringFunction(const std::string& name, bool runtimeAllocated);
    llvm::Value* takeOwnedString(llvm::Value* value);
    void promoteTemporaryString(llvm::Value* value);
    void registerStringTemporary(llvm::CallInst* call);
    llvm::Value* adoptRuntimeString(llvm::Value* text);
    llvm::Value* takeOwned(llvm::Value* value, const Type* type);
    void registerOwnedTemporary(llvm::Value* value, llvm::Function* drop);
    void markOwnedResultFunction(const std::string& name, llvm::Function* drop);
    void markOwnedParameters(const std::string& name, std::vector<bool> mask);
    llvm::Function* ownedValueDropFunction(const Type* type);
    bool isRefcountedType(const Type* type);
    bool ownsParameter(const std::string& function, unsigned index) const;
    std::unordered_map<std::string, std::vector<std::unique_ptr<Type>>> declaredStructFields;
    std::vector<std::unique_ptr<Type>> structFieldTypes(const Type* type);
    bool structHoldsCounted(const Type* type, int depth = 0);
    llvm::Function* structCountFunction(const Type* type, bool retain);
    void emitStructCount(llvm::Value* storage, const Type* type, bool retain);
    llvm::Function* structHeapDropFunction(const Type* type);
    llvm::Function* rcReleaseFunction();
    llvm::Function* enumFieldReleaseFunction(const Type* type);
    void finalizeOwnershipFunctions();
    struct PendingBody { llvm::Function* function; std::unique_ptr<Type> type; bool enumRelease; };
    std::vector<PendingBody> pendingBodies;
    void emitClassDropBody(llvm::Function* fn, const Type* type);
    void emitEnumReleaseBody(llvm::Function* fn, const Type* type);
    void registerOwnedSlotCleanup(llvm::Value* slot, llvm::Function* drop);
    void registerHandleSlotCleanup(llvm::Value* slot, bool weak);
    void registerArrayElementsCleanup(llvm::Value* array, const Type* elementType, unsigned count);
    void registerStringSlotCleanup(llvm::Value* slot);
    void registerTensorSlotCleanup(llvm::Value* slot);
    void registerDictSlotCleanup(llvm::Value* slot);
    llvm::Function* dictFreeFunction();
    void dropTensorValue(llvm::Value* tensor);
    void releaseString(llvm::Value* value);
    void emitAllCleanups();
    void emitAllCleanupsExcept(llvm::Value* retainedValue);
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
    bool isTensorAllocation(llvm::Value* tensor) const;
    bool isFreshTensor(llvm::Value* tensor) const;
    static void clearFreshTensor(llvm::Value* tensor);
    void stripFreshTensorMarks();
    void elideConstantRefcounts();
    void dropTemporary(llvm::Value* tensor, llvm::Value* result = nullptr);
    llvm::Value* borrowTensors(const std::vector<llvm::Value*>& operands, llvm::Value* result);
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
    enum class CleanupKind { Free = 0, TensorFree = 1, SharedRelease = 2, WeakRelease = 3, StringSlotRelease = 4, TensorSlotDrop = 5, DictSlotFree = 6, OwnedSlotDrop = 7, SharedSlotRelease = 8, WeakSlotRelease = 9 };
    std::unordered_map<llvm::Value*, llvm::Value*> dropFlags;  // owned pointer -> its i1 drop flag
    std::unordered_map<llvm::Value*, llvm::Function*> dropFunctions;
    std::vector<std::vector<std::pair<llvm::Value*, CleanupKind>>> cleanupScopes;
    void registerOwned(llvm::Value* pointer, CleanupKind kind);
    void emitCleanupEntry(llvm::Value* pointer, CleanupKind kind, const std::unordered_set<llvm::Value*>& retained);
    void emitCleanupBody(llvm::Value* pointer, CleanupKind kind);

    struct TemporaryFrame {
        llvm::Function* function;
        size_t cleanupDepth;
        std::vector<llvm::AllocaInst*> slots;
        std::vector<llvm::AllocaInst*> stringSlots;
        std::unordered_map<llvm::Value*, llvm::AllocaInst*> strings;
    };
    int stringHookDepth = 0;
    std::unordered_set<std::string> userStringFunctions;
    std::unordered_set<std::string> runtimeStringFunctions;
    std::unordered_map<std::string, llvm::Function*> ownedResultFunctions;
public:
    std::unordered_map<std::string, llvm::Function*> structResultFunctions;
private:
    std::unordered_map<std::string, std::vector<bool>> ownedParameterMasks;
    std::unordered_map<llvm::AllocaInst*, llvm::Function*> temporaryDrops;
    std::vector<std::string> dictProducerNames;
    bool dictProducersMarked = false;
    bool transferTemporary(llvm::Value* value);
    void onInstructionInserted(llvm::Instruction* inst);
    llvm::FunctionCallee rcFunction(const char* name);
    void emitStringSlotRelease(llvm::AllocaInst* slot);
    std::vector<TemporaryFrame> temporaryFrames;
    std::vector<llvm::WeakTrackingVH> freshTensorMarks;
    llvm::Function* getTensorDropFunction();
    void emitTemporarySlotDrop(llvm::AllocaInst* slot);
};
