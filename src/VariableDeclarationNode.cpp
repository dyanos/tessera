#include "codegen.h"
#include "type_checker.h"
#include "IdentifierNode.h"
#include "AccessFieldNode.h"

#include "VariableDeclarationNode.h"
#include "ASTVisitor.h"
#include "ClassInstanceCreationNode.h"
#include "ArrayCreationExpressionNode.h"
#include "ArrayLiteralNode.h"
#include "CallExpressionNode.h"
#include "FunctionCallNode.h"
#include "LambdaExpressionNode.h"
#include "MapStatementNode.h"
#include "FilterStatementNode.h"
#include "PMapStatementNode.h"
#include "PrefixExpressionNode.h"
#include "type_utils.h"
#include "dict_support.h"
#include "ArrayAccessNode.h"
#include "ArrayCreationNode.h"
#include "MethodCallNode.h"

#include <iostream>
#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>

namespace {
llvm::Value* coerceBoxValue(llvm::Value* value, llvm::Type* targetType) {
    if (!value || !targetType || value->getType() == targetType) {
        return value;
    }

    auto& cg = CodeGenerator::getInstance();
    llvm::Type* sourceType = value->getType();
    if (sourceType->isIntegerTy() && targetType->isIntegerTy()) {
        unsigned sourceBits = sourceType->getIntegerBitWidth();
        unsigned targetBits = targetType->getIntegerBitWidth();
        if (sourceBits < targetBits) {
            return cg.builder.CreateSExt(value, targetType, "box.sext");
        }
        return cg.builder.CreateTrunc(value, targetType, "box.trunc");
    }

    if (sourceType->isFloatingPointTy() && targetType->isFloatingPointTy()) {
        return cg.builder.CreateFPCast(value, targetType, "box.fpcast");
    }

    if (sourceType->isIntegerTy() && targetType->isFloatingPointTy()) {
        return cg.builder.CreateSIToFP(value, targetType, "box.sitofp");
    }

    if (sourceType->isFloatingPointTy() && targetType->isIntegerTy()) {
        return cg.builder.CreateFPToSI(value, targetType, "box.fptosi");
    }

    return value;
}
}

void VariableDeclarationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* VariableDeclarationNode::codegen() {
    auto& cg = CodeGenerator::getInstance();

    llvm::Value* initValue = nullptr;
    if (this->initializer) {
        initValue = this->initializer->codegen();
        if (!initValue) {
            return nullptr;
        }
    }
    else {
        initValue = llvm::Constant::getNullValue(cg.getLLVMType(type.get()));
    }

    if (this->type->getKind() == Type::Kind::UNKNOWN && initValue != nullptr) {
        if (auto* lambda = dynamic_cast<LambdaExpressionNode*>(initializer.get())) {
            this->type = lambda->getType();
        }
        else if (initValue->getType()->isIntegerTy(32)) {
            this->type = std::make_unique<BasicType>("Int");
        }
        else if (initValue->getType()->isIntegerTy(64)) {
            this->type = std::make_unique<BasicType>("Long");
        }
        else if (initValue->getType()->isIntegerTy(1)) {
            this->type = std::make_unique<BasicType>("Boolean");
        }
        else if (initValue->getType()->isIntegerTy(8)) {
            this->type = std::make_unique<BasicType>("Char");
        }
        else if (initValue->getType()->isIntegerTy(16)) {
            this->type = std::make_unique<BasicType>("Short");
        }
        else if (initValue->getType()->isFloatTy()) {
            this->type = std::make_unique<BasicType>("Float");
        }
        else if (initValue->getType()->isDoubleTy()) {
            this->type = std::make_unique<BasicType>("Double");
        }
        else if (initValue->getType()->isStructTy()) {
            std::string structName = initValue->getType()->getStructName().str();
            this->type = std::make_unique<ClassType>(structName);
        }
        else if (auto* classCreation = dynamic_cast<ClassInstanceCreationNode*>(initializer.get())) {
            this->type = classCreation->getType();
        }
        else if (auto* arrayCreation = dynamic_cast<ArrayCreationExpressionNode*>(initializer.get())) {
            this->type = arrayCreation->getType();
        }
        else if (auto initializerType = initializer->getType();
                 initializerType && initializerType->getKind() != Type::Kind::UNKNOWN &&
                 initValue->getType()->isPointerTy()) {
            // A pointer-valued initializer with a known type (a Dict, a list, ...): keep that type
            // rather than assuming a String.
            this->type = std::move(initializerType);
        }
        // string type (opaque pointer)
        else if (initValue->getType()->isPointerTy()) {
            this->type = std::make_unique<BasicType>("String");
        }
        else {
            std::cerr << "Error: Unable to infer variable type for '" << name << "'" << std::endl;
            return nullptr;
        }
    }

    // Where a Nat is declared but an integer initializes it (`val n: Nat = 5`, or a literal larger than
    // 64 bits), promote the value to an arbitrary-precision handle before it is stored/bound.
    initValue = coerceToNat(initValue, type.get(), this->initializer.get());
    if (initValue && (initValue->getType()->isPointerTy() || cg.structHoldsCounted(type.get())) &&
        (!type || type->getName() != "String") && !borrowsValue) {
        initValue = cg.takeOwned(initValue, type.get());
    }

    // A Nat variable lives in a slot holding its handle (a ptr), so a `var` can be reassigned
    // (`fact = fact * i`). Reads load the handle (IdentifierNode); assignments store a new one.
    if (type && type->getName() == "Nat") {
        auto* handleTy = llvm::PointerType::getUnqual(cg.context);
        if (!initValue || !initValue->getType()->isPointerTy()) {
            initValue = llvm::ConstantPointerNull::get(handleTy);
        }
        llvm::Value* slot = cg.builder.CreateAlloca(handleTy, nullptr, name + ".nat");
        cg.builder.CreateStore(initValue, slot);
        cg.symbolTable.addSymbol(
            name, std::make_unique<Symbol>(name, type->clone(), slot, isMutable, SymbolType::VARIABLE));
        return slot;
    }

    if (type && type->getKind() == Type::Kind::BOX && initValue && !initValue->getType()->isPointerTy()) {
        auto* boxType = dynamic_cast<BoxType*>(type.get());
        llvm::Type* baseType = boxType && boxType->baseType ? cg.getLLVMType(boxType->baseType.get()) : nullptr;
        if (!baseType) {
            std::cerr << "Error: Unsupported box base type for '" << name << "'" << std::endl;
            return nullptr;
        }

        initValue = coerceBoxValue(initValue, baseType);
        if (!initValue || initValue->getType() != baseType) {
            std::cerr << "Error: Box initializer for '" << name << "' has incompatible type" << std::endl;
            return nullptr;
        }

        const llvm::DataLayout& dl = cg.module->getDataLayout();
        uint64_t typeSize = dl.getTypeAllocSize(baseType);
        llvm::Value* allocSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(cg.context), typeSize);
        llvm::Value* rawPtr = cg.builder.CreateCall(cg.mallocFunction, allocSize, "box.malloc");
        llvm::Value* boxPtr = cg.builder.CreateBitCast(rawPtr, llvm::PointerType::getUnqual(baseType->getContext()), "box.ptr");
        cg.builder.CreateStore(initValue, boxPtr);
        initValue = boxPtr;
    }

    const bool declaredStructClass = isStructClassType(type.get());
    if (declaredStructClass && initValue && initValue->getType()->isPointerTy() &&
        dynamic_cast<ClassInstanceCreationNode*>(initializer.get()) == nullptr) {
        llvm::Type* valueType = cg.getLLVMType(type.get());
        initValue = coerceValueToLLVMType(initValue, valueType);
    }

    // An array or vector local initialized from a function call holds the returned array pointer.
    // Bind it directly (rather than into a pointer slot) so indexing can GEP on the pointer
    // without an intervening load, the same representation a `new T[n]` initializer produces.
    // This is restricted to call initializers so that array literals, which the parallel
    // collection lowering re-evaluates through their own node, keep their existing representation.
    // A list read out of a dictionary (`d[k]`, `d.get(k, ...)`, `d.valueAt(i)`) is likewise a
    // bare buffer pointer.
    auto readsFromDict = [&]() {
        if (auto* access = dynamic_cast<ArrayAccessNode*>(initializer.get())) {
            auto container = access->array ? access->array->getType() : nullptr;
            return isDictType(container.get());
        }
        if (auto* call = dynamic_cast<MethodCallNode*>(initializer.get())) {
            auto receiver = call->object ? call->object->getType() : nullptr;
            return isDictType(receiver.get());
        }
        return false;
    };
    const bool arrayVectorFromCall = type &&
        (type->getKind() == Type::Kind::ARRAY ||
         (type->getKind() == Type::Kind::GENERIC &&
          (type->getName() == "Array" || type->getName() == "Vector"))) &&
        (dynamic_cast<CallExpressionNode*>(initializer.get()) != nullptr ||
         dynamic_cast<FunctionCallNode*>(initializer.get()) != nullptr ||
         readsFromDict());

    // Keep a moved tensor as the original pointer value. Storing it in a new pointer slot would
    // make a later return use a different LLVM Value and defeat cleanup ownership transfer.
    const auto* prefixInitializer = dynamic_cast<PrefixExpressionNode*>(initializer.get());
    const bool tensorMoveInitializer = type && type->getName() == "Tensor" &&
        prefixInitializer && prefixInitializer->op == "<-";
    llvm::BasicBlock* declBlock = cg.builder.GetInsertBlock();
    const bool ownsTensorResult = cg.isFreshTensor(initValue) ||
        (isStrictOwnership() && !dynamic_cast<IdentifierNode*>(initializer.get()) &&
         !dynamic_cast<AccessFieldNode*>(initializer.get()) &&
         !(prefixInitializer && (prefixInitializer->op == "&" || prefixInitializer->op == "&mut")));
    const bool freshTensorInitializer = autoDrop && type && type->getName() == "Tensor" &&
        initValue && initValue->getType()->isPointerTy() && ownsTensorResult &&
        declBlock && declBlock->getParent() != cg.mainFunction;

    const bool bindPointerBackedValueDirectly =
        initValue &&
        initValue->getType()->isPointerTy() &&
        !(isMutable && type && type->getName() == "Tensor") &&
        !(isMutable && type && type->getKind() == Type::Kind::CLASS && !declaredStructClass) &&
        (dynamic_cast<ClassInstanceCreationNode*>(initializer.get()) != nullptr ||
         dynamic_cast<ArrayCreationExpressionNode*>(initializer.get()) != nullptr ||
         dynamic_cast<ArrayLiteralNode*>(initializer.get()) != nullptr ||
         // `map`/`filter`/`pmap` yield a bare element-base pointer (a collection buffer); bind it
         // directly like an array literal instead of trying to store it into an aggregate slot,
         // which would fail the LLVM type match and leave the variable unbound.
         dynamic_cast<MapStatementNode*>(initializer.get()) != nullptr ||
         dynamic_cast<FilterStatementNode*>(initializer.get()) != nullptr ||
         dynamic_cast<PMapStatementNode*>(initializer.get()) != nullptr ||
         dynamic_cast<FunctionType*>(type.get()) != nullptr ||
         (type->getKind() == Type::Kind::CLASS && !declaredStructClass) ||
         type->getKind() == Type::Kind::BOX ||
         (!isStrictOwnership() && type->getKind() == Type::Kind::GENERIC && (type->getName() == "Shared" || type->getName() == "Weak")) ||
         arrayVectorFromCall || tensorMoveInitializer || freshTensorInitializer);

    if (bindPointerBackedValueDirectly) {
        if (declaredStructClass && !borrowsValue && declBlock && declBlock->getParent() != cg.mainFunction) {
            if (llvm::Function* release = cg.structCountFunction(type.get(), false)) {
                cg.registerStructCleanup(initValue, release);
            }
        }
        if (auto* literal = dynamic_cast<ArrayLiteralNode*>(initializer.get());
            literal && isStrictOwnership() && !borrowsValue && llvm::isa<llvm::AllocaInst>(initValue) && !literal->elements.empty() &&
            declBlock && declBlock->getParent() != cg.mainFunction && initValue->getType()->isPointerTy()) {
            auto elementType = literal->elements[0]->getType();
            if (type) {
                if (auto* arrayType = dynamic_cast<ArrayType*>(type.get())) {
                    if (arrayType->elementType) elementType = arrayType->elementType->clone();
                }
                else if (auto* generic = dynamic_cast<GenericType*>(type.get())) {
                    if (!generic->typeArguments.empty() && generic->typeArguments[0]) elementType = generic->typeArguments[0]->clone();
                }
            }
            if (elementType && llvm::cast<llvm::AllocaInst>(initValue)->getAllocatedType()->isPointerTy()) {
                cg.registerArrayElementsCleanup(initValue, elementType.get(), static_cast<unsigned>(literal->elements.size()));
            }
        }
        cg.symbolTable.addSymbol(
            name,
            std::make_unique<Symbol>(name, type->clone(), initValue, isMutable, SymbolType::VARIABLE));
        const bool moveInitializer = prefixInitializer && prefixInitializer->op == "<-";
        if (type->getKind() == Type::Kind::BOX) {
            if (moveInitializer) cg.forgetCleanup(initValue);
            cg.registerCleanup(initValue);
        }
        else if (cg.isRefcountedType(type.get()) && isStrictOwnership() && declBlock && !borrowsValue &&
                 declBlock->getParent() != cg.mainFunction) {
            llvm::AllocaInst* holder = cg.createEntryAlloca(initValue->getType(), name + ".owner");
            cg.builder.CreateStore(initValue, holder);
            cg.registerStringSlotCleanup(holder);
        }
        else if (autoDrop && type->getKind() == Type::Kind::CLASS && !declaredStructClass) {
            if (!moveInitializer || cg.hasCleanup(initValue)) cg.registerOwnedValueCleanup(initValue, type.get());
        }
        else if (isStrictOwnership() && !borrowsValue && declBlock && declBlock->getParent() != cg.mainFunction &&
                 (type->getKind() == Type::Kind::ARRAY ||
                  (type->getKind() == Type::Kind::GENERIC && (type->getName() == "Array" || type->getName() == "Vector"))) &&
                 !llvm::isa<llvm::AllocaInst>(initValue) && !readsFromDict() &&
                 (dynamic_cast<ArrayCreationExpressionNode*>(initializer.get()) ||
                  dynamic_cast<ArrayCreationNode*>(initializer.get()) ||
                  dynamic_cast<FunctionCallNode*>(initializer.get()) ||
                  dynamic_cast<CallExpressionNode*>(initializer.get()) ||
                  (moveInitializer && cg.hasCleanup(initValue)))) {
            cg.registerOwnedValueCleanup(initValue, type.get());
        }
        else if (type->getName() == "Tensor") {
            bool sourceOwned = true;
            if (tensorMoveInitializer) {
                // The source and destination denote the same allocation. Transfer its one drop
                // obligation to this binding instead of keeping two stale registrations.
                sourceOwned = cg.forgetCleanup(initValue) || cg.isTensorAllocation(initValue);
            }
            if (autoDrop && sourceOwned) {
                cg.registerTensorCleanup(initValue);
            }
        }
        // Reference-class instances and array/vector locals are intentionally NOT auto-registered for
        // scope-exit free — the attempt to do so was unsafe and is disabled pending a proper ownership
        // model. It (1) double-freed an aliased/moved owner (`val b = a` / `val b = <- a`) since both
        // share one allocation; (2) tripped an LLVM "uses remain when a value is destroyed" assertion
        // when a class instance's type was also a template argument (e.g. `val dog = new Dog(...)`
        // alongside `new Kennel<Dog, 10>(...)`); and (3) hit the same assertion for an array literal
        // later consumed by a map/filter/reduce that re-evaluates it (e.g. 105_full_featured). These
        // are released by explicit `free(...)` only; scope-exit auto-free stays limited to box,
        // tensors, and Shared/Weak below, whose ownership is unambiguous.
        else if (type->getKind() == Type::Kind::GENERIC && type->getName() == "Shared") {
            // Each Shared<T> owner releases (decrements strong) at scope exit.
            cg.registerSharedCleanup(initValue);
        }
        else if (type->getKind() == Type::Kind::GENERIC && type->getName() == "Weak") {
            // Each Weak<T> handle releases (decrements weak) at scope exit.
            cg.registerWeakCleanup(initValue);
        }
        return initValue;
    }

    // constexpr: use constant value directly, no alloca
    if (this->isConstexpr) {
        auto* constVal = llvm::dyn_cast<llvm::Constant>(initValue);
        if (!constVal) {
            std::cerr << "Error: constexpr variable '" << name << "' must have a constant initializer" << std::endl;
            return nullptr;
        }
        cg.symbolTable.addSymbol(
            name,
            std::make_unique<Symbol>(name, type->clone(), constVal, false, SymbolType::VARIABLE));
        return constVal;
    }

    llvm::Type* varType = (type && type->getKind() == Type::Kind::CLASS && !declaredStructClass)
        ? getABIStorageType(type.get()) : cg.getLLVMType(type.get());
    if (!varType) {
        std::cerr << "Error: Unsupported variable type '" << type->getName() << "'" << std::endl;
        return nullptr;
    }
    if (varType->isVoidTy()) {
        cg.symbolTable.addSymbol(
            name,
            std::make_unique<Symbol>(name, type->clone(), nullptr, isMutable, SymbolType::VARIABLE));
        return nullptr;
    }

    initValue = coerceBoxValue(initValue, varType);
    if (!initValue || initValue->getType() != varType) {
        std::cerr << "Error: Variable initializer for '" << name << "' has incompatible LLVM type" << std::endl;
        return nullptr;
    }

    auto* currentSymbol = cg.symbolTable.getCurrentSymbol();
    // A top-level `var` (no enclosing function/class/namespace symbol) lowers to an LLVM global
    // rather than a stack alloca in the bootstrap `main`. Recognise true module scope by both a null
    // current symbol AND the builder still parked in the bootstrap main's block (the constructor's
    // parking spot) — this excludes the transient null-currentSymbol windows that class/instance and
    // call codegen open while emitting field/argument initializers.
    llvm::BasicBlock* insertBB = cg.builder.GetInsertBlock();
    const bool atModuleScope =
        currentSymbol == nullptr && insertBB && insertBB->getParent() == cg.mainFunction;
    const bool inNamespace = currentSymbol && currentSymbol->symbolType == SymbolType::NAMESPACE;
    if (atModuleScope || inNamespace) {
        llvm::Constant* globalInit = llvm::dyn_cast<llvm::Constant>(initValue);
        if (!globalInit) {
            globalInit = llvm::Constant::getNullValue(varType);
        }
        // Module-scope globals are read from other functions, so they need module-visible linkage;
        // namespace-scoped ones stay private as before.
        llvm::GlobalValue::LinkageTypes linkage =
            atModuleScope ? llvm::GlobalValue::InternalLinkage : llvm::GlobalValue::PrivateLinkage;
        auto* global = new llvm::GlobalVariable(
            *cg.module,
            varType,
            !isMutable,
            linkage,
            globalInit,
            name);
        cg.symbolTable.addSymbol(
            name,
            std::make_unique<Symbol>(name, type->clone(), global, isMutable, SymbolType::VARIABLE));
        return global;
    }

    llvm::AllocaInst* alloc = cg.builder.CreateAlloca(varType, nullptr, name.c_str());

    const bool stringLocal = type && type->getName() == "String" && !isField && varType->isPointerTy() && !borrowsValue;
    if (stringLocal) initValue = cg.takeOwnedString(initValue);
    cg.builder.CreateStore(initValue, alloc);
    if (stringLocal) cg.registerStringSlotCleanup(alloc);
    if (declaredStructClass && !borrowsValue && !isField && cg.builder.GetInsertBlock()->getParent() != cg.mainFunction) {
        if (llvm::Function* release = cg.structCountFunction(type.get(), false)) {
            cg.registerStructCleanup(alloc, release);
        }
    }
    if (type && type->getKind() == Type::Kind::GENERIC && (type->getName() == "Shared" || type->getName() == "Weak") &&
        varType->isPointerTy() && isStrictOwnership() && !borrowsValue &&
        cg.builder.GetInsertBlock()->getParent() != cg.mainFunction) {
        cg.registerHandleSlotCleanup(alloc, type->getName() == "Weak");
    }
    const bool referenceLocal = !borrowsValue && type && type->getKind() == Type::Kind::CLASS && !declaredStructClass &&
        varType->isPointerTy() && cg.builder.GetInsertBlock()->getParent() != cg.mainFunction && isStrictOwnership();
    if (referenceLocal && cg.isRefcountedType(type.get())) {
        cg.registerStringSlotCleanup(alloc);
    }
    else if (referenceLocal && autoDrop && !typeIsCopy(type.get())) {
        cg.registerOwnedSlotCleanup(alloc, cg.classDropFunction(type.get()));
    }
    const bool dictLocal = type && type->getKind() == Type::Kind::GENERIC &&
        (type->getName() == "Dict" || type->getName() == "Set");
    if (dictLocal && isStrictOwnership() && varType->isPointerTy() &&
        cg.builder.GetInsertBlock()->getParent() != cg.mainFunction && initializer) {
        bool owning = !dynamic_cast<IdentifierNode*>(initializer.get()) &&
            !dynamic_cast<AccessFieldNode*>(initializer.get()) &&
            !dynamic_cast<ArrayAccessNode*>(initializer.get()) &&
            !(prefixInitializer && (prefixInitializer->op == "&" || prefixInitializer->op == "&mut"));
        if (auto* call = dynamic_cast<MethodCallNode*>(initializer.get())) {
            owning = owning && call->methodName != "get" && call->methodName != "getOr" &&
                     call->methodName != "valueAt" && call->methodName != "value";
        }
        if (owning) cg.registerDictSlotCleanup(alloc);
    }
    if (type && type->getName() == "Tensor" && isMutable && isStrictOwnership() && ownsTensorResult &&
        varType->isPointerTy() && cg.builder.GetInsertBlock()->getParent() != cg.mainFunction) {
        if (!tensorMoveInitializer || cg.forgetCleanup(initValue) || cg.isTensorAllocation(initValue)) {
            cg.registerTensorSlotCleanup(alloc);
        }
    }

    cg.symbolTable.addSymbol(
        name,
        std::make_unique<Symbol>(name, type->clone(), alloc, isMutable, SymbolType::VARIABLE));

    return alloc;
}
