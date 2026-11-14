#include "codegen.h"

#include "IdentifierNode.h"
#include "ASTVisitor.h"
#include "type_utils.h"

#include <iostream>
#include <cctype>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>

namespace {
// A top-level function used as a first-class value must be presented as the uniform { code, env }
// closure the call sites consume (they load `code` and call `code(env, args...)`), not as a raw
// function pointer whose signature has no env parameter. Build (once) an env-ignoring thunk
// `thunk(ptr env, args...) -> fn(args...)` and return a { thunk, null } closure. Without this,
// passing a named function to a `(T...) => R` parameter reinterprets the function's code as a
// closure struct and crashes.
llvm::Value* wrapFunctionAsClosure(llvm::Function* fn) {
    auto& cg = CodeGenerator::getInstance();
    auto* opaquePtr = llvm::PointerType::getUnqual(cg.context);

    const std::string thunkName = fn->getName().str() + "$closure_thunk";
    llvm::Function* thunk = cg.module->getFunction(thunkName);
    if (!thunk) {
        std::vector<llvm::Type*> thunkParams;
        thunkParams.push_back(opaquePtr); // env slot (ignored)
        for (auto& param : fn->args()) {
            thunkParams.push_back(param.getType());
        }
        auto* thunkType = llvm::FunctionType::get(fn->getReturnType(), thunkParams, false);
        thunk = llvm::Function::Create(thunkType, llvm::Function::InternalLinkage, thunkName, cg.module.get());

        auto savedIP = cg.builder.saveIP();
        auto* entry = llvm::BasicBlock::Create(cg.context, "entry", thunk);
        cg.builder.SetInsertPoint(entry);
        std::vector<llvm::Value*> callArgs;
        auto argIt = thunk->arg_begin();
        ++argIt; // skip the env parameter
        for (; argIt != thunk->arg_end(); ++argIt) {
            callArgs.push_back(&*argIt);
        }
        llvm::Value* result = cg.builder.CreateCall(fn, callArgs);
        if (fn->getReturnType()->isVoidTy()) {
            cg.builder.CreateRetVoid();
        } else {
            cg.builder.CreateRet(result);
        }
        cg.builder.restoreIP(savedIP);
    }

    auto* closureType = llvm::StructType::get(cg.context, { opaquePtr, opaquePtr });
    llvm::Value* closure = cg.builder.CreateAlloca(closureType, nullptr, "fn.closure");
    cg.builder.CreateStore(thunk, cg.builder.CreateStructGEP(closureType, closure, 0, "closure.code"));
    cg.builder.CreateStore(llvm::ConstantPointerNull::get(opaquePtr),
        cg.builder.CreateStructGEP(closureType, closure, 1, "closure.env"));
    return closure;
}

bool isIntegerLiteralText(const std::string& text) {
    if (text.empty()) {
        return false;
    }

    size_t start = 0;
    if (text[0] == '+' || text[0] == '-') {
        start = 1;
    }
    if (start >= text.size()) {
        return false;
    }

    for (size_t i = start; i < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }
    return true;
}
}

void IdentifierNode::accept(ASTVisitor& visitor) {
	visitor.visit(*this);
}

llvm::Value* IdentifierNode::codegen() {
	auto symbolOpt = CodeGenerator::getInstance().symbolTable.lookup(value);
	if (!symbolOpt) {
        auto& cg = CodeGenerator::getInstance();
        if (value == "true" || value == "false") {
            return llvm::ConstantInt::get(llvm::Type::getInt1Ty(cg.context), value == "true" ? 1 : 0);
        }
        if (isIntegerLiteralText(value)) {
            // strtoll never throws (std::stoi throws on a literal beyond 2^31). A literal too large for
            // this scalar is only meaningful when the target is Nat, where coerceToNat() reparses the
            // exact text via from_decimal and ignores this (truncated) placeholder.
            long long parsed = std::strtoll(value.c_str(), nullptr, 10);
            // Text beyond the Int range (e.g. a substituted `Long` template argument) is a Long.
            if (parsed < INT32_MIN || parsed > INT32_MAX) {
                return llvm::ConstantInt::get(llvm::Type::getInt64Ty(cg.context), parsed, true);
            }
            return llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), static_cast<int32_t>(parsed));
        }
		std::cerr << "Undefined variable: " << value << std::endl;
		return llvm::ConstantFP::get(llvm::Type::getDoubleTy(cg.context), 0.0);
	}

	auto* symbol = symbolOpt;

	// A Nat variable is a slot holding its arbitrary-precision handle; reading it yields the handle.
	// (A Nat parameter is bound to the bare handle directly, not an alloca, so it is returned as-is.)
	if (symbol->type && symbol->type->getName() == "Nat" && symbol->value &&
		llvm::isa<llvm::AllocaInst>(symbol->value)) {
		auto& cg = CodeGenerator::getInstance();
		auto* handleTy = llvm::PointerType::getUnqual(cg.context);
		return cg.builder.CreateLoad(handleTy, symbol->value, value + ".nat");
	}

	// A bare reference to a top-level function (used as a value: passed as an argument, assigned to
	// a function-typed variable, returned) becomes a { code, env } closure so it matches the uniform
	// callable representation the call sites expect. Direct calls resolve the callee by name in
	// FunctionCallNode and never reach here, so this only fires for value uses.
	if (symbol->symbolType == SymbolType::FUNCTION && symbol->value &&
		llvm::isa<llvm::Function>(symbol->value)) {
		return wrapFunctionAsClosure(llvm::cast<llvm::Function>(symbol->value));
	}

	if (!symbol->value) {
        auto& cg = CodeGenerator::getInstance();
        if (symbol->type && symbol->type->isIntegerTy()) {
            return llvm::ConstantInt::get(cg.getLLVMType(symbol->type.get()), 0);
        }
        if (symbol->type && (symbol->type->isFloatTy() || symbol->type->isDoubleTy())) {
            return llvm::ConstantFP::get(cg.getLLVMType(symbol->type.get()), 0.0);
        }
        if (symbol->type && (symbol->type->getName() == "Boolean" || symbol->type->getName() == "Bool")) {
            return llvm::ConstantInt::getFalse(cg.context);
        }
		return llvm::ConstantFP::get(llvm::Type::getDoubleTy(cg.context), 0.0);
	}

    const bool isAddressLike =
        llvm::isa<llvm::AllocaInst>(symbol->value) ||
        llvm::isa<llvm::GlobalVariable>(symbol->value) ||
        llvm::isa<llvm::GetElementPtrInst>(symbol->value);

    if ((symbol->symbolType == SymbolType::VARIABLE || symbol->symbolType == SymbolType::FIELD) &&
        symbol->value->getType()->isPointerTy() &&
        isAddressLike) {
        // An array/vector local is bound directly to its element buffer (see
        // VariableDeclarationNode::bindPointerBackedValueDirectly), so return that pointer rather than
        // loading through it. `Array` and `Vector` are aliases: without recognising `Vector`, an
        // annotated `val v: Vector[Int] = [..]` loads a pointer out of the buffer's first element
        // (reading data as an address) and segfaults on the next index.
        if (symbol->type && (symbol->type->getName() == "Array" ||
            symbol->type->getName() == "Vector" ||
            dynamic_cast<ArrayType*>(symbol->type.get()) != nullptr)) {
            return symbol->value;
        }
        // A lambda value is a { code, env } closure pointer bound directly; return the pointer
        // itself rather than loading through it, so the whole closure is passed and called.
        if (symbol->type && symbol->type->getKind() == Type::Kind::FUNCTION) {
            return symbol->value;
        }
        if (symbol->type && symbol->type->getKind() == Type::Kind::CLASS) {
            if (isStructClassType(symbol->type.get())) {
                return symbol->value;
            }
            auto valueType = getABIStorageType(symbol->type.get());
            if (!valueType) {
                return nullptr;
            }
            return CodeGenerator::getInstance().builder.CreateLoad(valueType, symbol->value, value + ".load");
        }
        auto valueType = CodeGenerator::getInstance().getLLVMType(symbol->type.get());
        if (!valueType) {
            return nullptr;
        }
        return CodeGenerator::getInstance().builder.CreateLoad(valueType, symbol->value, value + ".load");
	}

	return symbol->value;
}

std::unique_ptr<Type> IdentifierNode::getType() {
	if (type) return type->clone();

	auto symbolOpt = CodeGenerator::getInstance().symbolTable.lookup(value);
	if (symbolOpt && symbolOpt->type) {
		return symbolOpt->type->clone();
	}

    if (value == "true" || value == "false") {
        return std::make_unique<BasicType>("Boolean");
    }
    if (isIntegerLiteralText(value)) {
        const long long parsed = std::strtoll(value.c_str(), nullptr, 10);
        return std::make_unique<BasicType>(parsed < INT32_MIN || parsed > INT32_MAX ? "Long" : "Int");
    }

	return std::make_unique<BasicType>("Real");
}
