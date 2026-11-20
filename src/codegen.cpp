// codegen.cpp
#include "codegen.h"
#include "TensorRuntime.h"
#include "type_utils.h"
#include "interfaces.h"
#include "type_checker.h"
#include "enums.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <algorithm>

#include "ProgramNode.h"
#include "ClassDeclarationNode.h"
#include "ClassBodyNode.h"
#include "ParameterNode.h"
#include "VariableDeclarationNode.h"


namespace {
void addUnique(std::vector<std::string>& values, const std::string& value) {
    if (value.empty()) return;
    for (const auto& existing : values) {
        if (existing == value) return;
    }
    values.push_back(value);
}
}

CodeGenerator::CodeGenerator()
    : builder(context, llvm::ConstantFolder(), llvm::IRBuilderCallbackInserter([this](llvm::Instruction* inst) { onInstructionInserted(inst); })) {
    this->module = std::make_unique<llvm::Module>("main", context);

    auto* i8PtrTy = llvm::PointerType::getUnqual(context);
    auto* argvTy = llvm::PointerType::getUnqual(i8PtrTy->getContext());
    llvm::FunctionType* funcType = llvm::FunctionType::get(this->builder.getInt32Ty(), {this->builder.getInt32Ty(), argvTy}, false);
    this->mainFunction = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module.get());

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", mainFunction);
    this->builder.SetInsertPoint(entry);
    this->builder.CreateRet(this->builder.getInt32(0));

    runtimeStringFunctions.insert("csec_string_concat");
    runtimeStringFunctions.insert("csec_string_concat_many");
    runtimeStringFunctions.insert("csec_string_substring");
    runtimeStringFunctions.insert("csec_string_to_upper");
    runtimeStringFunctions.insert("csec_string_to_lower");
    runtimeStringFunctions.insert("csec_string_trim");
    runtimeStringFunctions.insert("csec_to_string_i64");
    runtimeStringFunctions.insert("csec_to_string_double");
    runtimeStringFunctions.insert("csec_to_string_bool");
    runtimeStringFunctions.insert("csec_to_string_char");
    runtimeStringFunctions.insert("csec_read_line");
    runtimeStringFunctions.insert("csec_command_line_arg");
    runtimeStringFunctions.insert("csec_file_read_all_text");
    runtimeStringFunctions.insert("csec_tcp_recv");
    runtimeStringFunctions.insert("csec_posix_read");
    runtimeStringFunctions.insert("csec_posix_getcwd");
    runtimeStringFunctions.insert("csec_posix_getenv");
    runtimeStringFunctions.insert("csec_io_read");
    runtimeStringFunctions.insert("csec_pipe_read");
    runtimeStringFunctions.insert("csec_pipe_read_line");
    runtimeStringFunctions.insert("csec_piped_process_read_line");
    runtimeStringFunctions.insert("csec_piped_process_read_all");
    runtimeStringFunctions.insert("csec_shm_read_string");
    runtimeStringFunctions.insert("csec_mail_last_body");
    runtimeStringFunctions.insert("csec_mail_last_sender");
    runtimeStringFunctions.insert("csec_process_receive_text");
    runtimeStringFunctions.insert("csec_process_receive_text_timeout");
    runtimeStringFunctions.insert("csec_os_process_output");
    runtimeStringFunctions.insert("csec_df_last_error");
    runtimeStringFunctions.insert("csec_df_to_csv_text");
    runtimeStringFunctions.insert("csec_df_column_name");
    runtimeStringFunctions.insert("csec_df_column_type");
    runtimeStringFunctions.insert("csec_df_get_string");
    runtimeStringFunctions.insert("csec_df_to_string");
    runtimeStringFunctions.insert("csec_expr_to_string");
    runtimeStringFunctions.insert("csec_df_format");
    runtimeStringFunctions.insert("csec_df_schema");
    runtimeStringFunctions.insert("csec_df_dtype");
    runtimeStringFunctions.insert("csec_df_last_scan_info");
    runtimeStringFunctions.insert("csec_lazy_explain");
    runtimeStringFunctions.insert("csec_lazy_schema");
    runtimeStringFunctions.insert("csec_json_last_error");
    runtimeStringFunctions.insert("csec_json_type");
    runtimeStringFunctions.insert("csec_json_get_string");
    runtimeStringFunctions.insert("csec_json_key");
    runtimeStringFunctions.insert("csec_json_stringify");
    runtimeStringFunctions.insert("csec_json_quote");
    runtimeStringFunctions.insert("csec_http_last_error");
    runtimeStringFunctions.insert("csec_http_error");
    runtimeStringFunctions.insert("csec_http_headers");
    runtimeStringFunctions.insert("csec_http_header");
    runtimeStringFunctions.insert("csec_http_body");
    runtimeStringFunctions.insert("csec_http_line");
    runtimeStringFunctions.insert("csec_http_event_data");
    runtimeStringFunctions.insert("csec_http_event_name");
    runtimeStringFunctions.insert("csec_http_event_id");
    runtimeStringFunctions.insert("csec_url_encode");
    runtimeStringFunctions.insert("csec_url_decode");
    runtimeStringFunctions.insert("csec_url_resolve");
    runtimeStringFunctions.insert("csec_html_to_text");
    runtimeStringFunctions.insert("csec_html_title");
    runtimeStringFunctions.insert("csec_html_links");
    runtimeStringFunctions.insert("csec_bigint_to_string");
    for (const char* producer : {"csec_bigint_from_i64", "csec_bigint_from_decimal", "csec_bigint_add",
                                 "csec_bigint_sub", "csec_bigint_mul", "csec_bigint_div", "csec_bigint_mod",
                                 "csec_bigint_pow", "csec_bigint_gcd", "csec_bigint_isqrt"}) {
        userStringFunctions.insert(producer);
    }
    runtimeStringFunctions.insert("csec_dict_to_string");
    runtimeStringFunctions.insert("csec_dict_key_at");
    runtimeStringFunctions.insert("csec_alg_residue_to_string");
    runtimeStringFunctions.insert("csec_poly_to_string");
    runtimeStringFunctions.insert("csec_gf_to_string");
    runtimeStringFunctions.insert("csec_logic_to_string");
    runtimeStringFunctions.insert("csec_logic_counterexample");
    runtimeStringFunctions.insert("csec_logic_model");
    runtimeStringFunctions.insert("csec_thm_to_string");
    runtimeStringFunctions.insert("csec_sym_solve");
    runtimeStringFunctions.insert("csec_sym_to_string");
    runtimeStringFunctions.insert("csec_sym_to_latex");
    for (const char* producer : { "csec_dict_new", "csec_set_new", "csec_dict_copy", "csec_dict_keys" }) {
        dictProducerNames.push_back(producer);
    }
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
            dropFunctions.erase(entry.first);
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
    dropFunctions.erase(pointer);
    return found;
}

void CodeGenerator::noteMoved(llvm::Value* pointer) {
    if (llvm::Value* flag = dropFlagInCurrentFunction(pointer)) {
        builder.CreateStore(llvm::ConstantInt::getFalse(context), flag);
    }
}

namespace {
void collectRetained(llvm::Value* value, std::unordered_set<llvm::Value*>& retained, bool followLoads = true) {
    if (!value || !retained.insert(value).second) return;
    if (auto* insert = llvm::dyn_cast<llvm::InsertValueInst>(value)) {
        collectRetained(insert->getAggregateOperand(), retained, followLoads);
        collectRetained(insert->getInsertedValueOperand(), retained, followLoads);
    }
    else if (auto* phi = llvm::dyn_cast<llvm::PHINode>(value)) {
        for (llvm::Value* incoming : phi->incoming_values()) {
            if (!CodeGenerator::getInstance().hasDropFlag(incoming)) collectRetained(incoming, retained, followLoads);
        }
    }
    else if (auto* select = llvm::dyn_cast<llvm::SelectInst>(value)) {
        collectRetained(select->getTrueValue(), retained, followLoads);
        collectRetained(select->getFalseValue(), retained, followLoads);
    }
    else if (auto* cast = llvm::dyn_cast<llvm::CastInst>(value)) {
        collectRetained(cast->getOperand(0), retained, followLoads);
    }
    else if (auto* load = llvm::dyn_cast<llvm::LoadInst>(value); load && followLoads) {
        retained.insert(load->getPointerOperand());
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
    if (!pointer || (kind != CleanupKind::StringSlotRelease && retained.count(pointer))) {
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
        // The control block is { i64 strong, i64 weak, payload drop, payload }. A Shared owner decrements strong,
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
            llvm::Value* dropSlot = builder.CreateGEP(i64Ty, pointer, llvm::ConstantInt::get(i64Ty, 2), "block.drop.ptr");
            llvm::Value* payloadDrop = builder.CreateLoad(i8PtrTy, dropSlot, "block.drop");
            auto* dropBB = llvm::BasicBlock::Create(context, "block.drop.payload", fn);
            auto* weakBB = llvm::BasicBlock::Create(context, "block.weak", fn);
            builder.CreateCondBr(builder.CreateIsNull(payloadDrop), weakBB, dropBB);
            builder.SetInsertPoint(dropBB);
            builder.CreateCall(llvm::FunctionType::get(llvm::Type::getVoidTy(context), { i8PtrTy }, false), payloadDrop, { raw });
            builder.CreateBr(weakBB);
            builder.SetInsertPoint(weakBB);
        }
        llvm::Value* weak = builder.CreateAtomicRMW(llvm::AtomicRMWInst::Sub, weakPtr, one, llvm::MaybeAlign(8),
                                                    llvm::AtomicOrdering::AcquireRelease);
        builder.CreateCondBr(builder.CreateICmpEQ(weak, one, "weak.last"), freeBB, contBB);
        builder.SetInsertPoint(freeBB);
        builder.CreateCall(freeFunction, { raw });
        builder.CreateBr(contBB);
        builder.SetInsertPoint(contBB);
    }
    else if (kind == CleanupKind::SharedSlotRelease || kind == CleanupKind::WeakSlotRelease) {
        llvm::Value* held = builder.CreateLoad(i8PtrTy, pointer, "handle.local");
        llvm::Function* fn = builder.GetInsertBlock()->getParent();
        auto* releaseBB = llvm::BasicBlock::Create(context, "handle.release", fn);
        auto* doneBB = llvm::BasicBlock::Create(context, "handle.done", fn);
        builder.CreateCondBr(builder.CreateIsNull(held, "handle.null"), doneBB, releaseBB);
        builder.SetInsertPoint(releaseBB);
        emitCleanupBody(held, kind == CleanupKind::SharedSlotRelease ? CleanupKind::SharedRelease : CleanupKind::WeakRelease);
        builder.CreateBr(doneBB);
        builder.SetInsertPoint(doneBB);
    }
    else if (kind == CleanupKind::OwnedSlotDrop) {
        llvm::Value* held = builder.CreateLoad(i8PtrTy, pointer, "owned.local");
        auto found = dropFunctions.find(pointer);
        if (found != dropFunctions.end()) builder.CreateCall(found->second, { held });
    }
    else if (kind == CleanupKind::DictSlotFree) {
        llvm::Value* held = builder.CreateLoad(i8PtrTy, pointer, "dict.local");
        builder.CreateCall(dictFreeFunction(), { held });
    }
    else if (kind == CleanupKind::TensorSlotDrop) {
        llvm::Value* held = builder.CreateLoad(i8PtrTy, pointer, "tensor.local");
        builder.CreateCall(getTensorDropFunction(), { held });
    }
    else if (kind == CleanupKind::StringSlotRelease) {
        ++stringHookDepth;
        llvm::Value* held = builder.CreateLoad(i8PtrTy, pointer, "string.local");
        builder.CreateCall(rcFunction("csec_rc_release"), { held });
        --stringHookDepth;
    }
    else if (auto found = dropFunctions.find(pointer); found != dropFunctions.end()) {
        builder.CreateCall(found->second, { raw });
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

namespace {
bool ownedReferenceClass(const Type* type) {
    return type && type->getKind() == Type::Kind::CLASS && !typeIsCopy(type);
}

bool ownedArrayType(const Type* type) {
    if (!type) return false;
    if (type->getKind() == Type::Kind::ARRAY) return true;
    return type->getKind() == Type::Kind::GENERIC && (type->getName() == "Array" || type->getName() == "Vector");
}
}

llvm::Function* CodeGenerator::classDropFunction(const Type* type) {
    if (!ownedReferenceClass(type)) return nullptr;
    ClassSymbol* classSymbol = symbolTable.lookupClass(type->getName());
    auto* layout = classSymbol ? llvm::dyn_cast_or_null<llvm::StructType>(classSymbol->classType) : nullptr;
    if (!layout || layout->isOpaque()) return nullptr;
    const std::string name = "__csec_drop." + type->getName();
    if (llvm::Function* existing = module->getFunction(name)) return existing;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    auto* fn = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false),
                                      llvm::Function::InternalLinkage, name, module.get());
    pendingBodies.push_back({ fn, const_cast<Type*>(type)->clone(), false });
    return fn;
}

llvm::Function* CodeGenerator::arrayDropFunction(const Type* type) {
    if (!ownedArrayType(type)) return nullptr;
    const Type* element = nullptr;
    if (auto* array = dynamic_cast<const ArrayType*>(type)) element = array->elementType.get();
    else if (auto* generic = dynamic_cast<const GenericType*>(type)) {
        if (generic->typeArguments.empty()) return nullptr;
        element = generic->typeArguments[0].get();
    }
    if (!element) return nullptr;
    const bool counted = isRefcountedType(element);
    llvm::Function* structDrop = counted ? nullptr : structCountFunction(element, false);
    if (!counted && !structDrop) return nullptr;
    llvm::Type* elementType = getABIStorageType(element);
    if (!elementType) return nullptr;
    const std::string name = "__csec_array_drop." + element->getName();
    if (llvm::Function* existing = module->getFunction(name)) return existing;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    auto* i64Ty = llvm::Type::getInt64Ty(context);
    requireSystemNative();
    llvm::FunctionCallee untrack = module->getOrInsertFunction(
        "csec_array_untrack", llvm::FunctionType::get(i64Ty, { ptrTy }, false));
    auto* fn = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false),
                                      llvm::Function::InternalLinkage, name, module.get());
    llvm::IRBuilderBase::InsertPointGuard guard(builder);
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    ++stringHookDepth;
    auto* entryBB = llvm::BasicBlock::Create(context, "entry", fn);
    auto* headBB = llvm::BasicBlock::Create(context, "head", fn);
    auto* bodyBB = llvm::BasicBlock::Create(context, "body", fn);
    auto* freeBB = llvm::BasicBlock::Create(context, "free", fn);
    auto* doneBB = llvm::BasicBlock::Create(context, "done", fn);
    builder.SetInsertPoint(entryBB);
    llvm::Value* array = fn->getArg(0);
    llvm::BasicBlock* countBB = llvm::BasicBlock::Create(context, "count", fn, headBB);
    builder.CreateCondBr(builder.CreateIsNull(array), doneBB, countBB);
    builder.SetInsertPoint(countBB);
    llvm::Value* count = builder.CreateCall(untrack, { array }, "count");
    builder.CreateBr(headBB);
    builder.SetInsertPoint(headBB);
    llvm::PHINode* index = builder.CreatePHI(i64Ty, 2, "index");
    index->addIncoming(builder.getInt64(0), countBB);
    builder.CreateCondBr(builder.CreateICmpSLT(index, count), bodyBB, freeBB);
    builder.SetInsertPoint(bodyBB);
    llvm::Value* slot = builder.CreateGEP(elementType, array, index);
    if (structDrop) builder.CreateCall(structDrop, { slot });
    else builder.CreateCall(rcReleaseFunction(), { builder.CreateLoad(ptrTy, slot, "element") });
    index->addIncoming(builder.CreateAdd(index, builder.getInt64(1)), bodyBB);
    builder.CreateBr(headBB);
    builder.SetInsertPoint(freeBB);
    builder.CreateCall(freeFunction, { array });
    builder.CreateBr(doneBB);
    builder.SetInsertPoint(doneBB);
    builder.CreateRetVoid();
    --stringHookDepth;
    return fn;
}

llvm::Function* CodeGenerator::sharedStrongDropFunction() {
    const char* name = "__csec_shared_strong_drop";
    if (llvm::Function* fn = module->getFunction(name)) return fn;
    auto* ptr = llvm::PointerType::getUnqual(context);
    auto* fn = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {ptr}, false),
                                      llvm::Function::InternalLinkage, name, module.get());
    llvm::IRBuilderBase::InsertPointGuard guard(builder);
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    auto* entry = llvm::BasicBlock::Create(context, "entry", fn);
    auto* drop = llvm::BasicBlock::Create(context, "drop", fn);
    auto* done = llvm::BasicBlock::Create(context, "done", fn);
    builder.SetInsertPoint(entry);
    builder.CreateCondBr(builder.CreateIsNull(fn->getArg(0)), done, drop);
    builder.SetInsertPoint(drop);
    emitCleanupBody(fn->getArg(0), CleanupKind::SharedRelease);
    builder.CreateBr(done);
    builder.SetInsertPoint(done);
    builder.CreateRetVoid();
    return fn;
}

llvm::Function* CodeGenerator::sharedPayloadDropFunction(const Type* type, llvm::Type* payloadType) {
    if (!type || !payloadType) return nullptr;
    const bool counted = isRefcountedType(type) && payloadType->isPointerTy();
    llvm::Function* structRelease = counted ? nullptr : structCountFunction(type, false);
    if (!counted && !structRelease) return nullptr;
    const std::string name = "__csec_shared_payload_drop." + type->getName();
    if (llvm::Function* existing = module->getFunction(name)) return existing;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    auto* i64Ty = llvm::Type::getInt64Ty(context);
    auto* fn = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false),
                                      llvm::Function::InternalLinkage, name, module.get());
    llvm::IRBuilderBase::InsertPointGuard guard(builder);
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    ++stringHookDepth;
    builder.SetInsertPoint(llvm::BasicBlock::Create(context, "entry", fn));
    llvm::StructType* blockType = llvm::StructType::get(context, { i64Ty, i64Ty, ptrTy, payloadType });
    llvm::Value* payloadPtr = builder.CreateStructGEP(blockType, fn->getArg(0), 3, "payload.ptr");
    if (counted) builder.CreateCall(rcReleaseFunction(), { builder.CreateLoad(ptrTy, payloadPtr, "payload") });
    else builder.CreateCall(structRelease, { payloadPtr });
    builder.CreateRetVoid();
    --stringHookDepth;
    return fn;
}

void CodeGenerator::trackArrayElements(llvm::Value* array, llvm::Value* count) {
    if (!array || !count || !builder.GetInsertBlock()) return;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    auto* i64Ty = llvm::Type::getInt64Ty(context);
    requireSystemNative();
    llvm::FunctionCallee track = module->getOrInsertFunction(
        "csec_array_track", llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy, i64Ty }, false));
    ++stringHookDepth;
    builder.CreateCall(track, { array, builder.CreateZExtOrTrunc(count, i64Ty) });
    --stringHookDepth;
}

void CodeGenerator::emitClassDropBody(llvm::Function* fn, const Type* type) {
    ClassSymbol* classSymbol = symbolTable.lookupClass(type->getName());
    if (!classSymbol || !llvm::dyn_cast_or_null<llvm::StructType>(classSymbol->classType)) {
        std::string templateName;
        if (templateInstanceArguments(type->getName(), &templateName, nullptr)) {
            if (auto* templateSymbol = dynamic_cast<TemplateSymbol*>(symbolTable.lookup(templateName))) {
                auto found = templateSymbol->classInstantiations.find(type->getName());
                if (found != templateSymbol->classInstantiations.end()) classSymbol = found->second;
            }
        }
    }
    auto* layout = classSymbol ? llvm::dyn_cast_or_null<llvm::StructType>(classSymbol->classType) : nullptr;
    if (!layout || layout->isOpaque()) return;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    (void)ptrTy;

    std::vector<std::pair<unsigned, std::unique_ptr<Type>>> slots;
    unsigned index = 0;
    for (const auto& paramName : classSymbol->constructorParamOrder) {
        auto found = classSymbol->constructorParams.find(paramName);
        if (found != classSymbol->constructorParams.end() && found->second && found->second->type) {
            slots.emplace_back(index, found->second->type->clone());
        }
        ++index;
    }
    for (const auto& fieldName : classSymbol->fieldOrder) {
        auto found = classSymbol->fields.find(fieldName);
        if (found != classSymbol->fields.end() && found->second && found->second->type) {
            slots.emplace_back(index, found->second->type->clone());
        }
        ++index;
    }
    std::string templateName;
    std::vector<std::unique_ptr<Type>> templateArguments;
    if (templateInstanceArguments(type->getName(), &templateName, &templateArguments)) {
        auto* templateSymbol = dynamic_cast<TemplateSymbol*>(symbolTable.lookup(templateName));
        if (templateSymbol) {
            for (auto& slot : slots) {
                for (size_t k = 0; k < templateSymbol->typeParameters.size() && k < templateArguments.size(); ++k) {
                    if (slot.second && slot.second->getName() == templateSymbol->typeParameters[k] && templateArguments[k]) {
                        slot.second = templateArguments[k]->clone();
                    }
                }
            }
        }
    }
    llvm::IRBuilderBase::InsertPointGuard guard(builder);
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    auto* entryBB = llvm::BasicBlock::Create(context, "entry", fn);
    auto* bodyBB = llvm::BasicBlock::Create(context, "drop", fn);
    auto* doneBB = llvm::BasicBlock::Create(context, "done", fn);
    llvm::Value* object = fn->getArg(0);
    builder.SetInsertPoint(entryBB);
    builder.CreateCondBr(builder.CreateIsNull(object, "object.null"), doneBB, bodyBB);
    builder.SetInsertPoint(bodyBB);
    if (llvm::Function* userDrop = module->getFunction(type->getName() + "_drop")) {
        if (userDrop->arg_size() == 1) builder.CreateCall(userDrop, { object });
    }
    for (auto& slot : slots) {
        if (slot.first >= layout->getNumElements()) continue;
        llvm::Type* storage = layout->getElementType(slot.first);
        if (storage->isStructTy() && structHoldsCounted(slot.second.get())) {
            emitStructCount(builder.CreateStructGEP(layout, object, slot.first, "drop.struct"), slot.second.get(), false);
            continue;
        }
        if (!storage->isPointerTy()) continue;
        llvm::Value* fieldPtr = builder.CreateStructGEP(layout, object, slot.first, "drop.field");
        llvm::Value* fieldValue = builder.CreateLoad(storage, fieldPtr, "drop.value");
        emitOwnedValueDrop(fieldValue, slot.second.get());
    }
    builder.CreateCall(freeFunction, { object });
    builder.CreateBr(doneBB);
    builder.SetInsertPoint(doneBB);
    builder.CreateRetVoid();
}

void CodeGenerator::emitOwnedValueDrop(llvm::Value* value, const Type* type) {
    if (!value || !type || !value->getType()->isPointerTy() || !builder.GetInsertBlock()) return;
    if (llvm::Function* drop = classDropFunction(type)) {
        builder.CreateCall(drop, { value });
        return;
    }
    if (llvm::Function* drop = arrayDropFunction(type)) {
        builder.CreateCall(drop, { value });
        return;
    }
    if (type->getKind() == Type::Kind::BOX || ownedArrayType(type)) {
        builder.CreateCall(freeFunction, { value });
        return;
    }
    if (TensorRuntime::isTensorTypeName(type->getName())) {
        builder.CreateCall(getTensorDropFunction(), { value });
        return;
    }
    if (isRefcountedType(type)) {
        releaseString(value);
        return;
    }
    if (type->getKind() == Type::Kind::GENERIC && (type->getName() == "Dict" || type->getName() == "Set")) {
        builder.CreateCall(dictFreeFunction(), { value });
        return;
    }
    if (type->getKind() == Type::Kind::GENERIC && (type->getName() == "Shared" || type->getName() == "Weak")) {
        if (type->getName() == "Shared") {
            builder.CreateCall(sharedStrongDropFunction(), {value});
            return;
        }
        llvm::Function* fn = builder.GetInsertBlock()->getParent();
        auto* releaseBB = llvm::BasicBlock::Create(context, "handle.release", fn);
        auto* doneBB = llvm::BasicBlock::Create(context, "handle.done", fn);
        builder.CreateCondBr(builder.CreateIsNull(value, "handle.null"), doneBB, releaseBB);
        builder.SetInsertPoint(releaseBB);
        emitCleanupBody(value, CleanupKind::WeakRelease);
        builder.CreateBr(doneBB);
        builder.SetInsertPoint(doneBB);
    }
}

void CodeGenerator::registerOwnedValueCleanup(llvm::Value* value, const Type* type) {
    if (!value || !type) return;
    if (isRefcountedType(type)) {
        registerOwned(value, CleanupKind::Free);
        dropFunctions[value] = rcReleaseFunction();
        return;
    }
    if (llvm::Function* drop = classDropFunction(type)) {
        registerOwned(value, CleanupKind::Free);
        dropFunctions[value] = drop;
        return;
    }
    if (type->getKind() == Type::Kind::GENERIC && (type->getName() == "Dict" || type->getName() == "Set")) {
        registerOwned(value, CleanupKind::Free);
        dropFunctions[value] = dictFreeFunction();
        return;
    }
    if (llvm::Function* drop = arrayDropFunction(type)) {
        registerOwned(value, CleanupKind::Free);
        dropFunctions[value] = drop;
        return;
    }
    if (TensorRuntime::isTensorTypeName(type->getName())) registerOwned(value, CleanupKind::TensorFree);
    else if (type->getKind() == Type::Kind::BOX || ownedArrayType(type)) registerOwned(value, CleanupKind::Free);
    else if (type->getKind() == Type::Kind::GENERIC && type->getName() == "Shared") registerOwned(value, CleanupKind::SharedRelease);
    else if (type->getKind() == Type::Kind::GENERIC && type->getName() == "Weak") registerOwned(value, CleanupKind::WeakRelease);
    else if (ownedReferenceClass(type)) registerOwned(value, CleanupKind::Free);
}

void CodeGenerator::releaseFromCurrentScope(llvm::Value* value) {
    if (cleanupScopes.empty() || !value) return;
    std::unordered_set<llvm::Value*> retained;
    collectRetained(value, retained, false);
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

llvm::AllocaInst* CodeGenerator::createEntryAlloca(llvm::Type* type, const std::string& name, llvm::Value* count) {
    llvm::Function* fn = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock& entry = fn->getEntryBlock();
    llvm::IRBuilder<> at(&entry, entry.getFirstInsertionPt());
    return at.CreateAlloca(type, count, name);
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

llvm::FunctionCallee CodeGenerator::rcFunction(const char* name) {
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    requireSystemNative();
    return module->getOrInsertFunction(name, llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false));
}

void CodeGenerator::markStringFunction(const std::string& name, bool runtimeAllocated) {
    if (name.empty()) return;
    if (runtimeAllocated) runtimeStringFunctions.insert(name);
    else userStringFunctions.insert(name);
}

void CodeGenerator::onInstructionInserted(llvm::Instruction* inst) {
    if (stringHookDepth > 0) return;
    auto* call = llvm::dyn_cast<llvm::CallInst>(inst);
    llvm::Function* callee = call ? call->getCalledFunction() : nullptr;
    if (!callee) return;
    const std::string name = callee->getName().str();
    if (auto mask = ownedParameterMasks.find(name); mask != ownedParameterMasks.end()) {
        ++stringHookDepth;
        for (unsigned i = 0; i < call->arg_size() && i < mask->second.size(); ++i) {
            if (mask->second[i]) transferTemporary(call->getArgOperand(i));
        }
        --stringHookDepth;
    }
    if (auto counted = structResultFunctions.find(name); counted != structResultFunctions.end() &&
        call->getType()->isStructTy() && !temporaryFrames.empty() && temporaryFrames.back().function == call->getFunction()) {
        ++stringHookDepth;
        llvm::AllocaInst* spill = createEntryAlloca(call->getType(), "tmp.struct");
        builder.CreateStore(call, spill);
        registerOwnedTemporary(spill, counted->second);
        --stringHookDepth;
        return;
    }
    if (!call->getType()->isPointerTy()) return;
    if (auto owned = ownedResultFunctions.find(name); owned != ownedResultFunctions.end()) {
        ++stringHookDepth;
        registerOwnedTemporary(call, owned->second);
        --stringHookDepth;
        return;
    }
    if (std::find(dictProducerNames.begin(), dictProducerNames.end(), name) != dictProducerNames.end()) {
        ++stringHookDepth;
        registerOwnedTemporary(call, dictFreeFunction());
        --stringHookDepth;
        return;
    }
    const bool runtime = runtimeStringFunctions.count(name) != 0;
    if (!runtime && !userStringFunctions.count(name)) return;
    ++stringHookDepth;
    if (runtime) builder.CreateCall(rcFunction("csec_rc_adopt"), { call });
    --stringHookDepth;
    registerStringTemporary(call);
}

void CodeGenerator::registerStringTemporary(llvm::CallInst* call) {
    if (!call || !call->getType()->isPointerTy()) return;
    ++stringHookDepth;
    llvm::Function* fn = call->getFunction();
    if (!temporaryFrames.empty() && temporaryFrames.back().function == fn) {
        TemporaryFrame& frame = temporaryFrames.back();
        auto* ptrTy = llvm::PointerType::getUnqual(context);
        llvm::AllocaInst* slot = createEntryAlloca(ptrTy, "tmp.string.slot");
        llvm::IRBuilder<> init(slot->getParent(), std::next(slot->getIterator()));
        init.CreateStore(llvm::ConstantPointerNull::get(ptrTy), slot);
        builder.CreateStore(call, slot);
        frame.stringSlots.push_back(slot);
        frame.strings[call] = slot;
    }
    --stringHookDepth;
}

void CodeGenerator::emitStringSlotRelease(llvm::AllocaInst* slot) {
    ++stringHookDepth;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    llvm::Value* held = builder.CreateLoad(ptrTy, slot, "tmp.string.held");
    auto drop = temporaryDrops.find(slot);
    if (drop != temporaryDrops.end() && drop->second) builder.CreateCall(drop->second, { held });
    else builder.CreateCall(rcFunction("csec_rc_release"), { held });
    builder.CreateStore(llvm::ConstantPointerNull::get(ptrTy), slot);
    --stringHookDepth;
}

llvm::Value* CodeGenerator::takeOwnedString(llvm::Value* value) {
    llvm::BasicBlock* block = builder.GetInsertBlock();
    if (!value || !block || !value->getType()->isPointerTy() || llvm::isa<llvm::ConstantPointerNull>(value)) return value;
    llvm::Function* fn = block->getParent();
    ++stringHookDepth;
    for (auto frameIt = temporaryFrames.rbegin(); frameIt != temporaryFrames.rend(); ++frameIt) {
        if (frameIt->function != fn) continue;
        auto found = frameIt->strings.find(value);
        if (found == frameIt->strings.end()) continue;
        builder.CreateStore(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context)), found->second);
        frameIt->strings.erase(found);
        --stringHookDepth;
        return value;
    }
    builder.CreateCall(rcFunction("csec_rc_retain"), { value });
    --stringHookDepth;
    return value;
}

bool CodeGenerator::transferTemporary(llvm::Value* value) {
    llvm::BasicBlock* block = builder.GetInsertBlock();
    if (!value || !block) return false;
    llvm::Function* fn = block->getParent();
    for (auto frameIt = temporaryFrames.rbegin(); frameIt != temporaryFrames.rend(); ++frameIt) {
        if (frameIt->function != fn) continue;
        auto found = frameIt->strings.find(value);
        if (found == frameIt->strings.end()) continue;
        ++stringHookDepth;
        builder.CreateStore(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context)), found->second);
        --stringHookDepth;
        frameIt->strings.erase(found);
        return true;
    }
    return false;
}

void CodeGenerator::registerOwnedTemporary(llvm::Value* value, llvm::Function* drop) {
    llvm::BasicBlock* block = builder.GetInsertBlock();
    if (!value || !drop || !block || !value->getType()->isPointerTy()) return;
    if (temporaryFrames.empty() || temporaryFrames.back().function != block->getParent()) return;
    TemporaryFrame& frame = temporaryFrames.back();
    ++stringHookDepth;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    llvm::AllocaInst* slot = createEntryAlloca(ptrTy, "tmp.owned.slot");
    llvm::IRBuilder<> init(slot->getParent(), std::next(slot->getIterator()));
    init.CreateStore(llvm::ConstantPointerNull::get(ptrTy), slot);
    llvm::Value* previous = builder.CreateLoad(ptrTy, slot, "tmp.owned.previous");
    builder.CreateCall(drop, { previous });
    builder.CreateStore(value, slot);
    temporaryDrops[slot] = drop;
    frame.stringSlots.push_back(slot);
    frame.strings[value] = slot;
    --stringHookDepth;
}

void CodeGenerator::markOwnedResultFunction(const std::string& name, llvm::Function* drop) {
    if (!name.empty() && drop) ownedResultFunctions[name] = drop;
}

void CodeGenerator::markOwnedParameters(const std::string& name, std::vector<bool> mask) {
    bool any = false;
    for (bool owned : mask) any = any || owned;
    if (!name.empty() && any) ownedParameterMasks[name] = std::move(mask);
}

std::vector<std::unique_ptr<Type>> CodeGenerator::structFieldTypes(const Type* type) {
    std::vector<std::unique_ptr<Type>> result;
    if (!type || !isStructClassType(type)) return result;
    auto found = declaredStructFields.find(type->getName());
    if (found != declaredStructFields.end()) {
        for (const auto& field : found->second) result.push_back(field ? field->clone() : nullptr);
        return result;
    }
    std::string templateName;
    std::vector<std::unique_ptr<Type>> arguments;
    if (!templateInstanceArguments(type->getName(), &templateName, &arguments)) return result;
    auto* templateSymbol = dynamic_cast<TemplateSymbol*>(symbolTable.lookup(templateName));
    auto* declaration = templateSymbol ? dynamic_cast<ClassDeclarationNode*>(templateSymbol->declaration.get()) : nullptr;
    if (!declaration) return result;
    auto substitute = [&](const std::unique_ptr<Type>& fieldType) -> std::unique_ptr<Type> {
        if (!fieldType) return nullptr;
        for (size_t k = 0; k < templateSymbol->typeParameters.size() && k < arguments.size(); ++k) {
            if (fieldType->getName() == templateSymbol->typeParameters[k] && arguments[k]) return arguments[k]->clone();
        }
        return fieldType->clone();
    };
    for (auto& field : declaration->constructorParams) {
        if (auto* f = dynamic_cast<ParameterNode*>(field.get())) result.push_back(substitute(f->type));
    }
    if (auto* body = dynamic_cast<ClassBodyNode*>(declaration->body.get())) {
        for (auto& field : body->fields) {
            if (auto* f = dynamic_cast<VariableDeclarationNode*>(field.get())) result.push_back(substitute(f->type));
        }
    }
    return result;
}

bool CodeGenerator::structHoldsCounted(const Type* type, int depth) {
    if (!isStrictOwnership() || depth > 8 || !type || !isStructClassType(type)) return false;
    for (const auto& field : structFieldTypes(type)) {
        if (!field) continue;
        if (isRefcountedType(field.get())) return true;
        if (isStructClassType(field.get()) && structHoldsCounted(field.get(), depth + 1)) return true;
    }
    return false;
}

llvm::Function* CodeGenerator::structCountFunction(const Type* type, bool retain) {
    if (!structHoldsCounted(type)) return nullptr;
    const std::string name = std::string(retain ? "__csec_struct_retain." : "__csec_struct_release.") + type->getName();
    if (llvm::Function* existing = module->getFunction(name)) return existing;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    auto* fn = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false),
                                      llvm::Function::InternalLinkage, name, module.get());
    auto* layout = llvm::dyn_cast_or_null<llvm::StructType>(getLLVMType(type));
    llvm::IRBuilderBase::InsertPointGuard guard(builder);
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    ++stringHookDepth;
    auto* entryBB = llvm::BasicBlock::Create(context, "entry", fn);
    auto* bodyBB = llvm::BasicBlock::Create(context, "count", fn);
    auto* doneBB = llvm::BasicBlock::Create(context, "done", fn);
    builder.SetInsertPoint(entryBB);
    builder.CreateCondBr(builder.CreateIsNull(fn->getArg(0)), doneBB, bodyBB);
    builder.SetInsertPoint(bodyBB);
    if (layout && !layout->isOpaque()) {
        auto fields = structFieldTypes(type);
        for (unsigned i = 0; i < fields.size() && i < layout->getNumElements(); ++i) {
            const Type* field = fields[i].get();
            if (!field) continue;
            llvm::Value* fieldPtr = builder.CreateStructGEP(layout, fn->getArg(0), i, "struct.field");
            if (isRefcountedType(field)) {
                llvm::Value* value = builder.CreateLoad(ptrTy, fieldPtr, "struct.value");
                builder.CreateCall(rcFunction(retain ? "csec_rc_retain" : "csec_rc_release"), { value });
            }
            else if (isStructClassType(field)) {
                if (llvm::Function* nested = structCountFunction(field, retain)) builder.CreateCall(nested, { fieldPtr });
            }
        }
    }
    builder.CreateBr(doneBB);
    builder.SetInsertPoint(doneBB);
    builder.CreateRetVoid();
    --stringHookDepth;
    return fn;
}

void CodeGenerator::emitStructCount(llvm::Value* storage, const Type* type, bool retain) {
    llvm::Function* fn = structCountFunction(type, retain);
    if (!fn || !storage || !builder.GetInsertBlock()) return;
    llvm::Value* pointer = storage;
    if (!storage->getType()->isPointerTy()) {
        pointer = createEntryAlloca(storage->getType(), "struct.copy");
        builder.CreateStore(storage, pointer);
    }
    ++stringHookDepth;
    builder.CreateCall(fn, { pointer });
    --stringHookDepth;
}

llvm::Function* CodeGenerator::structHeapDropFunction(const Type* type) {
    llvm::Function* release = structCountFunction(type, false);
    if (!release) return nullptr;
    const std::string name = "__csec_struct_heap_drop." + type->getName();
    if (llvm::Function* existing = module->getFunction(name)) return existing;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    auto* fn = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false),
                                      llvm::Function::InternalLinkage, name, module.get());
    llvm::IRBuilderBase::InsertPointGuard guard(builder);
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    ++stringHookDepth;
    builder.SetInsertPoint(llvm::BasicBlock::Create(context, "entry", fn));
    builder.CreateCall(release, { fn->getArg(0) });
    builder.CreateCall(freeFunction, { fn->getArg(0) });
    builder.CreateRetVoid();
    --stringHookDepth;
    return fn;
}

bool CodeGenerator::ownsParameter(const std::string& function, unsigned index) const {
    auto found = ownedParameterMasks.find(function);
    return found != ownedParameterMasks.end() && index < found->second.size() && found->second[index];
}

bool CodeGenerator::isRefcountedType(const Type* type) {
    if (!type) return false;
    if (type->getName() == "String" || type->getName() == "Nat") return true;
    if (type->getKind() == Type::Kind::FUNCTION) return isStrictOwnership();
    if (isInterfaceType(type)) return isStrictOwnership();
    const EnumInfo* info = enumOfType(type);
    return info && info->isReference;
}

llvm::Function* CodeGenerator::rcReleaseFunction() {
    return llvm::cast<llvm::Function>(rcFunction("csec_rc_release").getCallee());
}

void CodeGenerator::registerArrayElementsCleanup(llvm::Value* array, const Type* elementType, unsigned count) {
    if (!array || !elementType || count == 0) return;
    const bool counted = isRefcountedType(elementType);
    llvm::Function* structRelease = counted ? nullptr : structCountFunction(elementType, false);
    if (!counted && !structRelease && typeIsCopy(elementType)) return;
    if (ownedArrayType(elementType)) return;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    static int arrayReleaseCount = 0;
    llvm::Function* fn = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false),
                                                llvm::Function::InternalLinkage,
                                                "__csec_array_release." + std::to_string(arrayReleaseCount++), module.get());
    {
        llvm::IRBuilderBase::InsertPointGuard guard(builder);
        builder.SetCurrentDebugLocation(llvm::DebugLoc());
        ++stringHookDepth;
        builder.SetInsertPoint(llvm::BasicBlock::Create(context, "entry", fn));
        llvm::Type* storage = structRelease ? getLLVMType(elementType) : nullptr;
        for (unsigned i = 0; i < count; ++i) {
            if (structRelease) {
                builder.CreateCall(structRelease, { builder.CreateGEP(storage, fn->getArg(0), builder.getInt64(i)) });
                continue;
            }
            llvm::Value* element = builder.CreateLoad(ptrTy, builder.CreateGEP(ptrTy, fn->getArg(0), builder.getInt64(i)), "element");
            if (counted) releaseString(element);
            else emitOwnedValueDrop(element, elementType);
        }
        builder.CreateRetVoid();
        --stringHookDepth;
    }
    registerOwned(array, CleanupKind::Free);
    dropFunctions[array] = fn;
}

void CodeGenerator::registerStructCleanup(llvm::Value* storage, llvm::Function* release) {
    if (!storage || !release) return;
    registerOwned(storage, CleanupKind::Free);
    dropFunctions[storage] = release;
}

void CodeGenerator::registerHandleSlotCleanup(llvm::Value* slot, bool weak) {
    registerOwned(slot, weak ? CleanupKind::WeakSlotRelease : CleanupKind::SharedSlotRelease);
}

void CodeGenerator::registerOwnedSlotCleanup(llvm::Value* slot, llvm::Function* drop) {
    if (!slot || !drop) return;
    registerOwned(slot, CleanupKind::OwnedSlotDrop);
    dropFunctions[slot] = drop;
}

llvm::Function* CodeGenerator::enumFieldReleaseFunction(const Type* type) {
    if (!isRefcountedType(type) || type->getName() == "String") return nullptr;
    ClassSymbol* classSymbol = symbolTable.lookupClass(type->getName());
    auto* layout = classSymbol ? llvm::dyn_cast_or_null<llvm::StructType>(classSymbol->classType) : nullptr;
    if (!layout || layout->isOpaque()) return nullptr;
    const std::string name = "__csec_enum_release." + type->getName();
    if (llvm::Function* existing = module->getFunction(name)) return existing;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    auto* fn = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false),
                                      llvm::Function::InternalLinkage, name, module.get());
    pendingBodies.push_back({ fn, const_cast<Type*>(type)->clone(), true });
    return fn;
}

void CodeGenerator::emitEnumReleaseBody(llvm::Function* fn, const Type* type) {
    ClassSymbol* classSymbol = symbolTable.lookupClass(type->getName());
    auto* layout = classSymbol ? llvm::dyn_cast_or_null<llvm::StructType>(classSymbol->classType) : nullptr;
    if (!layout || layout->isOpaque()) return;
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    (void)ptrTy;

    std::vector<std::pair<unsigned, std::unique_ptr<Type>>> slots;
    unsigned index = 0;
    for (const auto& paramName : classSymbol->constructorParamOrder) {
        auto found = classSymbol->constructorParams.find(paramName);
        if (found != classSymbol->constructorParams.end() && found->second && found->second->type) {
            slots.emplace_back(index, found->second->type->clone());
        }
        ++index;
    }
    llvm::IRBuilderBase::InsertPointGuard guard(builder);
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    ++stringHookDepth;
    builder.SetInsertPoint(llvm::BasicBlock::Create(context, "entry", fn));
    llvm::Value* object = fn->getArg(0);
    for (auto& slot : slots) {
        if (slot.first >= layout->getNumElements()) continue;
        llvm::Type* storage = layout->getElementType(slot.first);
        if (storage->isStructTy() && structHoldsCounted(slot.second.get())) {
            emitStructCount(builder.CreateStructGEP(layout, object, slot.first), slot.second.get(), false);
            continue;
        }
        if (!storage->isPointerTy()) continue;
        llvm::Value* fieldValue = builder.CreateLoad(storage, builder.CreateStructGEP(layout, object, slot.first), "enum.field");
        if (isRefcountedType(slot.second.get())) builder.CreateCall(rcReleaseFunction(), { fieldValue });
        else if (!typeIsCopy(slot.second.get())) emitOwnedValueDrop(fieldValue, slot.second.get());
    }
    builder.CreateRetVoid();
    --stringHookDepth;
}

llvm::Function* CodeGenerator::ownedValueDropFunction(const Type* type) {
    if (!type) return nullptr;
    if (type->getKind() == Type::Kind::GENERIC && (type->getName() == "Dict" || type->getName() == "Set")) {
        return dictFreeFunction();
    }
    return classDropFunction(type);
}

llvm::Value* CodeGenerator::takeOwned(llvm::Value* value, const Type* type) {
    if (value && builder.GetInsertBlock() && structHoldsCounted(type)) {
        emitStructCount(value, type, true);
        return value;
    }
    if (!value || !value->getType()->isPointerTy() || !builder.GetInsertBlock()) return value;
    if (isRefcountedType(type)) return takeOwnedString(value);
    if (transferTemporary(value)) return value;
    if (auto* phi = llvm::dyn_cast<llvm::PHINode>(value)) {
        for (llvm::Value* incoming : phi->incoming_values()) transferTemporary(incoming);
    }
    else if (auto* select = llvm::dyn_cast<llvm::SelectInst>(value)) {
        transferTemporary(select->getTrueValue());
        transferTemporary(select->getFalseValue());
    }
    return value;
}

llvm::Value* CodeGenerator::adoptRuntimeString(llvm::Value* text) {
    if (!text || !text->getType()->isPointerTy() || !builder.GetInsertBlock()) return text;
    ++stringHookDepth;
    builder.CreateCall(rcFunction("csec_rc_adopt"), { text });
    llvm::Function* fn = builder.GetInsertBlock()->getParent();
    if (!temporaryFrames.empty() && temporaryFrames.back().function == fn) {
        TemporaryFrame& frame = temporaryFrames.back();
        auto* ptrTy = llvm::PointerType::getUnqual(context);
        llvm::AllocaInst* slot = createEntryAlloca(ptrTy, "tmp.string.slot");
        llvm::IRBuilder<> init(slot->getParent(), std::next(slot->getIterator()));
        init.CreateStore(llvm::ConstantPointerNull::get(ptrTy), slot);
        llvm::Value* previous = builder.CreateLoad(ptrTy, slot, "tmp.string.previous");
        builder.CreateCall(rcFunction("csec_rc_release"), { previous });
        builder.CreateStore(text, slot);
        frame.stringSlots.push_back(slot);
        frame.strings[text] = slot;
    }
    --stringHookDepth;
    return text;
}

void CodeGenerator::promoteTemporaryString(llvm::Value* value) {
    if (!value || temporaryFrames.size() < 2) return;
    TemporaryFrame& current = temporaryFrames.back();
    TemporaryFrame& outer = temporaryFrames[temporaryFrames.size() - 2];
    if (outer.function != current.function) return;
    auto found = current.strings.find(value);
    if (found == current.strings.end()) return;
    llvm::AllocaInst* slot = found->second;
    current.strings.erase(found);
    current.stringSlots.erase(std::remove(current.stringSlots.begin(), current.stringSlots.end(), slot),
                              current.stringSlots.end());
    outer.stringSlots.push_back(slot);
    outer.strings[value] = slot;
}

void CodeGenerator::elideConstantRefcounts() {
    for (const char* name : { "csec_rc_retain", "csec_rc_release" }) {
        llvm::Function* fn = module->getFunction(name);
        if (!fn) continue;
        std::vector<llvm::CallInst*> dead;
        for (llvm::User* user : fn->users()) {
            auto* call = llvm::dyn_cast<llvm::CallInst>(user);
            if (call && call->getCalledFunction() == fn && call->arg_size() == 1 &&
                llvm::isa<llvm::Constant>(call->getArgOperand(0)->stripPointerCasts())) {
                dead.push_back(call);
            }
        }
        for (llvm::CallInst* call : dead) call->eraseFromParent();
    }
}

void CodeGenerator::releaseString(llvm::Value* value) {
    if (!value || !builder.GetInsertBlock() || !value->getType()->isPointerTy()) return;
    ++stringHookDepth;
    builder.CreateCall(rcFunction("csec_rc_release"), { value });
    --stringHookDepth;
}

llvm::Function* CodeGenerator::dictFreeFunction() {
    requireSystemNative();
    auto* ptrTy = llvm::PointerType::getUnqual(context);
    llvm::FunctionCallee callee = module->getOrInsertFunction(
        "csec_dict_free", llvm::FunctionType::get(llvm::Type::getVoidTy(context), { ptrTy }, false));
    return llvm::cast<llvm::Function>(callee.getCallee());
}

void CodeGenerator::registerDictSlotCleanup(llvm::Value* slot) {
    registerOwned(slot, CleanupKind::DictSlotFree);
}

void CodeGenerator::registerTensorSlotCleanup(llvm::Value* slot) {
    registerOwned(slot, CleanupKind::TensorSlotDrop);
}

void CodeGenerator::dropTensorValue(llvm::Value* tensor) {
    if (!tensor || !builder.GetInsertBlock()) return;
    builder.CreateCall(getTensorDropFunction(), { tensor });
}

void CodeGenerator::registerStringSlotCleanup(llvm::Value* slot) {
    registerOwned(slot, CleanupKind::StringSlotRelease);
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
    for (auto it = frame.stringSlots.rbegin(); it != frame.stringSlots.rend(); ++it) {
        emitStringSlotRelease(*it);
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
        for (auto it = frameIt->stringSlots.rbegin(); it != frameIt->stringSlots.rend(); ++it) {
            emitStringSlotRelease(*it);
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

void CodeGenerator::finalizeOwnershipFunctions() {
    for (size_t i = 0; i < pendingBodies.size(); ++i) {
        llvm::Function* fn = pendingBodies[i].function;
        if (!fn->empty()) continue;
        std::unique_ptr<Type> type = pendingBodies[i].type->clone();
        if (pendingBodies[i].enumRelease) emitEnumReleaseBody(fn, type.get());
        else emitClassDropBody(fn, type.get());
        if (fn->empty()) {
            llvm::IRBuilderBase::InsertPointGuard guard(builder);
            builder.SetInsertPoint(llvm::BasicBlock::Create(context, "entry", fn));
            if (!pendingBodies[i].enumRelease) builder.CreateCall(freeFunction, { fn->getArg(0) });
            builder.CreateRetVoid();
        }
    }
}
