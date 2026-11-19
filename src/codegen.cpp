// codegen.cpp
#include "codegen.h"
#include "TensorRuntime.h"
#include "type_utils.h"
#include "interfaces.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <algorithm>

#include "ProgramNode.h"


namespace {
void addUnique(std::vector<std::string>& values, const std::string& value) {
    if (value.empty()) return;
    for (const auto& existing : values) {
        if (existing == value) return;
    }
    values.push_back(value);
}
}

CodeGenerator::CodeGenerator() : builder(context) {
    this->module = std::make_unique<llvm::Module>("main", context);

    auto* i8PtrTy = llvm::PointerType::getUnqual(context);
    auto* argvTy = llvm::PointerType::getUnqual(i8PtrTy->getContext());
    llvm::FunctionType* funcType = llvm::FunctionType::get(this->builder.getInt32Ty(), {this->builder.getInt32Ty(), argvTy}, false);
    this->mainFunction = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module.get());

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", mainFunction);
    this->builder.SetInsertPoint(entry);
    this->builder.CreateRet(this->builder.getInt32(0));

    this->mallocFunction = module->getFunction("malloc");
    if (!mallocFunction) {
        mallocFunction = llvm::Function::Create(
            llvm::FunctionType::get(llvm::PointerType::getUnqual(context), { llvm::Type::getInt64Ty(context) }, false),
            llvm::Function::ExternalLinkage,
            "malloc",
            module.get()
        );
    }

    this->freeFunction = module->getFunction("free");
    if (!freeFunction) {
        freeFunction = llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(context), { llvm::PointerType::getUnqual(context) }, false),
            llvm::Function::ExternalLinkage,
            "free",
            module.get()
        );
    }

    this->symbolTable.initializeBuiltInTypes(context);

	// int?대옒?ㅼ뿉 異붽???toString硫붿꽌?쒖쓽 llvm 肄붾뱶 ?앹꽦
	auto initClassSymbolOpt = symbolTable.lookupClass("Int");
    if (!initClassSymbolOpt) {
        throw std::runtime_error("Fatal: 'Int' class not found in symbol table");
	}
    auto* initClassSymbol = initClassSymbolOpt;
	llvm::Function::Create(
		llvm::FunctionType::get(llvm::PointerType::getUnqual(context), { llvm::PointerType::getUnqual((initClassSymbol->classType)->getContext()) }, false),
		llvm::Function::ExternalLinkage,
		"toString",
		module.get()
	);

    llvm::Function::Create(
		llvm::FunctionType::get(llvm::PointerType::getUnqual(context), { llvm::PointerType::getUnqual(context), llvm::PointerType::getUnqual(context) }, false),
		llvm::Function::ExternalLinkage,
		"operator+",
		module.get()
	);

    // print ?⑥닔 ?앹꽦
    llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getVoidTy(context), { llvm::PointerType::getUnqual(context) }, false),
        llvm::Function::ExternalLinkage,
        "print",
        module.get()
    );

    llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getVoidTy(context), { llvm::PointerType::getUnqual(context) }, false),
        llvm::Function::ExternalLinkage,
        "println",
        module.get()
    );

    this->currentSymbol = nullptr;
}

void CodeGenerator::dumpIR() {
    module->print(llvm::outs(), nullptr);
}

void CodeGenerator::enterCleanupScope() {
    cleanupScopes.emplace_back();
}

void CodeGenerator::exitCleanupScope() {
    if (!cleanupScopes.empty()) {
        for (const auto& entry : cleanupScopes.back()) {
            dropFlags.erase(entry.first);
        }
        cleanupScopes.pop_back();
    }
}

llvm::Value* CodeGenerator::dropFlagInCurrentFunction(llvm::Value* pointer) const {
    auto it = dropFlags.find(pointer);
    if (it == dropFlags.end() || !builder.GetInsertBlock()) return nullptr;
    auto* flag = llvm::dyn_cast<llvm::Instruction>(it->second);
    if (!flag || flag->getFunction() != builder.GetInsertBlock()->getParent()) return nullptr;
    return flag;
}

void CodeGenerator::registerOwned(llvm::Value* pointer, CleanupKind kind) {
    if (!pointer || cleanupScopes.empty()) {
        return;
    }
    forgetCleanup(pointer);
    if (llvm::BasicBlock* block = builder.GetInsertBlock()) {
        llvm::Function* fn = block->getParent();
        llvm::IRBuilder<> entry(&fn->getEntryBlock(), fn->getEntryBlock().begin());
        llvm::Value* flag = entry.CreateAlloca(llvm::Type::getInt1Ty(context), nullptr, "drop.flag");
        entry.CreateStore(llvm::ConstantInt::getFalse(context), flag);
        builder.CreateStore(llvm::ConstantInt::getTrue(context), flag);
        dropFlags[pointer] = flag;
    }
    cleanupScopes.back().push_back({ pointer, kind });
}

void CodeGenerator::registerCleanup(llvm::Value* pointer) {
    registerOwned(pointer, CleanupKind::Free);
}

void CodeGenerator::registerTensorCleanup(llvm::Value* tensor) {
    registerOwned(tensor, CleanupKind::TensorFree);
}

void CodeGenerator::registerSharedCleanup(llvm::Value* controlBlock) {
    registerOwned(controlBlock, CleanupKind::SharedRelease);
}

void CodeGenerator::registerWeakCleanup(llvm::Value* controlBlock) {
    registerOwned(controlBlock, CleanupKind::WeakRelease);
}

void CodeGenerator::registerFlaggedCleanup(llvm::Value* pointer) {
    registerOwned(pointer, CleanupKind::Free);
}

bool CodeGenerator::hasCleanup(llvm::Value* pointer) const {
    if (!pointer) return false;
    for (const auto& scope : cleanupScopes) {
        for (const auto& entry : scope) {
            if (entry.first == pointer) return true;
        }
    }
    return false;
}

bool CodeGenerator::forgetCleanup(llvm::Value* pointer) {
    if (!pointer) return false;
    bool found = false;
    for (auto& scope : cleanupScopes) {
        const size_t before = scope.size();
        scope.erase(std::remove_if(scope.begin(), scope.end(), [&](const auto& entry) {
            return entry.first == pointer;
        }), scope.end());
        found = found || scope.size() != before;
    }
    dropFlags.erase(pointer);
    return found;
}

void CodeGenerator::noteMoved(llvm::Value* pointer) {
    if (llvm::Value* flag = dropFlagInCurrentFunction(pointer)) {
        builder.CreateStore(llvm::ConstantInt::getFalse(context), flag);
    }
}

namespace {
void collectRetained(llvm::Value* value, std::unordered_set<llvm::Value*>& retained) {
    if (!value || !retained.insert(value).second) return;
    if (auto* insert = llvm::dyn_cast<llvm::InsertValueInst>(value)) {
        collectRetained(insert->getAggregateOperand(), retained);
        collectRetained(insert->getInsertedValueOperand(), retained);
    }
    else if (auto* phi = llvm::dyn_cast<llvm::PHINode>(value)) {
        for (llvm::Value* incoming : phi->incoming_values()) collectRetained(incoming, retained);
    }
    else if (auto* select = llvm::dyn_cast<llvm::SelectInst>(value)) {
        collectRetained(select->getTrueValue(), retained);
        collectRetained(select->getFalseValue(), retained);
    }
    else if (auto* cast = llvm::dyn_cast<llvm::CastInst>(value)) {
        collectRetained(cast->getOperand(0), retained);
    }
}

bool belongsToOtherFunction(llvm::Value* value, llvm::Function* fn) {
    if (auto* inst = llvm::dyn_cast<llvm::Instruction>(value)) return inst->getFunction() != fn;
    if (auto* arg = llvm::dyn_cast<llvm::Argument>(value)) return arg->getParent() != fn;
    return false;
}
}

void CodeGenerator::emitCleanupEntry(llvm::Value* pointer, CleanupKind kind,
                                     const std::unordered_set<llvm::Value*>& retained) {
    if (!pointer || retained.count(pointer)) {
        return;
    }
    llvm::BasicBlock* block = builder.GetInsertBlock();
    if (!block || belongsToOtherFunction(pointer, block->getParent())) {
        return;
    }
    llvm::Value* flag = dropFlagInCurrentFunction(pointer);
    if (!flag) {
        emitCleanupBody(pointer, kind);
        return;
    }
    llvm::Function* fn = block->getParent();
    llvm::Value* live = builder.CreateLoad(llvm::Type::getInt1Ty(context), flag, "drop.live");
    auto* freeBB = llvm::BasicBlock::Create(context, "drop.free", fn);
    auto* contBB = llvm::BasicBlock::Create(context, "drop.cont", fn);
    builder.CreateCondBr(live, freeBB, contBB);
    builder.SetInsertPoint(freeBB);
    emitCleanupBody(pointer, kind);
    builder.CreateBr(contBB);
    builder.SetInsertPoint(contBB);
}

void CodeGenerator::emitCleanupBody(llvm::Value* pointer, CleanupKind kind) {
    auto* i8PtrTy = llvm::PointerType::getUnqual(context);
    llvm::Value* raw = pointer->getType() == i8PtrTy
        ? pointer
        : builder.CreateBitCast(pointer, i8PtrTy, "cleanup.ptr");
    if (kind == CleanupKind::TensorFree) {
        // TensorRuntime allocates its header, dimension buffer, and element buffer independently.
        // Dropping only the header leaks the latter two allocations.
        llvm::Value* dims = TensorRuntime::loadDims(*this, pointer);
        llvm::Value* data = TensorRuntime::loadData(*this, pointer);
        llvm::Value* dimsRaw = builder.CreateBitCast(dims, i8PtrTy, "tensor.dims.free");
        llvm::Value* dataRaw = builder.CreateBitCast(data, i8PtrTy, "tensor.data.free");
        builder.CreateCall(freeFunction, { dataRaw });
        builder.CreateCall(freeFunction, { dimsRaw });
        builder.CreateCall(freeFunction, { raw });
    }
    else if (kind == CleanupKind::SharedRelease || kind == CleanupKind::WeakRelease) {
        // The control block is { i64 strong, i64 weak, payload }. A Shared owner decrements strong,
        // a Weak handle decrements weak. The block is freed only when BOTH counts reach zero (so a
        // live Weak keeps the block, and dropping the last Weak after all strong owners reclaims it).
        auto* i64Ty = llvm::Type::getInt64Ty(context);
        llvm::Function* fn = builder.GetInsertBlock()->getParent();
        llvm::Value* strongPtr = pointer;
        llvm::Value* weakPtr = builder.CreateGEP(i64Ty, pointer, llvm::ConstantInt::get(i64Ty, 1), "block.weak.ptr");
        llvm::Value* one = llvm::ConstantInt::get(i64Ty, 1);
        auto* freeBB = llvm::BasicBlock::Create(context, "block.free", fn);
        auto* contBB = llvm::BasicBlock::Create(context, "block.cont", fn);
        if (kind == CleanupKind::SharedRelease) {
            llvm::Value* strong = builder.CreateAtomicRMW(llvm::AtomicRMWInst::Sub, strongPtr, one, llvm::MaybeAlign(8),
                                                          llvm::AtomicOrdering::AcquireRelease);
            auto* lastStrongBB = llvm::BasicBlock::Create(context, "block.last.strong", fn);
            builder.CreateCondBr(builder.CreateICmpEQ(strong, one, "strong.last"), lastStrongBB, contBB);
            builder.SetInsertPoint(lastStrongBB);
        }
        llvm::Value* weak = builder.CreateAtomicRMW(llvm::AtomicRMWInst::Sub, weakPtr, one, llvm::MaybeAlign(8),
                                                    llvm::AtomicOrdering::AcquireRelease);
        builder.CreateCondBr(builder.CreateICmpEQ(weak, one, "weak.last"), freeBB, contBB);
        builder.SetInsertPoint(freeBB);
        builder.CreateCall(freeFunction, { raw });
        builder.CreateBr(contBB);
        builder.SetInsertPoint(contBB);
    }
    else {
        builder.CreateCall(freeFunction, { raw });
    }
}

void CodeGenerator::emitCurrentScopeCleanups() {
    if (cleanupScopes.empty()) {
        return;
    }
    const std::unordered_set<llvm::Value*> none;
    auto& cleanups = cleanupScopes.back();
    for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) {
        emitCleanupEntry(it->first, it->second, none);
    }
    cleanups.clear();
}

llvm::Value* CodeGenerator::copyStackBufferToHeap(llvm::Value* value) {
    auto* stack = llvm::dyn_cast_or_null<llvm::AllocaInst>(value);
    if (!stack || !builder.GetInsertBlock()) return value;
    auto* i64Ty = llvm::Type::getInt64Ty(context);
    const llvm::DataLayout& layout = module->getDataLayout();
    llvm::Value* elementBytes = llvm::ConstantInt::get(i64Ty, layout.getTypeAllocSize(stack->getAllocatedType()));
    llvm::Value* elements = builder.CreateZExtOrTrunc(stack->getArraySize(), i64Ty, "stack.copy.count");
    llvm::Value* bytes = builder.CreateMul(elementBytes, elements, "stack.copy.bytes");
    llvm::Value* heap = builder.CreateCall(mallocFunction, { bytes }, "stack.copy.heap");
    builder.CreateMemCpy(heap, llvm::MaybeAlign(1), stack, llvm::MaybeAlign(1), bytes);
    return heap;
}

void CodeGenerator::releaseFromCurrentScope(llvm::Value* value) {
    if (cleanupScopes.empty() || !value) return;
    std::unordered_set<llvm::Value*> retained;
    collectRetained(value, retained);
    auto& scope = cleanupScopes.back();
    scope.erase(std::remove_if(scope.begin(), scope.end(), [&](const auto& entry) {
        if (!retained.count(entry.first)) return false;
        dropFlags.erase(entry.first);
        return true;
    }), scope.end());
}

void CodeGenerator::emitAllCleanups() {
    emitAllCleanupsExcept(nullptr);
}

void CodeGenerator::emitAllCleanupsExcept(llvm::Value* retainedValue) {
    // Emit drops for every live owned local along THIS return path, innermost scope first. Unlike a
    // normal scope exit, the lists are NOT cleared: a `return` is one runtime path among several, so
    // each early return and the fall-through exit must independently emit their own frees. Clearing
    // here made the first return consume the registry and leak the locals on every later path (e.g.
    // `if (c) { return 1; } return 0;` freed the box only on the `then` path). A single runtime path
    // reaches either a return or its block's normal scope-exit cleanup, never both, so leaving the
    // entries in place frees each local exactly once per path (M4 per-path drop elaboration).
    std::unordered_set<llvm::Value*> retained;
    collectRetained(retainedValue, retained);
    emitTemporaryDropsAbove(0);
    for (auto scopeIt = cleanupScopes.rbegin(); scopeIt != cleanupScopes.rend(); ++scopeIt) {
        auto& cleanups = *scopeIt;
        for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) {
            emitCleanupEntry(it->first, it->second, retained);
        }
    }
}

void CodeGenerator::reportError(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
    ++errorCount;
}

llvm::AllocaInst* CodeGenerator::createEntryAlloca(llvm::Type* type, const std::string& name) {
    llvm::Function* fn = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock& entry = fn->getEntryBlock();
    llvm::IRBuilder<> at(&entry, entry.getFirstInsertionPt());
    return at.CreateAlloca(type, nullptr, name);
}

void CodeGenerator::emitCleanupsAbove(size_t depth) {
    const std::unordered_set<llvm::Value*> none;
    emitTemporaryDropsAbove(depth);
    for (size_t i = cleanupScopes.size(); i > depth; --i) {
        auto& cleanups = cleanupScopes[i - 1];
        for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) {
            emitCleanupEntry(it->first, it->second, none);
        }
    }
}

namespace {
const char* const freshTensorKind = "csec.fresh.tensor";
const char* const tensorAllocationKind = "csec.tensor.allocation";
}

bool CodeGenerator::isTensorAllocation(llvm::Value* tensor) const {
    auto* inst = llvm::dyn_cast_or_null<llvm::Instruction>(tensor);
    return inst && inst->getMetadata(tensorAllocationKind) != nullptr;
}

void CodeGenerator::markFreshTensor(llvm::Value* tensor) {
    if (auto* inst = llvm::dyn_cast_or_null<llvm::Instruction>(tensor)) {
        inst->setMetadata(tensorAllocationKind, llvm::MDNode::get(context, {}));
        inst->setMetadata(freshTensorKind, llvm::MDNode::get(context, {}));
        freshTensorMarks.emplace_back(inst);
    }
}

bool CodeGenerator::isFreshTensor(llvm::Value* tensor) const {
    auto* inst = llvm::dyn_cast_or_null<llvm::Instruction>(tensor);
    return inst && inst->getMetadata(freshTensorKind) != nullptr;
}

void CodeGenerator::clearFreshTensor(llvm::Value* tensor) {
    if (auto* inst = llvm::dyn_cast_or_null<llvm::Instruction>(tensor)) {
        inst->setMetadata(freshTensorKind, nullptr);
    }
}

void CodeGenerator::stripFreshTensorMarks() {
    for (auto& mark : freshTensorMarks) {
        clearFreshTensor(mark);
    }
    freshTensorMarks.clear();
}

llvm::Value* CodeGenerator::borrowTensors(const std::vector<llvm::Value*>& operands, llvm::Value* result) {
    if (!result) return result;
    for (llvm::Value* operand : operands) {
        dropTemporary(operand, result);
    }
    return result;
}

llvm::Function* CodeGenerator::getTensorDropFunction() {
    const char* name = "csec.tensor.drop";
    if (llvm::Function* existing = module->getFunction(name)) return existing;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    auto* fnTy = llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false);
    auto* fn = llvm::Function::Create(fnTy, llvm::Function::InternalLinkage, name, module.get());
    llvm::IRBuilderBase::InsertPointGuard guard(builder);
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    auto* entryBB = llvm::BasicBlock::Create(context, "entry", fn);
    auto* freeBB = llvm::BasicBlock::Create(context, "free", fn);
    auto* doneBB = llvm::BasicBlock::Create(context, "done", fn);
    llvm::Value* tensor = fn->getArg(0);
    builder.SetInsertPoint(entryBB);
    builder.CreateCondBr(builder.CreateIsNull(tensor, "tensor.null"), doneBB, freeBB);
    builder.SetInsertPoint(freeBB);
    llvm::Value* data = TensorRuntime::loadData(*this, tensor);
    llvm::Value* dims = TensorRuntime::loadDims(*this, tensor);
    builder.CreateCall(freeFunction, { data });
    builder.CreateCall(freeFunction, { dims });
    builder.CreateCall(freeFunction, { tensor });
    builder.CreateBr(doneBB);
    builder.SetInsertPoint(doneBB);
    builder.CreateRetVoid();
    return fn;
}

void CodeGenerator::emitTemporarySlotDrop(llvm::AllocaInst* slot) {
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    llvm::Value* held = builder.CreateLoad(ptrTy, slot, "tmp.tensor.held");
    builder.CreateCall(getTensorDropFunction(), { held });
    builder.CreateStore(llvm::ConstantPointerNull::get(ptrTy), slot);
}

void CodeGenerator::dropTemporary(llvm::Value* tensor, llvm::Value* result) {
    if (!tensor || tensor == result || !isFreshTensor(tensor) || temporaryFrames.empty()) return;
    llvm::BasicBlock* block = builder.GetInsertBlock();
    if (!block || csec_llvm::terminatorOrNull(block)) return;
    TemporaryFrame& frame = temporaryFrames.back();
    if (frame.function != block->getParent()) return;
    clearFreshTensor(tensor);
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    llvm::AllocaInst* slot = createEntryAlloca(ptrTy, "tmp.tensor.slot");
    llvm::IRBuilder<> init(slot->getParent(), std::next(slot->getIterator()));
    init.CreateStore(llvm::ConstantPointerNull::get(ptrTy), slot);
    llvm::Value* previous = builder.CreateLoad(ptrTy, slot, "tmp.tensor.previous");
    builder.CreateCall(getTensorDropFunction(), { previous });
    builder.CreateStore(tensor, slot);
    frame.slots.push_back(slot);
}

void CodeGenerator::enterTemporaryFrame() {
    llvm::BasicBlock* block = builder.GetInsertBlock();
    temporaryFrames.push_back({ block ? block->getParent() : nullptr, cleanupScopes.size(), {} });
}

void CodeGenerator::exitTemporaryFrame() {
    if (temporaryFrames.empty()) return;
    TemporaryFrame frame = std::move(temporaryFrames.back());
    temporaryFrames.pop_back();
    llvm::BasicBlock* block = builder.GetInsertBlock();
    if (!block || csec_llvm::terminatorOrNull(block) || block->getParent() != frame.function) return;
    for (auto it = frame.slots.rbegin(); it != frame.slots.rend(); ++it) {
        emitTemporarySlotDrop(*it);
    }
}

void CodeGenerator::emitTemporaryDropsAbove(size_t depth) {
    llvm::BasicBlock* block = builder.GetInsertBlock();
    if (!block || csec_llvm::terminatorOrNull(block)) return;
    llvm::Function* fn = block->getParent();
    for (auto frameIt = temporaryFrames.rbegin(); frameIt != temporaryFrames.rend(); ++frameIt) {
        if (frameIt->function != fn || frameIt->cleanupDepth < depth) continue;
        for (auto it = frameIt->slots.rbegin(); it != frameIt->slots.rend(); ++it) {
            emitTemporarySlotDrop(*it);
        }
    }
}

void CodeGenerator::addExternalLinkLibrary(const std::string& library) {
    addUnique(externalLinkLibraries, library);
}

void CodeGenerator::addExternalLinkPath(const std::string& path) {
    addUnique(externalLinkPaths, path);
}

void CodeGenerator::requireSystemNative() {
    addExternalLinkLibrary("System.Native");
}

llvm::Type* CodeGenerator::getLLVMType(const Type* type) {
    if (type->getKind() == Type::Kind::BASIC) {
        const std::string name = type->getName();
        if (name == "Int") {
            return llvm::Type::getInt32Ty(context);
        }
        else if (name == "Short") {
            return llvm::Type::getInt16Ty(context);
        }
        else if (name == "Byte") {
            return llvm::Type::getInt8Ty(context);
        }
        else if (name == "Long") {
            return llvm::Type::getInt64Ty(context);
        }
        else if (name == "Natural" || name == "Integer") {
            return llvm::Type::getInt64Ty(context);
        }
        else if (name == "Real") {
            return llvm::Type::getDoubleTy(context);
        }
        else if (name == "Boolean" || name == "Bool") {
            return llvm::Type::getInt1Ty(context);
        }
        else if (type->isFloatTy()) {
            return llvm::Type::getFloatTy(context);
        }
        else if (type->isDoubleTy()) {
            return llvm::Type::getDoubleTy(context);
        }
        else if (type->isCharTy()) {
            return llvm::Type::getInt8Ty(context);
        }
        else if (type->isStringTy()) {
			return llvm::PointerType::getUnqual(context);
        }
        else if (type->isVoidTy()) {
            return llvm::Type::getVoidTy(context);
        }
        std::cerr << "Error: Unsupported basic type '" << type->getName() << "'" << std::endl;
        return nullptr;
    }
    else if (type->getKind() == Type::Kind::CLASS) {
        if (TensorRuntime::isTensorTypeName(type->getName())) {
            return TensorRuntime::getTensorPointerType(*this);
        }
        if (isInterfaceType(type)) {
            return llvm::PointerType::getUnqual(context);  // { object, vtable } on the heap
        }
        auto* classSymbol = symbolTable.lookupClass(type->getName());
        if (classSymbol && !classSymbol->classType && materializeTemplateClass(type->getName())) {
            classSymbol = symbolTable.lookupClass(type->getName());
        }
        // A template instance registered while type checking has an opaque struct until it is generated.
        if (classSymbol && classSymbol->classType && llvm::isa<llvm::StructType>(classSymbol->classType) &&
            llvm::cast<llvm::StructType>(classSymbol->classType)->isOpaque() && generatingCode &&
            materializeTemplateClass(type->getName())) {
            classSymbol = symbolTable.lookupClass(type->getName());
        }

		// Symbol??ClassSymbol濡?罹먯뒪?낇븯??classType???묎렐
        if (classSymbol) {
            return classSymbol->classType;
		}
		// Built-in value aggregates (Complex, Rational, Quaternion) are StructSymbols, not classes.
		if (auto* structSymbol = dynamic_cast<StructSymbol*>(symbolTable.lookup(type->getName()))) {
			if (structSymbol->structType) {
				return structSymbol->structType;
			}
		}
		std::cerr << "Error: Undefined class type '" << type->getName() << "'" << std::endl;
		return nullptr;
    }
    else if (type->getKind() == Type::Kind::GENERIC) {
        auto* genericType = static_cast<const GenericType*>(type);
        if (genericType->baseType->getName() == "Array") {
            if (genericType->typeArguments.size() != 1) {
                std::cerr << "Error: Array type must have exactly one type argument" << std::endl;
                return nullptr;
            }
            auto elementType = getLLVMType(genericType->typeArguments[0].get());
            if (!elementType) {
                std::cerr << "Error: Invalid element type in Array" << std::endl;
                return nullptr;
            }
            return llvm::PointerType::getUnqual(elementType->getContext());
        }
        if (genericType->baseType->getName() == "Tensor") {
            return TensorRuntime::getTensorPointerType(*this);
        }
        if (genericType->baseType->getName() == "Future") {
            // Future<T>: opaque runtime handle filled by an `async` call (process_runtime.inc).
            return llvm::PointerType::getUnqual(context);
        }
        // Check if this is a template class instantiation: e.g. Pair<Int, String>
        std::string baseName = genericType->baseType->getName();
        auto* symbol = symbolTable.lookup(baseName);
        if (symbol && symbol->symbolType == SymbolType::TEMPLATE) {
            // Build mangled name
            std::string mangledName = baseName;
            for (auto& ta : genericType->typeArguments) {
                mangledName += "$" + templateArgumentName(ta.get());
            }
            // Look up the instantiated class (generating one registered during type checking).
            auto* classSymbol = symbolTable.lookupClass(mangledName);
            if (classSymbol && !classSymbol->classType) {
                materializeTemplateClass(mangledName);
                classSymbol = symbolTable.lookupClass(mangledName);
            }
            if (classSymbol && classSymbol->classType) {
                return classSymbol->classType;
            }
            std::cerr << "Error: Template class '" << mangledName << "' not instantiated yet" << std::endl;
            return nullptr;
        }
        // Library-defined generic shapes such as Vector[Int] or Matrix[Double]
        // may be declared before their concrete runtime representation exists.
        // Treat them as opaque references so they can appear in signatures.
        return llvm::PointerType::getUnqual(context);
    }
    else if (type->getKind() == Type::Kind::STRUCT) {
        auto* structSymbol = symbolTable.lookupStruct(type->getName());
        if (structSymbol) {
            return static_cast<StructSymbol*>(structSymbol)->structType;
        }
        else {
            std::cerr << "Error: Undefined struct type '" << type->getName() << "'" << std::endl;
            return nullptr;
        }
	}
    else if (type->getKind() == Type::Kind::FUNCTION) {
        auto* functionType = static_cast<const FunctionType*>(type);
        std::vector<llvm::Type*> paramLLVMTypes;
        for (const auto& paramType : functionType->parameterTypes) {
            auto llvmParamType = getLLVMType(paramType.get());
            if (!llvmParamType) {
                std::cerr << "Error: Invalid parameter type in function" << std::endl;
                return nullptr;
            }
            paramLLVMTypes.push_back(llvmParamType);
        }
        auto returnLLVMType = getLLVMType(functionType->returnType.get());
        if (!returnLLVMType) {
            std::cerr << "Error: Invalid return type in function" << std::endl;
            return nullptr;
        }
		auto tp = llvm::FunctionType::get(returnLLVMType, paramLLVMTypes, false);
        return llvm::PointerType::getUnqual(tp->getContext());
    }
    else if (type->getKind() == Type::Kind::ARRAY) {
        auto* arrayType = static_cast<const ArrayType*>(type);
        auto elementLLVMType = getLLVMType(arrayType->elementType.get());
        if (!elementLLVMType) {
            std::cerr << "Error: Invalid element type in array" << std::endl;
            return nullptr;
        }
        return llvm::ArrayType::get(elementLLVMType, arrayType->size);
	}
    else if (type->getKind() == Type::Kind::TUPLE) {
        auto* tupleType = static_cast<const TupleType*>(type);
        std::vector<llvm::Type*> elems;
        for (const auto& e : tupleType->elementTypes) {
            llvm::Type* et = getABIStorageType(e.get());
            if (!et) {
                std::cerr << "Error: Invalid element type in tuple" << std::endl;
                return nullptr;
            }
            elems.push_back(et);
        }
        return llvm::StructType::get(context, elems);
    }
    else if (type->getKind() == Type::Kind::POINTER) {
        auto* pointerType = static_cast<const PointerType*>(type);
        auto baseLLVMType = getLLVMType(pointerType->baseType.get());
        if (!baseLLVMType) {
            std::cerr << "Error: Invalid base type for pointer" << std::endl;
            return nullptr;
        }
        return llvm::PointerType::getUnqual(baseLLVMType->getContext());
	}
    else if (type->getKind() == Type::Kind::BOX) {
        auto* boxType = static_cast<const BoxType*>(type);
        auto baseLLVMType = getLLVMType(boxType->baseType.get());
        if (!baseLLVMType) {
            std::cerr << "Error: Invalid base type for box" << std::endl;
            return nullptr;
        }
        return llvm::PointerType::getUnqual(baseLLVMType->getContext());
    }
    else if (type->getKind() == Type::Kind::BORROW || type->getKind() == Type::Kind::MUTABLE_BORROW) {
        auto* borrowType = static_cast<const BorrowType*>(type);
        auto baseLLVMType = getLLVMType(borrowType->baseType.get());
        if (!baseLLVMType) {
            std::cerr << "Error: Invalid base type for borrow" << std::endl;
            return nullptr;
        }
        return llvm::PointerType::getUnqual(baseLLVMType->getContext());
    }
    else if (type->getKind() == Type::Kind::UNSAFE_POINTER) {
        auto* pointerType = static_cast<const UnsafePointerType*>(type);
        auto baseLLVMType = getLLVMType(pointerType->baseType.get());
        if (!baseLLVMType) {
            std::cerr << "Error: Invalid base type for unsafe pointer" << std::endl;
            return nullptr;
        }
        return llvm::PointerType::getUnqual(baseLLVMType->getContext());
    }
    else if (type->getKind() == Type::Kind::UNKNOWN) {
        std::cerr << "Error: Unknown type '" << type->getName() << "'" << std::endl;
        return nullptr;
    }
    return nullptr;
}
