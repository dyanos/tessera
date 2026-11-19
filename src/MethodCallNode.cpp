#include "codegen.h"

#include "MethodCallNode.h"
#include "ASTVisitor.h"
#include "IdentifierNode.h"
#include "type_utils.h"
#include "interfaces.h"
#include "dict_support.h"

#include <iostream>
#include <sstream>
#include <llvm/IR/Constants.h>

#include "FunctionDeclarationNode.h"

llvm::Value* csecCoerceNumericValue(llvm::Value* value, llvm::Type* targetType);  // BinaryExpressionNode.cpp

namespace {
llvm::Value* defaultValueForType(llvm::Type* type) {
    auto& cg = CodeGenerator::getInstance();
    if (!type || type->isVoidTy()) {
        return nullptr;
    }
    if (type->isIntegerTy()) {
        return llvm::ConstantInt::get(type, 0, true);
    }
    if (type->isFloatingPointTy()) {
        return llvm::ConstantFP::get(type, 0.0);
    }
    if (type->isPointerTy()) {
        return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
    }
    return llvm::Constant::getNullValue(type);
}

llvm::Value* coerceArgumentValue(llvm::Value* value, llvm::Type* targetType) {
    return coerceValueToLLVMType(value, targetType);
}

llvm::Value* createMethodCallOrDefault(llvm::Function* function, std::vector<llvm::Value*> argValues) {
    auto& cg = CodeGenerator::getInstance();
    if (!function || argValues.size() != function->arg_size()) {
        return function ? defaultValueForType(function->getReturnType()) : nullptr;
    }

    std::vector<llvm::Value*> adjustedArgs;
    adjustedArgs.reserve(argValues.size());
    for (size_t i = 0; i < argValues.size(); ++i) {
        llvm::Value* argument = argValues[i];
        if (argument && argument->getType()->isPointerTy() && cg.ownsParameter(function->getName().str(), static_cast<unsigned>(i))) {
            argument = cg.copyStackBufferToHeap(argument);
        }
        adjustedArgs.push_back(coerceArgumentValue(
            argument,
            function->getFunctionType()->getParamType(static_cast<unsigned>(i))));
        if (adjustedArgs.back()->getType() != function->getFunctionType()->getParamType(static_cast<unsigned>(i))) {
            return defaultValueForType(function->getReturnType());
        }
    }

    return cg.builder.CreateCall(function, adjustedArgs, function->getReturnType()->isVoidTy() ? "" : "calltmp");
}

llvm::Function* getOrCreateRuntimeFunction(CodeGenerator& cg, const std::string& name, llvm::Type* returnType, const std::vector<llvm::Type*>& paramTypes) {
    if (auto* function = cg.module->getFunction(name)) return function;
    auto* functionTy = llvm::FunctionType::get(returnType, paramTypes, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, name, cg.module.get());
}

llvm::Value* coerceStringIntArg(CodeGenerator& cg, llvm::Value* value) {
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    if (!value || value->getType() == i32Ty) {
        return value;
    }
    if (value->getType()->isIntegerTy()) {
        unsigned bits = value->getType()->getIntegerBitWidth();
        return bits < 32
            ? cg.builder.CreateSExt(value, i32Ty, "str.arg.sext")
            : cg.builder.CreateTrunc(value, i32Ty, "str.arg.trunc");
    }
    return value;
}

llvm::Value* codegenStringMethod(CodeGenerator& cg, llvm::Value* objectValue, const std::string& methodName, const std::vector<std::unique_ptr<ASTNode>>& arguments) {
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());

    auto boolCall = [&](const std::string& runtimeName, llvm::Value* rhs) -> llvm::Value* {
        auto* raw = cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, runtimeName, i32Ty, {i8PtrTy, i8PtrTy}),
            {objectValue, rhs},
            "str.bool.i32");
        return cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Ty, 0), "str.bool");
    };

    if (methodName == "length" || methodName == "size" || methodName == "count") {
        if (!arguments.empty()) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_string_length", i64Ty, {i8PtrTy}),
            {objectValue},
            "str.length");
    }
    if (methodName == "isEmpty") {
        if (!arguments.empty()) return nullptr;
        auto* raw = cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_string_is_empty", i32Ty, {i8PtrTy}),
            {objectValue},
            "str.empty.i32");
        return cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Ty, 0), "str.empty");
    }
    if (methodName == "toString") {
        if (!arguments.empty()) return nullptr;
        return objectValue;
    }
    if (methodName == "toUpper" || methodName == "upper") {
        if (!arguments.empty()) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_string_to_upper", i8PtrTy, {i8PtrTy}),
            {objectValue},
            "str.upper");
    }
    if (methodName == "toLower" || methodName == "lower") {
        if (!arguments.empty()) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_string_to_lower", i8PtrTy, {i8PtrTy}),
            {objectValue},
            "str.lower");
    }
    if (methodName == "trim") {
        if (!arguments.empty()) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_string_trim", i8PtrTy, {i8PtrTy}),
            {objectValue},
            "str.trim");
    }
    if (methodName == "contains" || methodName == "startsWith" || methodName == "endsWith" || methodName == "indexOf") {
        if (arguments.size() != 1) return nullptr;
        llvm::Value* needle = arguments[0]->codegen();
        if (!needle || !needle->getType()->isPointerTy()) return nullptr;
        if (methodName == "contains") return boolCall("csec_string_contains", needle);
        if (methodName == "startsWith") return boolCall("csec_string_starts_with", needle);
        if (methodName == "endsWith") return boolCall("csec_string_ends_with", needle);
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_string_index_of", i64Ty, {i8PtrTy, i8PtrTy}),
            {objectValue, needle},
            "str.index");
    }
    if (methodName == "charAt") {
        if (arguments.size() != 1) return nullptr;
        llvm::Value* index = coerceStringIntArg(cg, arguments[0]->codegen());
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_string_char_at", i8Ty, {i8PtrTy, i32Ty}),
            {objectValue, index},
            "str.char");
    }
    if (methodName == "substring") {
        if (arguments.size() != 2) return nullptr;
        llvm::Value* start = coerceStringIntArg(cg, arguments[0]->codegen());
        llvm::Value* length = coerceStringIntArg(cg, arguments[1]->codegen());
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_string_substring", i8PtrTy, {i8PtrTy, i32Ty, i32Ty}),
            {objectValue, start, length},
            "str.substring");
    }

    return nullptr;
}

bool canPassValueToParameter(llvm::Value* value, llvm::Type* parameterType) {
    if (!value || !parameterType) {
        return false;
    }
    llvm::Type* valueType = value->getType();
    if (valueType == parameterType) {
        return true;
    }
    if (valueType->isIntegerTy() && parameterType->isIntegerTy()) {
        return true;
    }
    if (valueType->isFloatingPointTy() && parameterType->isFloatingPointTy()) {
        return true;
    }
    if (valueType->isIntegerTy() && parameterType->isFloatingPointTy()) {
        return true;
    }
    if (valueType->isFloatingPointTy() && parameterType->isIntegerTy()) {
        return true;
    }
    if (valueType->isPointerTy() && parameterType->isPointerTy()) {
        return true;
    }
    if (valueType->isPointerTy() && !parameterType->isPointerTy()) {
        return true;
    }
    return false;
}

llvm::Function* findNamespacedFunctionByArgs(CodeGenerator& cg, const std::string& namespaceName, const std::string& methodName, const std::vector<llvm::Value*>& argValues) {
    const std::string baseName = namespaceName + "#" + methodName;
    llvm::Function* coercibleMatch = nullptr;
    for (auto& function : cg.module->functions()) {
        std::string functionName = function.getName().str();
        if (functionName != baseName && functionName.rfind(baseName + ".", 0) != 0) {
            continue;
        }
        if (function.arg_size() != argValues.size()) {
            continue;
        }

        bool exact = true;
        bool matches = true;
        for (size_t i = 0; i < argValues.size(); ++i) {
            auto* parameterType = function.getFunctionType()->getParamType(static_cast<unsigned>(i));
            if (argValues[i]->getType() != parameterType) {
                exact = false;
            }
            if (!canPassValueToParameter(argValues[i], parameterType)) {
                matches = false;
                break;
            }
        }
        if (matches && exact) {
            return &function;
        }
        if (matches && !coercibleMatch) {
            coercibleMatch = &function;
        }
    }
    return coercibleMatch;
}

std::string methodStorageKey(const std::string& methodName, const std::vector<std::unique_ptr<Type>>& argTypes) {
    if (argTypes.empty()) {
        return methodName;
    }

    std::ostringstream key;
    key << methodName;
    for (const auto& argType : argTypes) {
        key << "@" << (argType ? argType->getName() : "Unknown");
    }
    return key.str();
}
}

void MethodCallNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* MethodCallNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    if (object && isInterfaceType(object->getType().get())) {
        return codegenInterfaceCall(*this);
    }

    if (object) {
        auto receiverType = object->getType();
        if (isSetType(receiverType.get())) {
            bool handled = false;
            llvm::Value* result = codegenSetMethod(object.get(), methodName, arguments, handled);
            if (handled) return result;
        }
        if (isDictType(receiverType.get())) {
            bool handled = false;
            llvm::Value* result = codegenDictMethod(object.get(), methodName, arguments, handled);
            if (handled) return result;
        }
    }

    // `this.method(args)` and `super.method(args)` call a method of the enclosing class (or its
    // parent) on the current receiver. Resolve the defining class by walking the parent chain and
    // call ClassName_method with `this` as the receiver.
    if (auto* objectId = dynamic_cast<IdentifierNode*>(object.get())) {
        if (objectId->value == "this" || objectId->value == "super") {
            // The enclosing class is the type of the bound `this` receiver.
            auto* thisSymbolForClass = cg.symbolTable.lookup("this");
            ClassSymbol* enclosingClass = (thisSymbolForClass && thisSymbolForClass->type)
                ? cg.symbolTable.lookupClass(thisSymbolForClass->type->getName()) : nullptr;
            ClassSymbol* targetClass = enclosingClass;
            if (objectId->value == "super" && enclosingClass && !enclosingClass->superClassName.empty()) {
                targetClass = cg.symbolTable.lookupClass(enclosingClass->superClassName);
            }
            ClassSymbol* definingClass = targetClass;
            while (definingClass && definingClass->methods.find(methodName) == definingClass->methods.end()) {
                definingClass = definingClass->superClassName.empty()
                    ? nullptr : cg.symbolTable.lookupClass(definingClass->superClassName);
            }
            if (definingClass) {
                llvm::Function* function = cg.module->getFunction(definingClass->name + "_" + methodName);
                if (function && function->arg_size() >= 1) {
                    auto* thisSymbol = cg.symbolTable.lookup("this");
                    llvm::Value* receiver = thisSymbol ? thisSymbol->value : nullptr;
                    if (receiver) {
                        std::vector<llvm::Value*> argValues;
                        auto* receiverParamType = function->getFunctionType()->getParamType(0);
                        if (receiver->getType() != receiverParamType) {
                            receiver = cg.builder.CreateBitCast(receiver, receiverParamType, "recv.cast");
                        }
                        argValues.push_back(receiver);
                        for (auto& argument : arguments) {
                            llvm::Value* value = argument->codegen();
                            if (!value) return nullptr;
                            argValues.push_back(value);
                        }
                        return createMethodCallOrDefault(function, std::move(argValues));
                    }
                }
            }
        }
    }

    if (auto* objectId = dynamic_cast<IdentifierNode*>(object.get())) {
        std::string qualifiedName = objectId->value + "." + methodName;
        std::vector<std::unique_ptr<Type>> argTypes;
        argTypes.reserve(arguments.size());
        for (auto& argument : arguments) {
            auto argType = argument->getType();
            argTypes.push_back(argType ? argType->clone() : std::make_unique<UnknownType>());
        }

        auto* functionSymbol = cg.symbolTable.lookupFunctionInNamespace(objectId->value, methodName, argTypes);
        Symbol* memberSymbol = functionSymbol ? static_cast<Symbol*>(functionSymbol) : cg.symbolTable.lookup(qualifiedName);
        if (memberSymbol && memberSymbol->symbolType == SymbolType::FUNCTION) {
            auto* funcType = dynamic_cast<FunctionType*>(memberSymbol->type.get());
            llvm::Function* function = static_cast<llvm::Function*>(memberSymbol->value);
            if (!function) {
                function = cg.module->getFunction(objectId->value + "#" + methodName);
            }
            if (!function) {
                function = cg.module->getFunction("_" + methodName);
            }
            if (!funcType || !function) {
                std::cerr << "Error: Invalid namespaced function '" << qualifiedName << "'" << std::endl;
                return nullptr;
            }

            std::vector<llvm::Value*> argValues;
            for (size_t i = 0; i < arguments.size(); ++i) {
                llvm::Value* argValue = arguments[i]->codegen();
                if (!argValue) {
                    return nullptr;
                }
                if (functionSymbol && i < funcType->parameterTypes.size()) {
                    if (argTypes[i] && !argTypes[i]->equals(funcType->parameterTypes[i]) &&
                        !argTypes[i]->isSubtypeOf(funcType->parameterTypes[i])) {
                        std::cerr << "Type error: Argument type mismatch in method call '" << methodName << "'" << std::endl;
                        return nullptr;
                    }
                }
                argValues.push_back(argValue);
            }

            if (!functionSymbol) {
                if (auto* overloadedFunction = findNamespacedFunctionByArgs(cg, objectId->value, methodName, argValues)) {
                    function = overloadedFunction;
                }
            }

            return createMethodCallOrDefault(function, std::move(argValues));
        }
    }

    llvm::Value* objectValue = this->object->codegen();
    if (!objectValue) {
        return nullptr;
    }

    auto typeValue = object->getType();
    // Auto-deref through a borrow: a `&T`/`&mut T` receiver dispatches like the borrowed T. The
    // borrow value is already the underlying object pointer, so only the static type needs unwrapping.
    if (typeValue && (typeValue->getKind() == Type::Kind::BORROW ||
                      typeValue->getKind() == Type::Kind::MUTABLE_BORROW)) {
        if (auto* bt = dynamic_cast<BorrowType*>(typeValue.get())) {
            if (bt->baseType) typeValue = bt->baseType->clone();
        }
    }
    auto* objectType = typeValue.get();

    // Shared<T>/Weak<T> methods over the control block { i64 strong, i64 weak, payload }.
    //   Shared.clone() retains (bumps strong, returns the same block);
    //   Shared.get()   reads the payload;
    //   Weak.get()     reads the payload if the value is still alive (strong > 0), else 0.
    if (objectType && objectType->getKind() == Type::Kind::GENERIC &&
        (objectType->getName() == "Shared" || objectType->getName() == "Weak")) {
        const bool isWeak = objectType->getName() == "Weak";
        auto* genType = dynamic_cast<GenericType*>(objectType);
        llvm::Type* payloadType = (genType && !genType->typeArguments.empty() && genType->typeArguments[0])
            ? cg.getLLVMType(genType->typeArguments[0].get()) : nullptr;
        if (payloadType) {
            auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
            llvm::StructType* blockType = llvm::StructType::get(cg.context, { i64Ty, i64Ty, payloadType });
            llvm::Value* block = cg.builder.CreateBitCast(
                objectValue, llvm::PointerType::getUnqual(blockType->getContext()), "shared.block");
            if (!isWeak && methodName == "clone") {
                llvm::Value* strongPtr = cg.builder.CreateStructGEP(blockType, block, 0, "shared.strong.ptr");
                cg.builder.CreateAtomicRMW(llvm::AtomicRMWInst::Add, strongPtr, llvm::ConstantInt::get(i64Ty, 1),
                                           llvm::MaybeAlign(8), llvm::AtomicOrdering::Monotonic);
                return objectValue;
            }
            if (methodName == "get") {
                llvm::Value* payloadPtr = cg.builder.CreateStructGEP(blockType, block, 2, "shared.payload.ptr");
                if (!isWeak) {
                    return cg.builder.CreateLoad(payloadType, payloadPtr, "shared.get");
                }
                // Weak.get(): return the payload only while the value is alive, else a zero value.
                auto* strong = cg.builder.CreateLoad(
                    i64Ty, cg.builder.CreateStructGEP(blockType, block, 0, "weak.strong.ptr"), "weak.strong");
                strong->setAtomic(llvm::AtomicOrdering::Acquire);
                strong->setAlignment(llvm::Align(8));
                llvm::Value* payload = cg.builder.CreateLoad(payloadType, payloadPtr, "shared.get");
                llvm::Value* alive = cg.builder.CreateICmpSGT(strong, llvm::ConstantInt::get(i64Ty, 0), "weak.alive");
                llvm::Value* zero = llvm::Constant::getNullValue(payloadType);
                return cg.builder.CreateSelect(alive, payload, zero, "weak.get");
            }
        }
    }

    if (objectType && objectType->isStringTy()) {
        if (auto* result = codegenStringMethod(cg, objectValue, methodName, arguments)) {
            return result;
        }
        std::cerr << "Error: String method '" << methodName << "' not found" << std::endl;
        return nullptr;
    }
    if (!objectType || objectType->getKind() != Type::Kind::CLASS) {
        std::cerr << "Error: Method call on non-class type" << std::endl;
        return nullptr;
    }

    auto symbol = cg.symbolTable.lookupClass(objectType->getName());
    if (!symbol) {
        std::cerr << "Error: Undefined class type '" << objectType->getName() << "'" << std::endl;
        return nullptr;
    }

    ClassSymbol* classSymbol = symbol;

    std::vector<std::unique_ptr<Type>> callArgTypes;
    callArgTypes.reserve(arguments.size());
    for (auto& argument : arguments) {
        auto argType = argument->getType();
        callArgTypes.push_back(argType ? argType->clone() : std::make_unique<UnknownType>());
    }

    auto methodSymbolOpt = cg.symbolTable.lookupMethod(*classSymbol, methodName, callArgTypes);
    if (!methodSymbolOpt) {
        std::cerr << "Error: Method '" << methodName << "' not found in class '" << objectType->getName() << "'" << std::endl;
        return nullptr;
    }

    auto* methodSymbol = methodSymbolOpt;

    auto* funcType = dynamic_cast<FunctionType*>(methodSymbol->type.get());
    llvm::Function* function = static_cast<llvm::Function*>(methodSymbol->value);
    if (!function) {
        auto symbol = cg.symbolTable.lookupClass(objectType->getName());
        if (!symbol) {
            std::cerr << "Error: Undefined class type '" << objectType->getName() << "'" << std::endl;
            return nullptr;
        }

        ClassSymbol* classSymbol = symbol;
        std::string storageKey = methodStorageKey(methodName, callArgTypes);
        if (classSymbol->methodBodies.find(storageKey) == classSymbol->methodBodies.end()) {
            std::cerr << "Error: Method '" << methodName << "' not found in class '" << objectType->getName() << "'" << std::endl;
            return nullptr;
        }

        llvm::BasicBlock* savedBB = cg.builder.GetInsertBlock();
        classSymbol->methodBodies[storageKey]->codegen();
        cg.builder.SetInsertPoint(savedBB);
        function = cg.module->getFunction(objectType->getName() + "_" + methodName);
        methodSymbol->value = function;
    }

    if (!funcType || !function) {
        std::cerr << "Error: Invalid method '" << methodName << "' in class '" << objectType->getName() << "'" << std::endl;
        return nullptr;
    }

    // A struct method receives `this` by address. When the receiver is a temporary struct value
    // (e.g. the result of a struct-returning call, `f(...).method()`), it lives in a register and has
    // no address yet, so the method would read garbage fields. Materialize it into a stack slot and
    // pass the pointer. A receiver that is already a pointer (a struct variable, or a reference
    // class) is used unchanged.
    if (objectValue && !objectValue->getType()->isPointerTy()) {
        llvm::Value* slot = cg.builder.CreateAlloca(objectValue->getType(), nullptr, "recv.materialized");
        cg.builder.CreateStore(objectValue, slot);
        objectValue = slot;
    }

    std::vector<llvm::Value*> argValues;
    argValues.push_back(objectValue);

    if (arguments.size() + 1 != funcType->parameterTypes.size()) {
        std::cerr << "Error: Argument count mismatch in method call '" << methodName << "'" << std::endl;
        return nullptr;
    }

    for (size_t i = 0; i < arguments.size(); ++i) {
        llvm::Value* argValue = arguments[i]->codegen();
        if (!argValue) {
            return nullptr;
        }

        auto argType = arguments[i]->getType();
        if (!argType || (!argType->equals(funcType->parameterTypes[i + 1]) && !argType->isSubtypeOf(funcType->parameterTypes[i + 1]))) {
            // Overload resolution allows numeric widening (Int -> Long/Double, ...); convert the value.
            if (!argType || !isNumericWidening(*argType, *funcType->parameterTypes[i + 1])) {
                std::cerr << "Type error: Argument type mismatch in method call '" << methodName << "'" << std::endl;
                return nullptr;
            }
        }
        // Numeric arguments chosen through widening (Type::equals is loose on numerics): convert by LLVM type.
        if (i + 1 < function->arg_size()) {
            llvm::Type* paramType = function->getFunctionType()->getParamType(static_cast<unsigned>(i + 1));
            if (argValue->getType() != paramType && (paramType->isIntegerTy() || paramType->isFloatingPointTy()) &&
                (argValue->getType()->isIntegerTy() || argValue->getType()->isFloatingPointTy())) {
                argValue = csecCoerceNumericValue(argValue, paramType);
            }
        }

        argValues.push_back(argValue);
    }

    llvm::Value* result = createMethodCallOrDefault(function, std::move(argValues));
    return result;
}

std::unique_ptr<Type> MethodCallNode::getType() {
    if (type) return type->clone();
    if (object) {
        auto receiverType = object->getType();
        if (const InterfaceInfo* info = isInterfaceType(receiverType.get()) ? findInterface(receiverType->getName()) : nullptr) {
            const int index = info->methodIndex(methodName, arguments.size());
            if (index < 0) return std::make_unique<UnknownType>();
            return info->methods[static_cast<size_t>(index)].returnType->clone();
        }
    }

    if (object) {
        auto receiverType = object->getType();
        if (isSetType(receiverType.get())) {
            auto result = setMethodType(receiverType.get(), methodName, arguments.size());
            return result ? std::move(result) : std::make_unique<UnknownType>();
        }
        if (isDictType(receiverType.get())) {
            auto result = dictMethodType(receiverType.get(), methodName, arguments.size());
            return result ? std::move(result) : std::make_unique<UnknownType>();
        }
    }

    if (auto* objectId = dynamic_cast<IdentifierNode*>(object.get())) {
        std::vector<std::unique_ptr<Type>> argTypes;
        argTypes.reserve(arguments.size());
        for (auto& argument : arguments) {
            auto argType = argument->getType();
            argTypes.push_back(argType ? argType->clone() : std::make_unique<UnknownType>());
        }

        // `this.method()` dispatches on the enclosing class; `super.method()` on its parent.
        // Methods are registered on the class with their declared parameter types (no implicit
        // receiver), so search the class's own method map and walk up the parent chain rather
        // than lookupMethod, whose matching assumes a receiver slot.
        if (objectId->value == "this" || objectId->value == "super") {
            auto& table = CodeGenerator::getInstance().symbolTable;
            ClassSymbol* enclosingClass = table.getEnclosingClassSymbol();
            ClassSymbol* targetClass = enclosingClass;
            if (objectId->value == "super" && enclosingClass && !enclosingClass->superClassName.empty()) {
                targetClass = table.lookupClass(enclosingClass->superClassName);
            }
            for (ClassSymbol* cls = targetClass; cls; cls = table.lookupClass(cls->superClassName)) {
                auto found = cls->methods.find(methodName);
                if (found != cls->methods.end() && found->second) {
                    if (auto* methodFuncType = dynamic_cast<FunctionType*>(found->second->type.get())) {
                        if (methodFuncType->returnType) {
                            return methodFuncType->returnType->clone();
                        }
                    }
                }
                if (cls->superClassName.empty()) break;
            }
        }

        auto* functionSymbol = CodeGenerator::getInstance().symbolTable.lookupFunctionInNamespace(objectId->value, methodName, argTypes);
        Symbol* memberSymbol = functionSymbol ? static_cast<Symbol*>(functionSymbol) :
            CodeGenerator::getInstance().symbolTable.lookup(objectId->value + "." + methodName);
        if (memberSymbol && memberSymbol->symbolType == SymbolType::FUNCTION) {
            auto* funcType = dynamic_cast<FunctionType*>(memberSymbol->type.get());
            if (funcType && funcType->returnType) {
                return funcType->returnType->clone();
            }
        }
    }

    auto trueObject = object->getType();
    // Auto-deref through a borrow: `ref.method()` where ref : &T (or &mut T) resolves against T.
    // Strict ownership (M2) makes `&T` parameters common, and a method call on a borrowed receiver
    // must behave like a call on the borrowed value itself.
    if (trueObject && (trueObject->getKind() == Type::Kind::BORROW ||
                       trueObject->getKind() == Type::Kind::MUTABLE_BORROW)) {
        if (auto* bt = dynamic_cast<BorrowType*>(trueObject.get())) {
            if (bt->baseType) trueObject = bt->baseType->clone();
        }
    }
    auto* objectType = trueObject.get();

    // Shared<T>.clone() : Shared<T> ; Shared<T>.get() / Weak<T>.get() : T
    if (objectType && objectType->getKind() == Type::Kind::GENERIC &&
        (objectType->getName() == "Shared" || objectType->getName() == "Weak")) {
        if (methodName == "clone" && objectType->getName() == "Shared") {
            return objectType->clone();
        }
        if (methodName == "get") {
            auto* genType = dynamic_cast<GenericType*>(objectType);
            if (genType && !genType->typeArguments.empty() && genType->typeArguments[0]) {
                return genType->typeArguments[0]->clone();
            }
        }
    }

    if (objectType && objectType->isStringTy()) {
        if (methodName == "length" || methodName == "size" || methodName == "count" || methodName == "indexOf") {
            return std::make_unique<BasicType>("Long");
        }
        if (methodName == "isEmpty" || methodName == "contains" || methodName == "startsWith" || methodName == "endsWith") {
            return std::make_unique<BasicType>("Boolean");
        }
        if (methodName == "charAt") {
            return std::make_unique<BasicType>("Char");
        }
        if (methodName == "toString" || methodName == "substring" || methodName == "toUpper" ||
            methodName == "upper" || methodName == "toLower" || methodName == "lower" || methodName == "trim") {
            return std::make_unique<BasicType>("String");
        }
        return std::make_unique<UnknownType>();
    }
    if (!objectType || objectType->getKind() != Type::Kind::CLASS) {
        return std::make_unique<BasicType>("Real");
    }

    auto classSymbolOpt = CodeGenerator::getInstance().symbolTable.lookupClass(objectType->getName());
    if (!classSymbolOpt) {
        return std::make_unique<BasicType>("Real");
    }

    auto* classSymbol = classSymbolOpt;

    std::vector<std::unique_ptr<Type>> callArgTypes;
    callArgTypes.reserve(arguments.size());
    for (auto& argument : arguments) {
        auto argType = argument->getType();
        callArgTypes.push_back(argType ? argType->clone() : std::make_unique<UnknownType>());
    }

    // Resolve the method by walking the parent chain. During type checking a child class's method
    // map does not yet contain copies of inherited methods (that copy happens at codegen), so a
    // lookup confined to the receiver's own class would miss an inherited method such as
    // `dog.getAge()` and fall back to `Real`, breaking callers that expect the declared return type.
    auto& table = CodeGenerator::getInstance().symbolTable;
    for (ClassSymbol* cls = classSymbol; cls; ) {
        if (auto* methodSymbol = table.lookupMethod(*cls, methodName, callArgTypes)) {
            if (auto* funcType = dynamic_cast<FunctionType*>(methodSymbol->type.get())) {
                if (funcType->returnType) {
                    return funcType->returnType->clone();
                }
            }
        }
        auto found = cls->methods.find(methodName);
        if (found != cls->methods.end() && found->second) {
            if (auto* funcType = dynamic_cast<FunctionType*>(found->second->type.get())) {
                if (funcType->returnType) {
                    return funcType->returnType->clone();
                }
            }
        }
        if (cls->superClassName.empty()) {
            break;
        }
        cls = table.lookupClass(cls->superClassName);
    }

    return std::make_unique<BasicType>("Real");
}
