// codegen.cpp
#include "codegen.h"
#include "TensorRuntime.h"
#include "type_utils.h"
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
        cleanupScopes.pop_back();
    }
}

void CodeGenerator::registerCleanup(llvm::Value* pointer) {
    if (!pointer || cleanupScopes.empty()) {
        return;
    }
    cleanupScopes.back().push_back({ pointer, CleanupKind::Free });
}

void CodeGenerator::registerTensorCleanup(llvm::Value* tensor) {
    if (!tensor || cleanupScopes.empty()) {
        return;
    }
    cleanupScopes.back().push_back({ tensor, CleanupKind::TensorFree });
}

void CodeGenerator::forgetCleanup(llvm::Value* pointer) {
    if (!pointer) return;
    for (auto& scope : cleanupScopes) {
        scope.erase(std::remove_if(scope.begin(), scope.end(), [&](const auto& entry) {
            return entry.first == pointer;
        }), scope.end());
    }
}

void CodeGenerator::registerSharedCleanup(llvm::Value* controlBlock) {
    if (!controlBlock || cleanupScopes.empty()) {
        return;
    }
    cleanupScopes.back().push_back({ controlBlock, CleanupKind::SharedRelease });
}

void CodeGenerator::registerWeakCleanup(llvm::Value* controlBlock) {
    if (!controlBlock || cleanupScopes.empty()) {
        return;
    }
    cleanupScopes.back().push_back({ controlBlock, CleanupKind::WeakRelease });
}

void CodeGenerator::emitCleanupEntry(llvm::Value* pointer, CleanupKind kind, llvm::Value* retainedPointer) {
    if (!pointer || pointer == retainedPointer) {
        return;
    }
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
        llvm::Function* fn = builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        if (!fn) return;
        llvm::Value* strongPtr = pointer;  // strong is field 0 (offset 0)
        llvm::Value* weakPtr = builder.CreateGEP(i64Ty, pointer, llvm::ConstantInt::get(i64Ty, 1), "block.weak.ptr");
        llvm::Value* counterPtr = kind == CleanupKind::SharedRelease ? strongPtr : weakPtr;
        llvm::Value* count = builder.CreateLoad(i64Ty, counterPtr, "refcount");
        llvm::Value* decremented = builder.CreateSub(count, llvm::ConstantInt::get(i64Ty, 1), "refcount.dec");
        builder.CreateStore(decremented, counterPtr);
        llvm::Value* strong = builder.CreateLoad(i64Ty, strongPtr, "block.strong");
        llvm::Value* weak = builder.CreateLoad(i64Ty, weakPtr, "block.weak");
        llvm::Value* zero = llvm::ConstantInt::get(i64Ty, 0);
        llvm::Value* bothZero = builder.CreateAnd(
            builder.CreateICmpEQ(strong, zero, "strong.zero"),
            builder.CreateICmpEQ(weak, zero, "weak.zero"), "block.dead");
        auto* freeBB = llvm::BasicBlock::Create(context, "block.free", fn);
        auto* contBB = llvm::BasicBlock::Create(context, "block.cont", fn);
        builder.CreateCondBr(bothZero, freeBB, contBB);
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
    auto& cleanups = cleanupScopes.back();
    for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) {
        emitCleanupEntry(it->first, it->second, nullptr);
    }
    cleanups.clear();
}

void CodeGenerator::emitAllCleanups() {
    emitAllCleanupsExcept(nullptr);
}

void CodeGenerator::emitAllCleanupsExcept(llvm::Value* retainedPointer) {
    // Emit drops for every live owned local along THIS return path, innermost scope first. Unlike a
    // normal scope exit, the lists are NOT cleared: a `return` is one runtime path among several, so
    // each early return and the fall-through exit must independently emit their own frees. Clearing
    // here made the first return consume the registry and leak the locals on every later path (e.g.
    // `if (c) { return 1; } return 0;` freed the box only on the `then` path). A single runtime path
    // reaches either a return or its block's normal scope-exit cleanup, never both, so leaving the
    // entries in place frees each local exactly once per path (M4 per-path drop elaboration).
    for (auto scopeIt = cleanupScopes.rbegin(); scopeIt != cleanupScopes.rend(); ++scopeIt) {
        auto& cleanups = *scopeIt;
        for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) {
            emitCleanupEntry(it->first, it->second, retainedPointer);
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
        auto* classSymbol = symbolTable.lookupClass(type->getName());
        if (classSymbol && !classSymbol->classType && materializeTemplateClass(type->getName())) {
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
                mangledName += "$" + (ta ? ta->getName() : "Unknown");
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
