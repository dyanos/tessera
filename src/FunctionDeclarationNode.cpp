#include "codegen.h"
#include "ArrayLiteralNode.h"
#include "type_checker.h"
#include "default_arguments.h"
#include "debug_info.h"

#include "FunctionDeclarationNode.h"
#include "ASTVisitor.h"
#include "utils.h"
#include "type_utils.h"

#include <iostream>
#include <llvm/IR/Function.h>

#include "ParameterNode.h"
#include "BlockNode.h"

void FunctionDeclarationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

namespace {
FunctionSymbol* updateOrRegisterFunctionSymbolNamed(
    const std::string& name, std::unique_ptr<Type> functionType, llvm::Function* function, bool isExternalSymbol) {
    auto& symbolTable = CodeGenerator::getInstance().symbolTable;
    FunctionSymbol* existingFunction = nullptr;
    if (auto* concreteFunctionType = dynamic_cast<FunctionType*>(functionType.get())) {
        // Exact signature, so an overload never overwrites another overload's symbol.
        existingFunction = symbolTable.lookupFunctionExact(name, concreteFunctionType->parameterTypes);
    }
    auto* existingSymbol = existingFunction ? static_cast<Symbol*>(existingFunction) : nullptr;
    if (existingSymbol && existingSymbol->symbolType == SymbolType::FUNCTION) {
        existingSymbol->type = functionType ? functionType->clone() : std::make_unique<UnknownType>();
        existingSymbol->value = function;
        existingSymbol->function = function;
        existingSymbol->isMutable = isExternalSymbol;
        return dynamic_cast<FunctionSymbol*>(existingSymbol);
    }

    auto functionSymbol = std::make_unique<FunctionSymbol>(name, std::move(functionType), function, isExternalSymbol, SymbolType::FUNCTION);
    auto* functionSymbolRaw = functionSymbol.get();
    if (!symbolTable.addSymbol(name, std::move(functionSymbol))) {
        std::cerr << "Error: Failed to register function symbol '" << name << "'" << std::endl;
        return nullptr;
    }
    return functionSymbolRaw;
}

std::string mangledFunctionName(CodeGenerator& cg, const std::string& name) {
    return cg.scopes.empty() ? "_" + name : join(cg.scopes, "#") + "#" + name;
}

// Give an overload a distinct LLVM name when a different-signature function already owns the
// mangled name. Object/namespace methods mangle purely by scope and name (`O#p`), so overloads
// such as `p(String)` and `p(Boolean)` would otherwise collide: the second body would attach to
// the first function and read its parameter with the wrong type (e.g. a pointer where the body
// expects a Boolean, producing an illegal zext). findNamespacedFunctionByArgs already matches both
// the base name and `base.*`, so a signature-qualified suffix resolves correctly at the call site.
std::string overloadUniqueFunctionName(CodeGenerator& cg, const std::string& name, const std::string& baseName,
                                       llvm::FunctionType* funcType,
                                       const std::vector<std::unique_ptr<ASTNode>>& parameters) {
    // An overload that already has its function (e.g. from the prototype pass) keeps it.
    std::vector<std::unique_ptr<Type>> parameterTypes;
    for (auto& param : parameters) {
        auto type = param ? param->getType() : nullptr;
        parameterTypes.push_back(type ? type->clone() : std::make_unique<UnknownType>());
    }
    if (auto* self = cg.symbolTable.lookupFunctionExact(name, parameterTypes)) {
        if (auto* own = llvm::dyn_cast_or_null<llvm::Function>(self->value)) {
            return own->getName().str();
        }
    }
    llvm::Function* existing = cg.module->getFunction(baseName);
    // The base name belongs to another overload when a different-signature function symbol owns it,
    // even if the LLVM types coincide (String and a class are both `ptr`).
    bool ownedByOtherOverload = false;
    if (existing) {
        for (auto* overload : cg.symbolTable.functionOverloads(name)) {
            if (overload->value == existing) {
                ownedByOtherOverload = true;
            }
        }
    }
    if (!existing || (existing->getFunctionType() == funcType && !ownedByOtherOverload)) {
        return baseName;
    }
    std::string suffix;
    for (auto& param : parameters) {
        auto type = param ? param->getType() : nullptr;
        suffix += "." + (type ? type->getName() : std::string("Unknown"));
    }
    std::string candidate = baseName + suffix;
    for (int counter = 0;; ++counter) {
        llvm::Function* clash = cg.module->getFunction(candidate);
        if (!clash || clash->getFunctionType() == funcType) {
            return candidate;
        }
        candidate = baseName + suffix + "." + std::to_string(counter);
    }
}
}

void markOwnership(CodeGenerator& cg, const std::string& functionName, const Type* returnType,
                   const std::vector<std::unique_ptr<ASTNode>>& parameters, bool hasReceiver,
                   const std::string& className, const std::string& methodName) {
    if (!isStrictOwnership()) return;
    if (returnType && !typeIsCopy(returnType)) {
        if (llvm::Function* drop = cg.ownedValueDropFunction(returnType)) cg.markOwnedResultFunction(functionName, drop);
    }
    if (returnType && returnType->getName() != "String" && cg.isRefcountedType(returnType)) {
        cg.markStringFunction(functionName, false);
    }
    if (llvm::Function* release = cg.structCountFunction(returnType, false)) {
        cg.structResultFunctions[functionName] = release;
    }
    std::vector<bool> mask;
    if (hasReceiver) {
        ClassType receiverType(className);
        std::string declared = className;
        std::string templateName;
        if (templateInstanceArguments(className, &templateName, nullptr)) declared = templateName;
        mask.push_back(!cg.isRefcountedType(&receiverType) && !typeIsCopy(&receiverType) &&
                       methodMayKeepThis(declared, methodName));
    }
    for (const auto& parameter : parameters) {
        auto type = parameter ? parameter->getType() : nullptr;
        const bool owned = type && !typeIsCopy(type.get()) && type->getName() != "String" &&
                           type->getKind() != Type::Kind::BORROW && type->getKind() != Type::Kind::MUTABLE_BORROW;
        mask.push_back(owned);
    }
    cg.markOwnedParameters(functionName, std::move(mask));
}

llvm::Function* FunctionDeclarationNode::declarePrototype() {
    if (this->isExternal) return nullptr;
    auto& cg = CodeGenerator::getInstance();

    std::vector<llvm::Type*> paramTypes;
    for (auto& param : this->parameters) {
        auto* paramType = getABIStorageType(param->getType().get());
        if (!paramType) return nullptr;
        paramTypes.push_back(paramType);
    }
    llvm::Type* returnType = getABIStorageType(this->returnType.get());
    if (!returnType) return nullptr;

    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
    const std::string funcName = overloadUniqueFunctionName(cg, this->name, mangledFunctionName(cg, this->name), funcType, this->parameters);
    llvm::Function* function = cg.module->getFunction(funcName);
    if (!function) {
        function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, funcName, cg.module.get());
        if (this->isConstexpr) {
            function->addFnAttr(llvm::Attribute::AlwaysInline);
        }
    }
    if (this->returnType && this->returnType->getName() == "String") cg.markStringFunction(funcName, false);
    markOwnership(cg, funcName, this->returnType.get(), this->parameters, false, "", "");
    updateOrRegisterFunctionSymbolNamed(this->name, this->getType(), function, false);
    return function;
}

llvm::Value* FunctionDeclarationNode::codegen() {
    if (isIntrinsic) {
        return nullptr;
    }
    auto updateOrRegisterFunctionSymbol = [&](std::unique_ptr<Type> functionType, llvm::Function* function, bool isExternalSymbol) -> FunctionSymbol* {
        return updateOrRegisterFunctionSymbolNamed(this->name, std::move(functionType), function, isExternalSymbol);
    };

    if (this->isExternal) {
        std::vector<llvm::Type*> paramTypes;
        for (auto& param : this->parameters) {
            auto* paramType = getABIStorageType(param->getType().get());
            if (!paramType) {
                std::cerr << "Error: Not supported parameter type in external function '" << this->name << "'" << std::endl;
                return nullptr;
            }
            paramTypes.push_back(paramType);
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(
            getABIStorageType(this->returnType.get()),
            paramTypes,
            false);
        const std::string llvmName = this->externalSymbolName.empty() ? this->name : this->externalSymbolName;
        llvm::Function * function = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            llvmName,
            CodeGenerator::getInstance().module.get()
		);

        auto functionType = std::make_unique<FunctionType>();
        functionType->returnType = this->returnType ? this->returnType->clone() : std::make_unique<UnknownType>();
        for (auto& param : this->parameters) {
            functionType->parameterTypes.push_back(param->getType()->clone());
        }
        if (!updateOrRegisterFunctionSymbol(std::move(functionType), function, true)) {
            return nullptr;
        }

		return function;
	}

    auto& cg = CodeGenerator::getInstance();

    std::vector<llvm::Type*> paramTypes;

    for (auto& param : this->parameters) {
        auto* paramType = getABIStorageType(param->getType().get());
        if (!paramType) {
            std::cerr << "Error: Not supported parameter type in function '" << this->name << "'" << std::endl;
            return nullptr;
        }
        paramTypes.push_back(paramType);
    }

    llvm::Type* returnType = getABIStorageType(this->returnType.get());
    if (!returnType) {
        std::cerr << "Error: Not supported the return type of '" << this->name << "'" << std::endl;
        return nullptr;
    }
    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
    std::string funcName = overloadUniqueFunctionName(cg, this->name, mangledFunctionName(cg, this->name), funcType, this->parameters);

    // Reuse the prototype if one was already declared (for example by the pre-declaration pass
    // that enables forward references), so the body attaches to the function the callers resolved
    // against rather than a renamed duplicate.
    llvm::Function* function = cg.module->getFunction(funcName);
    if (!function) {
        function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, funcName, cg.module.get());
    }
    if (this->returnType && this->returnType->getName() == "String") cg.markStringFunction(funcName, false);
    markOwnership(cg, funcName, this->returnType.get(), this->parameters, false, "", "");

    if (this->isConstexpr) {
        function->addFnAttr(llvm::Attribute::AlwaysInline);
    }

    auto* functionSymbolRaw = updateOrRegisterFunctionSymbol(this->getType(), function, false);
    if (!functionSymbolRaw) {
        return nullptr;
    }
    auto* savedCurrentSymbol = cg.symbolTable.getCurrentSymbol();
    cg.symbolTable.saveCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(functionSymbolRaw);
    cg.symbolTable.enterScope();

    llvm::BasicBlock* bb = llvm::BasicBlock::Create(cg.context, "entry", function);
    cg.builder.SetInsertPoint(bb);
    debuginfo::beginFunction(function, this->name, this->line);

    int cnt = 0;
    std::vector<std::pair<llvm::Value*, std::unique_ptr<Type>>> ownedParameters;
    for (auto& arg : this->parameters) {
        auto* paramNode = dynamic_cast<ParameterNode*>(arg.get());
        if (!paramNode) {
            std::cerr << "Error: Non-parameter node in parameters list of '" << this->name << "'" << std::endl;
            cnt += 1;
            continue;
        }
        auto paramType = arg->getType();
        llvm::Value* paramValue = function->getArg(cnt);
        if (isStructClassType(paramType.get())) {
            llvm::Type* storageType = cg.getLLVMType(paramType.get());
            auto* paramSlot = cg.createEntryAlloca(storageType, (paramNode->name + ".addr").c_str());
            cg.builder.CreateStore(paramValue, paramSlot);
            paramValue = paramSlot;
        }
        if (isStrictOwnership() && paramType && !isStructClassType(paramType.get()) && !typeIsCopy(paramType.get())) {
            ownedParameters.emplace_back(paramValue, paramType->clone());
        }
        auto paramSymbol = std::make_unique<Symbol>(paramNode->name, std::move(paramType), paramValue, false, SymbolType::VARIABLE);
        cg.symbolTable.addSymbol(paramNode->name, std::move(paramSymbol));
        cnt += 1;
    }

    cg.enterCleanupScope();
    for (auto& owned : ownedParameters) cg.registerOwnedValueCleanup(owned.first, owned.second.get());
    if (this->body) {
        ArrayLiteralNode::markHoistable(this->body.get());
        std::set<std::string> outerMoved = std::move(cg.movedNames);
        cg.movedNames = ArrayLiteralNode::movedNames(this->body.get());
        this->body->codegen();
        cg.movedNames = std::move(outerMoved);
    }
    if (cg.builder.GetInsertBlock() && !csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
        cg.emitCurrentScopeCleanups();
    }
    cg.exitCleanupScope();

    llvm::BasicBlock* currentBlock = cg.builder.GetInsertBlock();
    if (currentBlock && !csec_llvm::terminatorOrNull(currentBlock)) {
        if (returnType->isVoidTy()) {
            cg.builder.CreateRetVoid();
        }
        else if (returnType->isIntegerTy()) {
            cg.builder.CreateRet(llvm::ConstantInt::get(returnType, 0));
        }
        else if (returnType->isFloatTy()) {
            cg.builder.CreateRet(llvm::ConstantFP::get(returnType, 0.0));
        }
        else if (returnType->isDoubleTy()) {
            cg.builder.CreateRet(llvm::ConstantFP::get(returnType, 0.0));
        }
        else {
            cg.builder.CreateRet(llvm::Constant::getNullValue(returnType));
        }
    }

    cg.symbolTable.exitScope();
    cg.symbolTable.popCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(savedCurrentSymbol);

    return function;
}
