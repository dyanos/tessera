#include "codegen.h"
#include "source_map.h"
#include "verifier.h"
#include "dict_support.h"

#include "FunctionCallNode.h"
#include "FunctionDeclarationNode.h"
#include "ParameterNode.h"
#include "ASTVisitor.h"
#include "BinaryExpressionNode.h"
#include "BlockNode.h"
#include "ClassInstanceCreationNode.h"
#include "IdentifierNode.h"
#include "IfStatementNode.h"
#include "LambdaExpressionNode.h"
#include "intrinsics.h"
#include "ReturnStatementNode.h"
#include "VariableDeclarationNode.h"
#include "TensorRuntime.h"
#include "type_utils.h"

#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/DataLayout.h>

namespace {
bool isMathFallbackFunction(const std::string& name) {
    static const char* names[] = {
        "sin", "cos", "tan", "cot", "sec", "csc", "arcsin", "arccos", "arctan",
        "sinh", "cosh", "tanh", "coth", "sqrt", "ln", "log", "lg", "exp",
        "frac", "binom", "min", "max", "det", "dim", "gcd", "pow",
        "abs", "sign", "floor", "ceil", "round", "lcm", "approxEq", "clamp", "between",
        "emptySet", "singleton", "cardinality", "disjoint", "complement",
        "transpose", "conjugate", "adjoint", "tensorProduct", "innerProduct", "outerProduct",
        "sum", "mean", "norm", "relu", "softmax", "mse", "prod", "int", "lim", "liminf", "limsup",
        "oint", "iint", "bigcup", "bigcap", "biguplus", "bigoplus",
        "bigvee", "bigotimes", "bigwedge", "coprod", "bigodot", "bigsqcup"
    };
    for (const char* candidate : names) {
        if (name == candidate) return true;
    }
    return false;
}

bool isNativeIOFunction(const std::string& name) {
    return name == "print" || name == "println" ||
           name == "readLine" || name == "readChar" ||
           name == "readInt" || name == "readDouble";
}

bool isNativeProcessFunction(const std::string& name) {
    return name == "commandLineArgCount" || name == "commandLineArg";
}

bool isNativeNetworkFunction(const std::string& name) {
    return name == "tcpConnect" || name == "tcpListen" || name == "tcpAccept" || name == "tcpSend" ||
           name == "tcpRecv" || name == "tcpClose";
}

bool isNativePosixFunction(const std::string& name) {
    static const char* names[] = {
        "posixOpen", "posixRead", "posixWrite", "posixClose", "posixLseek",
        "posixUnlink", "posixRename", "posixMkdir", "posixRmdir", "posixChdir",
        "posixGetcwd", "posixAccess", "posixGetenv", "posixSetenv", "posixUnsetenv",
        "posixSleep", "posixTime", "posixErrno",
        "posixFlagReadOnly", "posixFlagWriteOnly", "posixFlagReadWrite",
        "posixFlagCreate", "posixFlagTruncate", "posixFlagAppend",
        "posixSeekSet", "posixSeekCur", "posixSeekEnd",
        "posixAccessExists", "posixAccessRead", "posixAccessWrite", "posixAccessExecute"
    };
    for (const char* candidate : names) {
        if (name == candidate) return true;
    }
    return false;
}

bool isNativeSystemFileFunction(const std::string& name) {
    return name == "systemFileReadAllText" ||
           name == "systemFileWriteAllText" ||
           name == "systemFileAppendAllText" ||
           name == "systemFileExists" ||
           name == "systemFileDelete";
}

bool isNativeParallelFunction(const std::string& name) {
    return name == "parallelThreads" ||
           name == "setParallelThreads" ||
           name == "parallelBackendAvailable" ||
           name == "parallelBackendImplemented";
}

// Erlang-style microthread builtins (runtime: native_runtime/process_runtime.inc). A user-declared
// function or variable of the same name takes precedence, so these never shadow existing programs.
bool isNativeMicrothreadName(const std::string& name) {
    static const char* names[] = {
        "spawn", "self", "send", "receive", "receiveTimeout", "yield",
        "isAlive", "monitor", "mailboxSize", "processCount", "joinProcesses",
        "threadStart"
    };
    for (const char* candidate : names) {
        if (name == candidate) return true;
    }
    return false;
}

bool resolvesToMicrothreadBuiltin(const std::string& name) {
    // Only active once a library module declared it `[@Intrinsic]` (e.g. `import std.microthread`).
    return isNativeMicrothreadName(name) && isDeclaredIntrinsic(name);
}

bool isNativeDynamicLibraryFunction(const std::string& name) {
    static const char* names[] = {
        "loadLibrary", "getSymbol", "closeLibrary",
        "callNative0", "callNative1", "callNative2", "callNative3",
        "callNativeDouble0", "callNativeDouble1", "callNativeDouble2"
    };
    for (const char* candidate : names) {
        if (name == candidate) return true;
    }
    return false;
}

bool isNativeSimulationFunction(const std::string& name) {
    return name == "mdSimulate" || name == "cfdSimulate" || name == "proteinMcmc" || name == "blackHoleMerge";
}

bool isBuiltinOdeFunction(const std::string& name) {
    return name == "odeEuler" || name == "numericDerivative" || name == "integral";
}

bool isNativeScalarMathFunction(const std::string& name) {
    static const char* names[] = {
        "sin", "cos", "tan", "cot", "sec", "csc", "arcsin", "arccos", "arctan",
        "sinh", "cosh", "tanh", "coth", "sqrt", "ln", "log", "lg", "exp",
        "frac", "binom", "min", "max", "gcd", "pow",
        "abs", "sign", "floor", "ceil", "round", "lcm", "approxEq", "clamp", "between",
        "emptySet", "singleton", "cardinality", "disjoint", "complement"
    };
    for (const char* candidate : names) {
        if (name == candidate) return true;
    }
    return false;
}

llvm::Value* cString(CodeGenerator& cg, const std::string& value, const std::string& name = "cstr") {
    return cg.builder.CreateGlobalString(value, name);
}

llvm::Function* getOrCreateRuntimeFunction(
    CodeGenerator& cg,
    const std::string& name,
    llvm::Type* returnType,
    const std::vector<llvm::Type*>& paramTypes) {
    cg.requireSystemNative();
    if (auto* function = cg.module->getFunction(name)) {
        return function;
    }
    auto* functionTy = llvm::FunctionType::get(returnType, paramTypes, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, name, cg.module.get());
}

llvm::Function* getOrCreateUnaryDoubleFunction(CodeGenerator& cg, const std::string& cName) {
    if (auto* function = cg.module->getFunction(cName)) {
        return function;
    }
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    auto* functionTy = llvm::FunctionType::get(f64Ty, {f64Ty}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, cName, cg.module.get());
}

llvm::Function* getOrCreateBinaryDoubleFunction(CodeGenerator& cg, const std::string& cName) {
    if (auto* function = cg.module->getFunction(cName)) {
        return function;
    }
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    auto* functionTy = llvm::FunctionType::get(f64Ty, {f64Ty, f64Ty}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, cName, cg.module.get());
}

llvm::Function* getOrCreateGcdI64(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("__csec_gcd_i64")) {
        return function;
    }

    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* functionTy = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty}, false);
    auto* function = llvm::Function::Create(functionTy, llvm::Function::InternalLinkage, "__csec_gcd_i64", cg.module.get());
    auto* entryBB = llvm::BasicBlock::Create(cg.context, "entry", function);
    auto* loopBB = llvm::BasicBlock::Create(cg.context, "loop", function);
    auto* bodyBB = llvm::BasicBlock::Create(cg.context, "body", function);
    auto* exitBB = llvm::BasicBlock::Create(cg.context, "exit", function);

    llvm::IRBuilder<> builder(cg.context);
    auto args = function->arg_begin();
    llvm::Value* aArg = &*args++;
    llvm::Value* bArg = &*args++;

    builder.SetInsertPoint(entryBB);
    llvm::Value* zero = llvm::ConstantInt::get(i64Ty, 0);
    llvm::Value* aAbs = builder.CreateSelect(builder.CreateICmpSLT(aArg, zero), builder.CreateNeg(aArg), aArg, "a.abs");
    llvm::Value* bAbs = builder.CreateSelect(builder.CreateICmpSLT(bArg, zero), builder.CreateNeg(bArg), bArg, "b.abs");
    builder.CreateBr(loopBB);

    builder.SetInsertPoint(loopBB);
    auto* xPhi = builder.CreatePHI(i64Ty, 2, "x");
    auto* yPhi = builder.CreatePHI(i64Ty, 2, "y");
    builder.CreateCondBr(builder.CreateICmpEQ(yPhi, zero), exitBB, bodyBB);

    builder.SetInsertPoint(bodyBB);
    llvm::Value* rem = builder.CreateSRem(xPhi, yPhi, "rem");
    builder.CreateBr(loopBB);

    xPhi->addIncoming(aAbs, entryBB);
    xPhi->addIncoming(yPhi, bodyBB);
    yPhi->addIncoming(bAbs, entryBB);
    yPhi->addIncoming(rem, bodyBB);

    builder.SetInsertPoint(exitBB);
    builder.CreateRet(xPhi);
    return function;
}

llvm::Value* coerceMathValueToDouble(CodeGenerator& cg, llvm::Value* value) {
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    if (!value) return llvm::ConstantFP::get(f64Ty, 0.0);
    if (value->getType() == f64Ty) return value;
    if (value->getType()->isFloatTy()) return cg.builder.CreateFPExt(value, f64Ty, "math.fpext");
    if (value->getType()->isIntegerTy()) return cg.builder.CreateSIToFP(value, f64Ty, "math.sitofp");
    return llvm::ConstantFP::get(f64Ty, 0.0);
}

llvm::Value* coerceMathValueToI64(CodeGenerator& cg, llvm::Value* value) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    if (!value) return llvm::ConstantInt::get(i64Ty, 0);
    if (value->getType() == i64Ty) return value;
    if (value->getType()->isIntegerTy()) {
        unsigned bits = value->getType()->getIntegerBitWidth();
        if (bits < 64) return cg.builder.CreateSExt(value, i64Ty, "math.sext");
        if (bits > 64) return cg.builder.CreateTrunc(value, i64Ty, "math.trunc");
        return value;
    }
    if (value->getType()->isFloatingPointTy()) return cg.builder.CreateFPToSI(value, i64Ty, "math.fptosi");
    return llvm::ConstantInt::get(i64Ty, 0);
}

llvm::Function* getOrCreatePrintf(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("printf")) {
        return function;
    }
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto* functionTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(cg.context), {i8PtrTy}, true);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "printf", cg.module.get());
}

llvm::Function* getOrCreateScanf(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("scanf")) {
        return function;
    }
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto* functionTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(cg.context), {i8PtrTy}, true);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "scanf", cg.module.get());
}

llvm::Function* getOrCreateGetchar(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("getchar")) {
        return function;
    }
    auto* functionTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(cg.context), {}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "getchar", cg.module.get());
}

llvm::Function* getOrCreateStrlen(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("strlen")) {
        return function;
    }
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto* functionTy = llvm::FunctionType::get(llvm::Type::getInt64Ty(cg.context), {i8PtrTy}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "strlen", cg.module.get());
}

llvm::Function* getOrCreateWSAStartup(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("WSAStartup")) {
        return function;
    }
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto* functionTy = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(cg.context),
        {llvm::Type::getInt16Ty(cg.context), i8PtrTy},
        false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "WSAStartup", cg.module.get());
}

llvm::Function* getOrCreateSocket(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("socket")) {
        return function;
    }
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* functionTy = llvm::FunctionType::get(llvm::Type::getInt64Ty(cg.context), {i32Ty, i32Ty, i32Ty}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "socket", cg.module.get());
}

llvm::Function* getOrCreateInetAddr(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("inet_addr")) {
        return function;
    }
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto* functionTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(cg.context), {i8PtrTy}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "inet_addr", cg.module.get());
}

llvm::Function* getOrCreateHtons(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("htons")) {
        return function;
    }
    auto* i16Ty = llvm::Type::getInt16Ty(cg.context);
    auto* functionTy = llvm::FunctionType::get(i16Ty, {i16Ty}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "htons", cg.module.get());
}

llvm::Function* getOrCreateConnect(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("connect")) {
        return function;
    }
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto* functionTy = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(cg.context),
        {llvm::Type::getInt64Ty(cg.context), i8PtrTy, llvm::Type::getInt32Ty(cg.context)},
        false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "connect", cg.module.get());
}

llvm::Function* getOrCreateSend(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("send")) {
        return function;
    }
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* functionTy = llvm::FunctionType::get(i32Ty, {llvm::Type::getInt64Ty(cg.context), i8PtrTy, i32Ty, i32Ty}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "send", cg.module.get());
}

llvm::Function* getOrCreateRecv(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("recv")) {
        return function;
    }
    auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* functionTy = llvm::FunctionType::get(i32Ty, {llvm::Type::getInt64Ty(cg.context), i8PtrTy, i32Ty, i32Ty}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "recv", cg.module.get());
}

llvm::Function* getOrCreateCloseSocket(CodeGenerator& cg) {
    if (auto* function = cg.module->getFunction("closesocket")) {
        return function;
    }
    auto* functionTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(cg.context), {llvm::Type::getInt64Ty(cg.context)}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, "closesocket", cg.module.get());
}

llvm::Value* asCStringPointer(CodeGenerator& cg, llvm::Value* value) {
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());
    if (!value) {
        return llvm::ConstantPointerNull::get(i8PtrTy);
    }
    if (value->getType() == i8PtrTy) {
        return value;
    }
    if (value->getType()->isPointerTy()) {
        return cg.builder.CreateBitCast(value, i8PtrTy, "str.ptr");
    }
    return llvm::ConstantPointerNull::get(i8PtrTy);
}

llvm::Value* coercePrintfValue(CodeGenerator& cg, llvm::Value* value, const Type* type) {
    if (!value || !type) return value;
    if (type->isStringTy()) {
        return asCStringPointer(cg, value);
    }
    if (type->isCharTy()) {
        return value->getType()->isIntegerTy(32) ? value : cg.builder.CreateSExt(value, llvm::Type::getInt32Ty(cg.context), "char.printf");
    }
    if (type->getName() == "Boolean" || type->getName() == "Bool") {
        return value->getType()->isIntegerTy(32) ? value : cg.builder.CreateZExt(value, llvm::Type::getInt32Ty(cg.context), "bool.printf");
    }
    if (type->isIntegerTy()) {
        if (!value->getType()->isIntegerTy()) return value;
        unsigned bits = value->getType()->getIntegerBitWidth();
        if (bits < 32) return cg.builder.CreateSExt(value, llvm::Type::getInt32Ty(cg.context), "int.printf");
        return value;
    }
    if (type->isFloatTy()) {
        return cg.builder.CreateFPExt(value, llvm::Type::getDoubleTy(cg.context), "float.printf");
    }
    return value;
}

std::string printfFormatForType(const Type* type) {
    if (!type) return "%p";
    if (type->isStringTy()) return "%s";
    if (type->isCharTy()) return "%c";
    if (type->getName() == "Boolean" || type->getName() == "Bool") return "%d";
    if (type->getName() == "Long" || type->getName() == "Natural" || type->getName() == "Integer") return "%lld";
    if (type->isIntegerTy()) return "%d";
    if (type->isFloatTy() || type->isDoubleTy()) return "%f";
    return "%p";
}

llvm::Value* codegenNativeIOCall(
    const std::string& functionName,
    const std::vector<llvm::Value*>& argValues,
    const std::vector<std::unique_ptr<Type>>& argTypes) {
    auto& cg = CodeGenerator::getInstance();
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    auto* voidTy = llvm::Type::getVoidTy(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());

    if (functionName == "print" || functionName == "println") {
        for (size_t i = 0; i < argValues.size(); ++i) {
            const Type* argType = i < argTypes.size() ? argTypes[i].get() : nullptr;
            if (argType && argType->isStringTy()) {
                cg.builder.CreateCall(
                    getOrCreateRuntimeFunction(cg, "csec_print_string", voidTy, {i8PtrTy}),
                    {asCStringPointer(cg, argValues[i])});
            }
            else if (argType && argType->isCharTy()) {
                llvm::Value* value = argValues[i]->getType()->isIntegerTy(8)
                    ? argValues[i]
                    : cg.builder.CreateTrunc(argValues[i], i8Ty, "print.char");
                cg.builder.CreateCall(
                    getOrCreateRuntimeFunction(cg, "csec_print_char", voidTy, {i8Ty}),
                    {value});
            }
            else if (argType && (argType->getName() == "Boolean" || argType->getName() == "Bool")) {
                llvm::Value* value = argValues[i]->getType()->isIntegerTy(32)
                    ? argValues[i]
                    : cg.builder.CreateZExt(argValues[i], i32Ty, "print.bool");
                cg.builder.CreateCall(
                    getOrCreateRuntimeFunction(cg, "csec_print_bool", voidTy, {i32Ty}),
                    {value});
            }
            else if (argType && (argType->isFloatTy() || argType->isDoubleTy())) {
                cg.builder.CreateCall(
                    getOrCreateRuntimeFunction(cg, "csec_print_double", voidTy, {f64Ty}),
                    {coerceMathValueToDouble(cg, argValues[i])});
            }
            else if (argType && (isDictType(argType) || isSetType(argType)) && argValues[i]->getType()->isPointerTy()) {
                // Print a dictionary as Python's dict repr: {'a': 1, 'b': [2, 3]}.
                llvm::Value* handle = argValues[i];
                if (auto* slot = llvm::dyn_cast<llvm::AllocaInst>(handle)) {
                    if (slot->getAllocatedType()->isPointerTy()) handle = cg.builder.CreateLoad(i8PtrTy, slot, "dict.load");
                }
                cg.builder.CreateCall(
                    getOrCreateRuntimeFunction(cg, "csec_print_string", voidTy, {i8PtrTy}),
                    {codegenDictToString(handle)});
            }
            else if (argType && argType->getName() == "Nat" && argValues[i]->getType()->isPointerTy()) {
                // Print a Nat by its exact decimal expansion.
                llvm::Value* text = cg.builder.CreateCall(
                    getOrCreateRuntimeFunction(cg, "csec_bigint_to_string", i8PtrTy, {i8PtrTy}),
                    {argValues[i]});
                cg.builder.CreateCall(
                    getOrCreateRuntimeFunction(cg, "csec_print_string", voidTy, {i8PtrTy}),
                    {text});
            }
            else {
                cg.builder.CreateCall(
                    getOrCreateRuntimeFunction(cg, "csec_print_i64", voidTy, {i64Ty}),
                    {coerceMathValueToI64(cg, argValues[i])});
            }
        }
        if (functionName == "println") {
            cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_print_newline", voidTy, {}), {});
        }
        return nullptr;
    }

    if (!argValues.empty()) {
        std::cerr << "Error: Native input function '" << functionName << "' does not take arguments" << std::endl;
        return nullptr;
    }

    if (functionName == "readLine") {
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_read_line", i8PtrTy, {}), {}, "readline");
    }

    if (functionName == "readChar") {
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_read_char", i8Ty, {}), {}, "readchar");
    }

    if (functionName == "readInt") {
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_read_int", i32Ty, {}), {}, "readint");
    }

    if (functionName == "readDouble") {
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_read_double", f64Ty, {}), {}, "readdouble");
    }

    return nullptr;
}

llvm::Value* codegenNativeNetworkCall(
    const std::string& functionName,
    const std::vector<llvm::Value*>& argValues,
    const std::vector<std::unique_ptr<Type>>& argTypes) {
    auto& cg = CodeGenerator::getInstance();
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i16Ty = llvm::Type::getInt16Ty(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());

    if (functionName == "tcpConnect") {
        if (argValues.size() != 2) {
            std::cerr << "Error: tcpConnect(host, port) requires two arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_tcp_connect", i64Ty, {i8PtrTy, i32Ty}),
            {asCStringPointer(cg, argValues[0]), cg.builder.CreateTrunc(coerceMathValueToI64(cg, argValues[1]), i32Ty, "net.port")},
            "net.connect");
    }

    if (functionName == "tcpListen") {
        if (argValues.size() != 3) {
            std::cerr << "Error: tcpListen(host, port, backlog) requires three arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_tcp_listen", i64Ty, {i8PtrTy, i32Ty, i32Ty}),
            {
                asCStringPointer(cg, argValues[0]),
                cg.builder.CreateTrunc(coerceMathValueToI64(cg, argValues[1]), i32Ty, "net.port"),
                cg.builder.CreateTrunc(coerceMathValueToI64(cg, argValues[2]), i32Ty, "net.backlog")
            },
            "net.listen");
    }

    if (functionName == "tcpAccept") {
        if (argValues.size() != 1) {
            std::cerr << "Error: tcpAccept(socket) requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_tcp_accept", i64Ty, {i64Ty}),
            {coerceMathValueToI64(cg, argValues[0])},
            "net.accept");
    }

    if (functionName == "tcpSend") {
        if (argValues.size() != 2) {
            std::cerr << "Error: tcpSend(socket, data) requires two arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_tcp_send", i32Ty, {i64Ty, i8PtrTy}),
            {coerceMathValueToI64(cg, argValues[0]), asCStringPointer(cg, argValues[1])},
            "net.send");
    }

    if (functionName == "tcpRecv") {
        if (argValues.size() != 2) {
            std::cerr << "Error: tcpRecv(socket, maxBytes) requires two arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_tcp_recv", i8PtrTy, {i64Ty, i32Ty}),
            {coerceMathValueToI64(cg, argValues[0]), cg.builder.CreateTrunc(coerceMathValueToI64(cg, argValues[1]), i32Ty, "net.maxbytes")},
            "net.recv");
    }

    if (functionName == "tcpClose") {
        if (argValues.size() != 1) {
            std::cerr << "Error: tcpClose(socket) requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_tcp_close", i32Ty, {i64Ty}),
            {coerceMathValueToI64(cg, argValues[0])},
            "net.close");
    }

    auto asSocket = [&](llvm::Value* value) -> llvm::Value* {
        return coerceMathValueToI64(cg, value);
    };
    auto asI32 = [&](llvm::Value* value) -> llvm::Value* {
        if (!value) return llvm::ConstantInt::get(i32Ty, 0);
        if (value->getType() == i32Ty) return value;
        if (value->getType()->isIntegerTy()) {
            unsigned bits = value->getType()->getIntegerBitWidth();
            if (bits < 32) return cg.builder.CreateSExt(value, i32Ty, "net.sext");
            if (bits > 32) return cg.builder.CreateTrunc(value, i32Ty, "net.trunc");
        }
        if (value->getType()->isFloatingPointTy()) return cg.builder.CreateFPToSI(value, i32Ty, "net.fptosi");
        return llvm::ConstantInt::get(i32Ty, 0);
    };

    if (functionName == "tcpConnect") {
        if (argValues.size() != 2) {
            std::cerr << "Error: tcpConnect(host, port) requires two arguments" << std::endl;
            return nullptr;
        }

        auto* wsadataTy = llvm::ArrayType::get(i8Ty, 512);
        llvm::Value* wsadata = cg.builder.CreateAlloca(wsadataTy, nullptr, "net.wsadata");
        llvm::Value* wsadataPtr = cg.builder.CreateBitCast(wsadata, i8PtrTy, "net.wsadata.ptr");
        cg.builder.CreateCall(getOrCreateWSAStartup(cg), {llvm::ConstantInt::get(i16Ty, 0x0202), wsadataPtr});

        llvm::Value* sock = cg.builder.CreateCall(
            getOrCreateSocket(cg),
            {llvm::ConstantInt::get(i32Ty, 2), llvm::ConstantInt::get(i32Ty, 1), llvm::ConstantInt::get(i32Ty, 6)},
            "net.socket");

        auto* zero8Ty = llvm::ArrayType::get(i8Ty, 8);
        auto* sockaddrTy = llvm::StructType::get(cg.context, {i16Ty, i16Ty, i32Ty, zero8Ty});
        llvm::Value* addr = cg.builder.CreateAlloca(sockaddrTy, nullptr, "net.sockaddr");
        cg.builder.CreateStore(
            llvm::ConstantInt::get(i16Ty, 2),
            cg.builder.CreateStructGEP(sockaddrTy, addr, 0, "net.family"));
        llvm::Value* port16 = cg.builder.CreateTrunc(asI32(argValues[1]), i16Ty, "net.port16");
        cg.builder.CreateStore(
            cg.builder.CreateCall(getOrCreateHtons(cg), {port16}, "net.port.be"),
            cg.builder.CreateStructGEP(sockaddrTy, addr, 1, "net.port"));
        llvm::Value* host = asCStringPointer(cg, argValues[0]);
        cg.builder.CreateStore(
            cg.builder.CreateCall(getOrCreateInetAddr(cg), {host}, "net.addr"),
            cg.builder.CreateStructGEP(sockaddrTy, addr, 2, "net.addr.slot"));
        cg.builder.CreateStore(
            llvm::Constant::getNullValue(zero8Ty),
            cg.builder.CreateStructGEP(sockaddrTy, addr, 3, "net.zero"));
        cg.builder.CreateCall(
            getOrCreateConnect(cg),
            {sock, cg.builder.CreateBitCast(addr, i8PtrTy, "net.sockaddr.ptr"), llvm::ConstantInt::get(i32Ty, 16)});
        return sock;
    }

    if (functionName == "tcpSend") {
        if (argValues.size() != 2) {
            std::cerr << "Error: tcpSend(socket, data) requires two arguments" << std::endl;
            return nullptr;
        }
        llvm::Value* data = asCStringPointer(cg, argValues[1]);
        llvm::Value* length = cg.builder.CreateCall(getOrCreateStrlen(cg), {data}, "net.send.len64");
        llvm::Value* length32 = cg.builder.CreateTrunc(length, i32Ty, "net.send.len");
        return cg.builder.CreateCall(
            getOrCreateSend(cg),
            {asSocket(argValues[0]), data, length32, llvm::ConstantInt::get(i32Ty, 0)},
            "net.send");
    }

    if (functionName == "tcpRecv") {
        if (argValues.size() != 2) {
            std::cerr << "Error: tcpRecv(socket, maxBytes) requires two arguments" << std::endl;
            return nullptr;
        }
        llvm::Value* maxBytes = asI32(argValues[1]);
        llvm::Value* allocBytes = cg.builder.CreateZExt(cg.builder.CreateAdd(maxBytes, llvm::ConstantInt::get(i32Ty, 1)), i64Ty, "net.recv.alloc");
        llvm::Value* buffer = cg.builder.CreateCall(cg.mallocFunction, allocBytes, "net.recv.buffer");
        llvm::Value* read = cg.builder.CreateCall(
            getOrCreateRecv(cg),
            {asSocket(argValues[0]), buffer, maxBytes, llvm::ConstantInt::get(i32Ty, 0)},
            "net.recv");
        llvm::Value* readOk = cg.builder.CreateICmpSGE(read, llvm::ConstantInt::get(i32Ty, 0), "net.recv.ok");
        llvm::Value* terminatorIndex = cg.builder.CreateSelect(readOk, read, llvm::ConstantInt::get(i32Ty, 0), "net.recv.end");
        llvm::Value* termPtr = cg.builder.CreateGEP(i8Ty, buffer, terminatorIndex, "net.recv.term");
        cg.builder.CreateStore(llvm::ConstantInt::get(i8Ty, 0), termPtr);
        return buffer;
    }

    if (functionName == "tcpClose") {
        if (argValues.size() != 1) {
            std::cerr << "Error: tcpClose(socket) requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(getOrCreateCloseSocket(cg), {asSocket(argValues[0])}, "net.close");
    }

    return nullptr;
}

llvm::Value* codegenNativePosixCall(
    const std::string& functionName,
    const std::vector<llvm::Value*>& argValues) {
    auto& cg = CodeGenerator::getInstance();
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());

    auto asI32 = [&](llvm::Value* value, const char* name = "posix.i32") -> llvm::Value* {
        llvm::Value* i64Value = coerceMathValueToI64(cg, value);
        return cg.builder.CreateTrunc(i64Value, i32Ty, name);
    };

    auto requireArgCount = [&](size_t expected) -> bool {
        if (argValues.size() == expected) return true;
        std::cerr << "Error: " << functionName << " requires " << expected << " argument(s)" << std::endl;
        return false;
    };

    auto noArgIntConstant = [&](const std::string& runtimeName) -> llvm::Value* {
        if (!requireArgCount(0)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, runtimeName, i32Ty, {}), {}, "posix.const");
    };

    if (functionName == "posixFlagReadOnly") return noArgIntConstant("csec_posix_flag_read_only");
    if (functionName == "posixFlagWriteOnly") return noArgIntConstant("csec_posix_flag_write_only");
    if (functionName == "posixFlagReadWrite") return noArgIntConstant("csec_posix_flag_read_write");
    if (functionName == "posixFlagCreate") return noArgIntConstant("csec_posix_flag_create");
    if (functionName == "posixFlagTruncate") return noArgIntConstant("csec_posix_flag_truncate");
    if (functionName == "posixFlagAppend") return noArgIntConstant("csec_posix_flag_append");
    if (functionName == "posixSeekSet") return noArgIntConstant("csec_posix_seek_set");
    if (functionName == "posixSeekCur") return noArgIntConstant("csec_posix_seek_cur");
    if (functionName == "posixSeekEnd") return noArgIntConstant("csec_posix_seek_end");
    if (functionName == "posixAccessExists") return noArgIntConstant("csec_posix_access_exists");
    if (functionName == "posixAccessRead") return noArgIntConstant("csec_posix_access_read");
    if (functionName == "posixAccessWrite") return noArgIntConstant("csec_posix_access_write");
    if (functionName == "posixAccessExecute") return noArgIntConstant("csec_posix_access_execute");

    if (functionName == "posixOpen") {
        if (!requireArgCount(3)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_open", i32Ty, {i8PtrTy, i32Ty, i32Ty}),
            {asCStringPointer(cg, argValues[0]), asI32(argValues[1], "posix.open.flags"), asI32(argValues[2], "posix.open.mode")},
            "posix.open");
    }

    if (functionName == "posixRead") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_read", i8PtrTy, {i32Ty, i32Ty}),
            {asI32(argValues[0], "posix.read.fd"), asI32(argValues[1], "posix.read.max")},
            "posix.read");
    }

    if (functionName == "posixWrite") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_write", i32Ty, {i32Ty, i8PtrTy}),
            {asI32(argValues[0], "posix.write.fd"), asCStringPointer(cg, argValues[1])},
            "posix.write");
    }

    if (functionName == "posixClose") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_close", i32Ty, {i32Ty}),
            {asI32(argValues[0], "posix.close.fd")},
            "posix.close");
    }

    if (functionName == "posixLseek") {
        if (!requireArgCount(3)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_lseek", i64Ty, {i32Ty, i64Ty, i32Ty}),
            {asI32(argValues[0], "posix.lseek.fd"), coerceMathValueToI64(cg, argValues[1]), asI32(argValues[2], "posix.lseek.whence")},
            "posix.lseek");
    }

    if (functionName == "posixUnlink") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_unlink", i32Ty, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "posix.unlink");
    }

    if (functionName == "posixRename") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_rename", i32Ty, {i8PtrTy, i8PtrTy}),
            {asCStringPointer(cg, argValues[0]), asCStringPointer(cg, argValues[1])},
            "posix.rename");
    }

    if (functionName == "posixMkdir") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_mkdir", i32Ty, {i8PtrTy, i32Ty}),
            {asCStringPointer(cg, argValues[0]), asI32(argValues[1], "posix.mkdir.mode")},
            "posix.mkdir");
    }

    if (functionName == "posixRmdir") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_rmdir", i32Ty, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "posix.rmdir");
    }

    if (functionName == "posixChdir") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_chdir", i32Ty, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "posix.chdir");
    }

    if (functionName == "posixGetcwd") {
        if (!requireArgCount(0)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_posix_getcwd", i8PtrTy, {}), {}, "posix.getcwd");
    }

    if (functionName == "posixAccess") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_access", i32Ty, {i8PtrTy, i32Ty}),
            {asCStringPointer(cg, argValues[0]), asI32(argValues[1], "posix.access.mode")},
            "posix.access");
    }

    if (functionName == "posixGetenv") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_getenv", i8PtrTy, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "posix.getenv");
    }

    if (functionName == "posixSetenv") {
        if (!requireArgCount(3)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_setenv", i32Ty, {i8PtrTy, i8PtrTy, i32Ty}),
            {asCStringPointer(cg, argValues[0]), asCStringPointer(cg, argValues[1]), asI32(argValues[2], "posix.setenv.overwrite")},
            "posix.setenv");
    }

    if (functionName == "posixUnsetenv") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_unsetenv", i32Ty, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "posix.unsetenv");
    }

    if (functionName == "posixSleep") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_posix_sleep", i32Ty, {i32Ty}),
            {asI32(argValues[0], "posix.sleep.seconds")},
            "posix.sleep");
    }

    if (functionName == "posixTime") {
        if (!requireArgCount(0)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_posix_time", i64Ty, {}), {}, "posix.time");
    }

    if (functionName == "posixErrno") {
        if (!requireArgCount(0)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_posix_errno", i32Ty, {}), {}, "posix.errno");
    }

    return nullptr;
}

llvm::Value* codegenNativeMicrothreadCall(
    const std::string& functionName,
    const std::vector<std::unique_ptr<ASTNode>>& arguments,
    const std::vector<llvm::Value*>& argValues) {
    auto& cg = CodeGenerator::getInstance();
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* voidTy = llvm::Type::getVoidTy(cg.context);
    auto* opaquePtr = llvm::PointerType::getUnqual(cg.context);
    auto requireArgCount = [&](size_t count) {
        if (argValues.size() == count) return true;
        std::cerr << "Error: " << functionName << "() requires " << count << " argument(s)" << std::endl;
        return false;
    };
    auto asPid = [&](llvm::Value* value) {
        return cg.builder.CreateTrunc(coerceMathValueToI64(cg, value), i32Ty, "proc.pid");
    };
    auto asMessageResult = [&](llvm::Value* raw) {
        return cg.builder.CreateTrunc(raw, i32Ty, "proc.msg");
    };

    if (functionName == "spawn" || functionName == "threadStart") {
        if (!requireArgCount(1)) return nullptr;
        llvm::Value* closure = argValues[0];
        if (!closure || !closure->getType()->isPointerTy()) {
            std::cerr << "Error: " << functionName << "() expects a zero-argument lambda or function" << std::endl;
            return nullptr;
        }
        // Closures are { code, env }. The runtime copies `envSize` bytes of the environment so the
        // child shares nothing with (and does not depend on) the spawning frame.
        auto* closureType = llvm::StructType::get(cg.context, { opaquePtr, opaquePtr });
        llvm::Value* code = cg.builder.CreateLoad(opaquePtr,
            cg.builder.CreateStructGEP(closureType, closure, 0, "spawn.code.ptr"), "spawn.code");
        llvm::Value* env = cg.builder.CreateLoad(opaquePtr,
            cg.builder.CreateStructGEP(closureType, closure, 1, "spawn.env.ptr"), "spawn.env");
        // sizeof(env) as a constant expression, resolved with the target's data layout.
        llvm::Constant* envSizeValue = llvm::ConstantInt::get(i64Ty, 0);
        if (auto* lambda = dynamic_cast<LambdaExpressionNode*>(arguments[0].get())) {
            if (lambda->environmentType) {
                envSizeValue = llvm::ConstantExpr::getPtrToInt(llvm::ConstantExpr::getGetElementPtr(lambda->environmentType,
                    llvm::ConstantPointerNull::get(opaquePtr), llvm::ArrayRef<llvm::Constant*>{ llvm::ConstantInt::get(i32Ty, 1) }), i64Ty);
            }
        }
        if (functionName == "threadStart") {
            return cg.builder.CreateCall(
                getOrCreateRuntimeFunction(cg, "csec_thread_start", i64Ty, {opaquePtr, opaquePtr, i64Ty}),
                {code, env, envSizeValue}, "thread.start");
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_process_spawn", i32Ty, {opaquePtr, opaquePtr, i64Ty}),
            {code, env, envSizeValue}, "proc.spawn");
    }
    if (functionName == "self") {
        if (!requireArgCount(0)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_process_self", i32Ty, {}), {}, "proc.self");
    }
    if (functionName == "send") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_process_send", i32Ty, {i32Ty, i64Ty}),
            {asPid(argValues[0]), coerceMathValueToI64(cg, argValues[1])}, "proc.send");
    }
    if (functionName == "receive") {
        if (!requireArgCount(0)) return nullptr;
        return asMessageResult(cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_process_receive", i64Ty, {}), {}, "proc.receive"));
    }
    if (functionName == "receiveTimeout") {
        if (!requireArgCount(2)) return nullptr;
        return asMessageResult(cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_process_receive_timeout", i64Ty, {i64Ty, i64Ty}),
            {coerceMathValueToI64(cg, argValues[0]), coerceMathValueToI64(cg, argValues[1])}, "proc.receive.timeout"));
    }
    if (functionName == "yield") {
        if (!requireArgCount(0)) return nullptr;
        cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_process_yield", voidTy, {}), {});
        return llvm::ConstantInt::get(i32Ty, 0);
    }
    if (functionName == "isAlive") {
        if (!requireArgCount(1)) return nullptr;
        llvm::Value* raw = cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_process_alive", i32Ty, {i32Ty}), {asPid(argValues[0])}, "proc.alive.raw");
        return cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Ty, 0), "proc.alive");
    }
    if (functionName == "monitor") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_process_monitor", i32Ty, {i32Ty, i64Ty}),
            {asPid(argValues[0]), coerceMathValueToI64(cg, argValues[1])}, "proc.monitor");
    }
    if (functionName == "mailboxSize") {
        if (!requireArgCount(0)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_process_mailbox_size", i32Ty, {}), {}, "proc.mailbox");
    }
    if (functionName == "processCount") {
        if (!requireArgCount(0)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_process_count", i32Ty, {}), {}, "proc.count");
    }
    if (functionName == "joinProcesses") {
        if (!requireArgCount(0)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_process_join", i32Ty, {}), {}, "proc.join");
    }
    return nullptr;
}

llvm::Value* codegenNativeParallelCall(
    const std::string& functionName,
    const std::vector<llvm::Value*>& argValues) {
    auto& cg = CodeGenerator::getInstance();
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* voidTy = llvm::Type::getVoidTy(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());

    if (functionName == "parallelThreads") {
        if (!argValues.empty()) {
            std::cerr << "Error: parallelThreads() does not take arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_parallel_get_num_threads", i32Ty, {}),
            {},
            "parallel.threads");
    }

    if (functionName == "setParallelThreads") {
        if (argValues.size() != 1) {
            std::cerr << "Error: setParallelThreads(count) requires one argument" << std::endl;
            return nullptr;
        }
        cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_parallel_set_num_threads", voidTy, {i32Ty}),
            {cg.builder.CreateTrunc(coerceMathValueToI64(cg, argValues[0]), i32Ty, "parallel.thread.count")});
        return llvm::ConstantInt::get(i32Ty, 0);
    }

    if (functionName == "parallelBackendAvailable") {
        if (argValues.size() != 1) {
            std::cerr << "Error: parallelBackendAvailable(name) requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_parallel_backend_available", i32Ty, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "parallel.backend.available");
    }

    if (functionName == "parallelBackendImplemented") {
        if (argValues.size() != 1) {
            std::cerr << "Error: parallelBackendImplemented(name) requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_parallel_backend_implemented", i32Ty, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "parallel.backend.implemented");
    }

    return nullptr;
}

llvm::Value* codegenNativeSystemFileCall(
    const std::string& functionName,
    const std::vector<llvm::Value*>& argValues) {
    auto& cg = CodeGenerator::getInstance();
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());

    auto requireArgCount = [&](size_t expected) -> bool {
        if (argValues.size() == expected) return true;
        std::cerr << "Error: " << functionName << " requires " << expected << " argument(s)" << std::endl;
        return false;
    };

    if (functionName == "systemFileReadAllText") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_file_read_all_text", i8PtrTy, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "file.read_all_text");
    }

    if (functionName == "systemFileWriteAllText") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_file_write_all_text", i32Ty, {i8PtrTy, i8PtrTy}),
            {asCStringPointer(cg, argValues[0]), asCStringPointer(cg, argValues[1])},
            "file.write_all_text");
    }

    if (functionName == "systemFileAppendAllText") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_file_append_all_text", i32Ty, {i8PtrTy, i8PtrTy}),
            {asCStringPointer(cg, argValues[0]), asCStringPointer(cg, argValues[1])},
            "file.append_all_text");
    }

    if (functionName == "systemFileExists") {
        if (!requireArgCount(1)) return nullptr;
        auto* raw = cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_file_exists", i32Ty, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "file.exists.i32");
        return cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Ty, 0), "file.exists");
    }

    if (functionName == "systemFileDelete") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_file_delete", i32Ty, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "file.delete");
    }

    return nullptr;
}

llvm::Value* codegenNativeDynamicLibraryCall(
    const std::string& functionName,
    const std::vector<llvm::Value*>& rawArgValues,
    const std::vector<std::unique_ptr<Type>>& argTypes) {
    auto& cg = CodeGenerator::getInstance();
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());
    std::vector<llvm::Value*> argValues = rawArgValues;
    for (size_t i = 0; i < argValues.size() && i < argTypes.size(); ++i) {
        if (!argValues[i] || !argValues[i]->getType()->isPointerTy()) continue;
        if (!argTypes[i] || argTypes[i]->isStringTy() || TensorRuntime::isTensorTypeName(argTypes[i]->getName())) continue;
        if (auto* valueType = cg.getLLVMType(argTypes[i].get())) {
            argValues[i] = cg.builder.CreateLoad(valueType, argValues[i], "dyn.arg.load");
        }
    }

    auto requireArgCount = [&](size_t expected) -> bool {
        if (argValues.size() == expected) return true;
        std::cerr << "Error: " << functionName << " requires " << expected << " argument(s)" << std::endl;
        return false;
    };

    if (functionName == "loadLibrary") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_load_library", i64Ty, {i8PtrTy}),
            {asCStringPointer(cg, argValues[0])},
            "dynlib.handle");
    }
    if (functionName == "getSymbol") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_get_symbol", i64Ty, {i64Ty, i8PtrTy}),
            {coerceMathValueToI64(cg, argValues[0]), asCStringPointer(cg, argValues[1])},
            "dynlib.symbol");
    }
    if (functionName == "closeLibrary") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_close_library", i32Ty, {i64Ty}),
            {coerceMathValueToI64(cg, argValues[0])},
            "dynlib.close");
    }
    if (functionName == "callNative0") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_call_native0", i64Ty, {i64Ty}), {coerceMathValueToI64(cg, argValues[0])}, "native.call");
    }
    if (functionName == "callNative1") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_call_native1", i64Ty, {i64Ty, i64Ty}), {coerceMathValueToI64(cg, argValues[0]), coerceMathValueToI64(cg, argValues[1])}, "native.call");
    }
    if (functionName == "callNative2") {
        if (!requireArgCount(3)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_call_native2", i64Ty, {i64Ty, i64Ty, i64Ty}), {coerceMathValueToI64(cg, argValues[0]), coerceMathValueToI64(cg, argValues[1]), coerceMathValueToI64(cg, argValues[2])}, "native.call");
    }
    if (functionName == "callNative3") {
        if (!requireArgCount(4)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_call_native3", i64Ty, {i64Ty, i64Ty, i64Ty, i64Ty}), {coerceMathValueToI64(cg, argValues[0]), coerceMathValueToI64(cg, argValues[1]), coerceMathValueToI64(cg, argValues[2]), coerceMathValueToI64(cg, argValues[3])}, "native.call");
    }
    if (functionName == "callNativeDouble0") {
        if (!requireArgCount(1)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_call_native_double0", f64Ty, {i64Ty}), {coerceMathValueToI64(cg, argValues[0])}, "native.dcall");
    }
    if (functionName == "callNativeDouble1") {
        if (!requireArgCount(2)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_call_native_double1", f64Ty, {i64Ty, f64Ty}), {coerceMathValueToI64(cg, argValues[0]), coerceMathValueToDouble(cg, argValues[1])}, "native.dcall");
    }
    if (functionName == "callNativeDouble2") {
        if (!requireArgCount(3)) return nullptr;
        return cg.builder.CreateCall(getOrCreateRuntimeFunction(cg, "csec_call_native_double2", f64Ty, {i64Ty, f64Ty, f64Ty}), {coerceMathValueToI64(cg, argValues[0]), coerceMathValueToDouble(cg, argValues[1]), coerceMathValueToDouble(cg, argValues[2])}, "native.dcall");
    }

    return nullptr;
}

llvm::Value* codegenNativeScalarMathCall(
    const std::string& functionName,
    const std::vector<llvm::Value*>& rawArgValues,
    const std::vector<std::unique_ptr<Type>>& argTypes) {
    auto& cg = CodeGenerator::getInstance();
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    std::vector<llvm::Value*> argValues = rawArgValues;
    for (size_t i = 0; i < argValues.size() && i < argTypes.size(); ++i) {
        if (!argValues[i] || !argValues[i]->getType()->isPointerTy()) continue;
        if (argTypes[i] && TensorRuntime::isTensorTypeName(argTypes[i]->getName())) continue;
        llvm::Type* valueType = cg.getLLVMType(argTypes[i].get());
        if (valueType) {
            argValues[i] = cg.builder.CreateLoad(valueType, argValues[i], "math.arg.load");
        }
    }
    auto callUnaryRuntime = [&](const std::string& runtimeName) -> llvm::Value* {
        if (argValues.size() != 1) {
            std::cerr << "Error: Math function '" << functionName << "' requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, runtimeName, f64Ty, {f64Ty}),
            {coerceMathValueToDouble(cg, argValues[0])},
            "math.call");
    };
    auto callBinaryRuntime = [&](const std::string& runtimeName) -> llvm::Value* {
        if (argValues.size() != 2) {
            std::cerr << "Error: Math function '" << functionName << "' requires two arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, runtimeName, f64Ty, {f64Ty, f64Ty}),
            {coerceMathValueToDouble(cg, argValues[0]), coerceMathValueToDouble(cg, argValues[1])},
            "math.call");
    };

    if (functionName == "sin") return callUnaryRuntime("csec_math_sin");
    if (functionName == "cos") return callUnaryRuntime("csec_math_cos");
    if (functionName == "tan") return callUnaryRuntime("csec_math_tan");
    if (functionName == "cot") return callUnaryRuntime("csec_math_cot");
    if (functionName == "sec") return callUnaryRuntime("csec_math_sec");
    if (functionName == "csc") return callUnaryRuntime("csec_math_csc");
    if (functionName == "arcsin") return callUnaryRuntime("csec_math_asin");
    if (functionName == "arccos") return callUnaryRuntime("csec_math_acos");
    if (functionName == "arctan") return callUnaryRuntime("csec_math_atan");
    if (functionName == "sinh") return callUnaryRuntime("csec_math_sinh");
    if (functionName == "cosh") return callUnaryRuntime("csec_math_cosh");
    if (functionName == "tanh") return callUnaryRuntime("csec_math_tanh");
    if (functionName == "coth") return callUnaryRuntime("csec_math_coth");
    if (functionName == "sqrt") return callUnaryRuntime("csec_math_sqrt");
    if (functionName == "ln") return callUnaryRuntime("csec_math_log");
    if (functionName == "lg") return callUnaryRuntime("csec_math_log10");
    if (functionName == "exp") return callUnaryRuntime("csec_math_exp");
    if (functionName == "abs") return callUnaryRuntime("csec_math_abs");
    if (functionName == "sign") return callUnaryRuntime("csec_math_sign");
    if (functionName == "floor") return callUnaryRuntime("csec_math_floor");
    if (functionName == "ceil") return callUnaryRuntime("csec_math_ceil");
    if (functionName == "round") return callUnaryRuntime("csec_math_round");
    if (functionName == "pow") return callBinaryRuntime("csec_math_pow");
    if (functionName == "frac") return callBinaryRuntime("csec_math_frac");
    if (functionName == "binom") return callBinaryRuntime("csec_math_binom");
    if (functionName == "lcm") return callBinaryRuntime("csec_math_lcm");

    if (functionName == "approxEq") {
        if (argValues.size() != 3) {
            std::cerr << "Error: approxEq(a, b, eps) requires three arguments" << std::endl;
            return nullptr;
        }
        llvm::Value* diff = cg.builder.CreateFSub(
            coerceMathValueToDouble(cg, argValues[0]),
            coerceMathValueToDouble(cg, argValues[1]),
            "approx.diff");
        llvm::Value* absDiff = cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_math_abs", f64Ty, {f64Ty}),
            {diff},
            "approx.abs");
        return cg.builder.CreateFCmpOLE(absDiff, coerceMathValueToDouble(cg, argValues[2]), "approx.eq");
    }

    if (functionName == "clamp") {
        if (argValues.size() != 3) {
            std::cerr << "Error: clamp(value, min, max) requires three arguments" << std::endl;
            return nullptr;
        }
        llvm::Value* value = coerceMathValueToDouble(cg, argValues[0]);
        llvm::Value* minValue = coerceMathValueToDouble(cg, argValues[1]);
        llvm::Value* maxValue = coerceMathValueToDouble(cg, argValues[2]);
        llvm::Value* aboveMin = cg.builder.CreateSelect(cg.builder.CreateFCmpOLT(value, minValue), minValue, value, "clamp.min");
        return cg.builder.CreateSelect(cg.builder.CreateFCmpOGT(aboveMin, maxValue), maxValue, aboveMin, "clamp.max");
    }

    if (functionName == "between") {
        if (argValues.size() != 3) {
            std::cerr << "Error: between(value, min, max) requires three arguments" << std::endl;
            return nullptr;
        }
        llvm::Value* value = coerceMathValueToDouble(cg, argValues[0]);
        llvm::Value* minValue = coerceMathValueToDouble(cg, argValues[1]);
        llvm::Value* maxValue = coerceMathValueToDouble(cg, argValues[2]);
        return cg.builder.CreateAnd(
            cg.builder.CreateFCmpOGE(value, minValue, "between.lo"),
            cg.builder.CreateFCmpOLE(value, maxValue, "between.hi"),
            "between.result");
    }

    if (functionName == "emptySet") {
        if (!argValues.empty()) {
            std::cerr << "Error: emptySet() does not take arguments" << std::endl;
            return nullptr;
        }
        return llvm::ConstantInt::get(i64Ty, 0);
    }

    if (functionName == "singleton") {
        if (argValues.size() != 1) {
            std::cerr << "Error: singleton(value) requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateShl(llvm::ConstantInt::get(i64Ty, 1), coerceMathValueToI64(cg, argValues[0]), "set.singleton");
    }

    if (functionName == "cardinality") {
        if (argValues.size() != 1) {
            std::cerr << "Error: cardinality(set) requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_set_cardinality", i64Ty, {i64Ty}),
            {coerceMathValueToI64(cg, argValues[0])},
            "set.cardinality");
    }

    if (functionName == "disjoint") {
        if (argValues.size() != 2) {
            std::cerr << "Error: disjoint(a, b) requires two arguments" << std::endl;
            return nullptr;
        }
        llvm::Value* overlap = cg.builder.CreateAnd(coerceMathValueToI64(cg, argValues[0]), coerceMathValueToI64(cg, argValues[1]), "set.overlap");
        return cg.builder.CreateICmpEQ(overlap, llvm::ConstantInt::get(i64Ty, 0), "set.disjoint");
    }

    if (functionName == "complement") {
        if (argValues.size() != 2) {
            std::cerr << "Error: complement(set, universe) requires two arguments" << std::endl;
            return nullptr;
        }
        llvm::Value* setValue = coerceMathValueToI64(cg, argValues[0]);
        llvm::Value* universe = coerceMathValueToI64(cg, argValues[1]);
        return cg.builder.CreateAnd(universe, cg.builder.CreateNot(setValue, "set.not"), "set.complement");
    }

    if (functionName == "log") {
        if (argValues.size() == 1) return callUnaryRuntime("csec_math_log");
        if (argValues.size() == 2) return callBinaryRuntime("csec_math_log_base");
        std::cerr << "Error: Math function 'log' requires one or two arguments" << std::endl;
        return nullptr;
    }

    if (functionName == "min" || functionName == "max" || functionName == "gcd") {
        if (argValues.empty() || (functionName == "gcd" && argValues.size() < 2)) {
            std::cerr << "Error: Math function '" << functionName << "' has too few arguments" << std::endl;
            return nullptr;
        }
        std::string runtimeName = functionName == "min" ? "csec_math_min" :
                                  functionName == "max" ? "csec_math_max" : "csec_math_gcd";
        llvm::Value* result = coerceMathValueToDouble(cg, argValues[0]);
        for (size_t i = 1; i < argValues.size(); ++i) {
            result = cg.builder.CreateCall(
                getOrCreateRuntimeFunction(cg, runtimeName, f64Ty, {f64Ty, f64Ty}),
                {result, coerceMathValueToDouble(cg, argValues[i])},
                "math.fold");
        }
        return result;
    }

    auto unary = [&](const std::string& cName) -> llvm::Value* {
        if (argValues.size() != 1) {
            std::cerr << "Error: Math function '" << functionName << "' requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(getOrCreateUnaryDoubleFunction(cg, cName), {coerceMathValueToDouble(cg, argValues[0])}, "math.call");
    };
    auto binary = [&](const std::string& cName) -> llvm::Value* {
        if (argValues.size() != 2) {
            std::cerr << "Error: Math function '" << functionName << "' requires two arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateBinaryDoubleFunction(cg, cName),
            {coerceMathValueToDouble(cg, argValues[0]), coerceMathValueToDouble(cg, argValues[1])},
            "math.call");
    };

    if (functionName == "sin") return unary("sin");
    if (functionName == "cos") return unary("cos");
    if (functionName == "tan") return unary("tan");
    if (functionName == "arcsin") return unary("asin");
    if (functionName == "arccos") return unary("acos");
    if (functionName == "arctan") return unary("atan");
    if (functionName == "sinh") return unary("sinh");
    if (functionName == "cosh") return unary("cosh");
    if (functionName == "tanh") return unary("tanh");
    if (functionName == "sqrt") return unary("sqrt");
    if (functionName == "ln") return unary("log");
    if (functionName == "lg") return unary("log10");
    if (functionName == "exp") return unary("exp");
    if (functionName == "pow") return binary("pow");

    if (functionName == "log") {
        if (argValues.size() == 1) return unary("log");
        if (argValues.size() == 2) {
            llvm::Value* valueLog = cg.builder.CreateCall(
                getOrCreateUnaryDoubleFunction(cg, "log"),
                {coerceMathValueToDouble(cg, argValues[1])},
                "math.log.value");
            llvm::Value* baseLog = cg.builder.CreateCall(
                getOrCreateUnaryDoubleFunction(cg, "log"),
                {coerceMathValueToDouble(cg, argValues[0])},
                "math.log.base");
            return cg.builder.CreateFDiv(valueLog, baseLog, "math.log");
        }
        std::cerr << "Error: Math function 'log' requires one or two arguments" << std::endl;
        return nullptr;
    }

    if (functionName == "cot" || functionName == "sec" || functionName == "csc" || functionName == "coth") {
        if (argValues.size() != 1) {
            std::cerr << "Error: Math function '" << functionName << "' requires one argument" << std::endl;
            return nullptr;
        }
        std::string cName = functionName == "cot" ? "tan" :
                            functionName == "sec" ? "cos" :
                            functionName == "csc" ? "sin" : "tanh";
        llvm::Value* denominator = cg.builder.CreateCall(
            getOrCreateUnaryDoubleFunction(cg, cName),
            {coerceMathValueToDouble(cg, argValues[0])},
            "math.trig.den");
        return cg.builder.CreateFDiv(llvm::ConstantFP::get(f64Ty, 1.0), denominator, "math.recip");
    }

    if (functionName == "frac") {
        if (argValues.size() != 2) {
            std::cerr << "Error: Math function 'frac' requires two arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateFDiv(coerceMathValueToDouble(cg, argValues[0]), coerceMathValueToDouble(cg, argValues[1]), "math.frac");
    }

    if (functionName == "min" || functionName == "max") {
        if (argValues.empty()) {
            std::cerr << "Error: Math function '" << functionName << "' requires at least one argument" << std::endl;
            return nullptr;
        }
        llvm::Value* result = coerceMathValueToDouble(cg, argValues[0]);
        for (size_t i = 1; i < argValues.size(); ++i) {
            llvm::Value* next = coerceMathValueToDouble(cg, argValues[i]);
            llvm::Value* cmp = functionName == "min"
                ? cg.builder.CreateFCmpOLT(next, result, "math.min.cmp")
                : cg.builder.CreateFCmpOGT(next, result, "math.max.cmp");
            result = cg.builder.CreateSelect(cmp, next, result, "math.select");
        }
        return result;
    }

    if (functionName == "gcd") {
        if (argValues.size() < 2) {
            std::cerr << "Error: Math function 'gcd' requires at least two arguments" << std::endl;
            return nullptr;
        }
        llvm::Value* result = coerceMathValueToI64(cg, argValues[0]);
        auto* gcdFn = getOrCreateGcdI64(cg);
        for (size_t i = 1; i < argValues.size(); ++i) {
            result = cg.builder.CreateCall(gcdFn, {result, coerceMathValueToI64(cg, argValues[i])}, "math.gcd");
        }
        return cg.builder.CreateSIToFP(result, f64Ty, "math.gcd.real");
    }

    if (functionName == "binom") {
        if (argValues.size() != 2) {
            std::cerr << "Error: Math function 'binom' requires two arguments" << std::endl;
            return nullptr;
        }
        auto* tgammaFn = getOrCreateUnaryDoubleFunction(cg, "tgamma");
        llvm::Value* n = coerceMathValueToDouble(cg, argValues[0]);
        llvm::Value* k = coerceMathValueToDouble(cg, argValues[1]);
        llvm::Value* one = llvm::ConstantFP::get(f64Ty, 1.0);
        llvm::Value* numerator = cg.builder.CreateCall(tgammaFn, {cg.builder.CreateFAdd(n, one)}, "math.binom.n");
        llvm::Value* kFact = cg.builder.CreateCall(tgammaFn, {cg.builder.CreateFAdd(k, one)}, "math.binom.k");
        llvm::Value* nkFact = cg.builder.CreateCall(tgammaFn, {cg.builder.CreateFAdd(cg.builder.CreateFSub(n, k), one)}, "math.binom.nk");
        return cg.builder.CreateFDiv(numerator, cg.builder.CreateFMul(kFact, nkFact), "math.binom");
    }

    return nullptr;
}
}

llvm::Value* codegenNativeSimulationCall(const std::string& functionName, const std::vector<llvm::Value*>& argValues) {
    auto& cg = CodeGenerator::getInstance();
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);

    auto asI32 = [&](llvm::Value* value, const char* name) -> llvm::Value* {
        llvm::Value* i64Value = coerceMathValueToI64(cg, value);
        return cg.builder.CreateTrunc(i64Value, i32Ty, name);
    };
    auto asF64 = [&](llvm::Value* value) -> llvm::Value* {
        return coerceMathValueToDouble(cg, value);
    };

    if (functionName == "mdSimulate") {
        if (argValues.size() != 5) {
            std::cerr << "Error: mdSimulate(atomCount, bondCount, steps, dt, temperature) requires five arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_sim_md_lennard_jones", f64Ty, {i32Ty, i32Ty, i32Ty, f64Ty, f64Ty}),
            {
                asI32(argValues[0], "sim.md.atoms"),
                asI32(argValues[1], "sim.md.bonds"),
                asI32(argValues[2], "sim.md.steps"),
                asF64(argValues[3]),
                asF64(argValues[4])
            },
            "sim.md");
    }

    if (functionName == "cfdSimulate") {
        if (argValues.size() != 6) {
            std::cerr << "Error: cfdSimulate(width, height, steps, dt, viscosity, velocity) requires six arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_sim_cfd_lid_cavity", f64Ty, {i32Ty, i32Ty, i32Ty, f64Ty, f64Ty, f64Ty}),
            {
                asI32(argValues[0], "sim.cfd.width"),
                asI32(argValues[1], "sim.cfd.height"),
                asI32(argValues[2], "sim.cfd.steps"),
                asF64(argValues[3]),
                asF64(argValues[4]),
                asF64(argValues[5])
            },
            "sim.cfd");
    }

    if (functionName == "proteinMcmc") {
        if (argValues.size() != 3) {
            std::cerr << "Error: proteinMcmc(residueCount, steps, temperature) requires three arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_sim_protein_mcmc", f64Ty, {i32Ty, i32Ty, f64Ty}),
            {
                asI32(argValues[0], "sim.protein.residues"),
                asI32(argValues[1], "sim.protein.steps"),
                asF64(argValues[2])
            },
            "sim.protein");
    }

    if (functionName == "blackHoleMerge") {
        if (argValues.size() != 6) {
            std::cerr << "Error: blackHoleMerge(mass1, mass2, separation, relativeVelocity, steps, dt) requires six arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_sim_black_hole_merge", f64Ty, {f64Ty, f64Ty, f64Ty, f64Ty, i32Ty, f64Ty}),
            {
                asF64(argValues[0]),
                asF64(argValues[1]),
                asF64(argValues[2]),
                asF64(argValues[3]),
                asI32(argValues[4], "sim.blackhole.steps"),
                asF64(argValues[5])
            },
            "sim.blackhole");
    }

    return nullptr;
}

llvm::Value* codegenBuiltinOdeCall(const std::string& functionName, const std::vector<llvm::Value*>& argValues) {
    auto& cg = CodeGenerator::getInstance();
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);

    if (functionName == "numericDerivative") {
        if (argValues.size() != 3) {
            std::cerr << "Error: numericDerivative(f, order, x) requires three arguments" << std::endl;
            return nullptr;
        }

        auto* fType = llvm::FunctionType::get(f64Ty, {f64Ty}, false);
        llvm::Value* derivativeFunction = argValues[0];
        llvm::Value* order = cg.builder.CreateTrunc(coerceMathValueToI64(cg, argValues[1]), i32Ty, "derivative.order");
        llvm::Value* x = coerceMathValueToDouble(cg, argValues[2]);
        llvm::Value* h = llvm::ConstantFP::get(f64Ty, 1e-5);
        llvm::Value* two = llvm::ConstantFP::get(f64Ty, 2.0);

        llvm::Value* xPlus = cg.builder.CreateFAdd(x, h, "derivative.x.plus");
        llvm::Value* xMinus = cg.builder.CreateFSub(x, h, "derivative.x.minus");
        llvm::Value* fPlus = cg.builder.CreateCall(fType, derivativeFunction, {xPlus}, "derivative.f.plus");
        llvm::Value* fMinus = cg.builder.CreateCall(fType, derivativeFunction, {xMinus}, "derivative.f.minus");
        llvm::Value* first = cg.builder.CreateFDiv(
            cg.builder.CreateFSub(fPlus, fMinus, "derivative.first.numerator"),
            cg.builder.CreateFMul(two, h, "derivative.first.denominator"),
            "derivative.first");

        llvm::Value* fCenter = cg.builder.CreateCall(fType, derivativeFunction, {x}, "derivative.f.center");
        llvm::Value* secondNumerator = cg.builder.CreateFAdd(
            cg.builder.CreateFSub(fPlus, cg.builder.CreateFMul(two, fCenter, "derivative.twice.center"), "derivative.second.left"),
            fMinus,
            "derivative.second.numerator");
        llvm::Value* second = cg.builder.CreateFDiv(
            secondNumerator,
            cg.builder.CreateFMul(h, h, "derivative.second.denominator"),
            "derivative.second");

        llvm::Value* isSecond = cg.builder.CreateICmpEQ(order, llvm::ConstantInt::get(i32Ty, 2), "derivative.is.second");
        return cg.builder.CreateSelect(isSecond, second, first, "derivative.result");
    }

    if (functionName == "integral") {
        if (argValues.size() != 3 && argValues.size() != 4) {
            std::cerr << "Error: integral(f, a, b[, steps]) requires three or four arguments" << std::endl;
            return nullptr;
        }

        auto* function = cg.builder.GetInsertBlock()->getParent();
        auto* fType = llvm::FunctionType::get(f64Ty, {f64Ty}, false);
        llvm::Value* integrand = argValues[0];
        llvm::Value* a = coerceMathValueToDouble(cg, argValues[1]);
        llvm::Value* b = coerceMathValueToDouble(cg, argValues[2]);
        llvm::Value* steps = argValues.size() == 4
            ? cg.builder.CreateTrunc(coerceMathValueToI64(cg, argValues[3]), i32Ty, "integral.steps")
            : llvm::ConstantInt::get(i32Ty, 1024);

        llvm::Value* zero = llvm::ConstantInt::get(i32Ty, 0);
        llvm::Value* one = llvm::ConstantInt::get(i32Ty, 1);
        steps = cg.builder.CreateSelect(
            cg.builder.CreateICmpSGT(steps, zero, "integral.positive.steps"),
            steps,
            one,
            "integral.safe.steps");

        llvm::Value* stepsAsDouble = cg.builder.CreateSIToFP(steps, f64Ty, "integral.steps.fp");
        llvm::Value* width = cg.builder.CreateFDiv(cg.builder.CreateFSub(b, a, "integral.range"), stepsAsDouble, "integral.width");
        llvm::Value* half = llvm::ConstantFP::get(f64Ty, 0.5);

        llvm::Value* iPtr = cg.builder.CreateAlloca(i32Ty, nullptr, "integral.i");
        llvm::Value* sumPtr = cg.builder.CreateAlloca(f64Ty, nullptr, "integral.sum");
        cg.builder.CreateStore(zero, iPtr);
        cg.builder.CreateStore(llvm::ConstantFP::get(f64Ty, 0.0), sumPtr);

        auto* condBB = llvm::BasicBlock::Create(cg.context, "integral.cond", function);
        auto* bodyBB = llvm::BasicBlock::Create(cg.context, "integral.body", function);
        auto* endBB = llvm::BasicBlock::Create(cg.context, "integral.end", function);

        cg.builder.CreateBr(condBB);

        cg.builder.SetInsertPoint(condBB);
        llvm::Value* i = cg.builder.CreateLoad(i32Ty, iPtr, "integral.i.load");
        cg.builder.CreateCondBr(cg.builder.CreateICmpSLT(i, steps, "integral.keep"), bodyBB, endBB);

        cg.builder.SetInsertPoint(bodyBB);
        llvm::Value* iAsDouble = cg.builder.CreateSIToFP(i, f64Ty, "integral.i.fp");
        llvm::Value* midpoint = cg.builder.CreateFAdd(
            a,
            cg.builder.CreateFMul(cg.builder.CreateFAdd(iAsDouble, half, "integral.mid.index"), width, "integral.offset"),
            "integral.midpoint");
        llvm::Value* sample = cg.builder.CreateCall(fType, integrand, {midpoint}, "integral.sample");
        llvm::Value* sum = cg.builder.CreateLoad(f64Ty, sumPtr, "integral.sum.load");
        cg.builder.CreateStore(cg.builder.CreateFAdd(sum, sample, "integral.next.sum"), sumPtr);
        cg.builder.CreateStore(cg.builder.CreateAdd(i, one, "integral.next.i"), iPtr);
        cg.builder.CreateBr(condBB);

        cg.builder.SetInsertPoint(endBB);
        return cg.builder.CreateFMul(cg.builder.CreateLoad(f64Ty, sumPtr, "integral.sum.result"), width, "integral.result");
    }

    if (functionName != "odeEuler") {
        return nullptr;
    }
    if (argValues.size() != 5) {
        std::cerr << "Error: odeEuler(f, t0, y0, h, steps) requires five arguments" << std::endl;
        return nullptr;
    }

    auto* function = cg.builder.GetInsertBlock()->getParent();
    // The differential function is passed as a first-class value, i.e. a { code, env } closure. It is
    // invoked below via the closure convention (code(env, t, y)); calling the closure pointer as a
    // bare `double(double,double)` would execute the struct data as code and crash.
    auto* opaquePtr = llvm::PointerType::getUnqual(cg.context);
    auto* closureStructType = llvm::StructType::get(cg.context, { opaquePtr, opaquePtr });
    auto* closureFnType = llvm::FunctionType::get(f64Ty, { opaquePtr, f64Ty, f64Ty }, false);
    llvm::Value* derivative = argValues[0];
    llvm::Value* t0 = coerceMathValueToDouble(cg, argValues[1]);
    llvm::Value* y0 = coerceMathValueToDouble(cg, argValues[2]);
    llvm::Value* h = coerceMathValueToDouble(cg, argValues[3]);
    llvm::Value* steps64 = coerceMathValueToI64(cg, argValues[4]);
    llvm::Value* steps = cg.builder.CreateTrunc(steps64, i32Ty, "ode.steps");

    llvm::Value* tPtr = cg.builder.CreateAlloca(f64Ty, nullptr, "ode.t");
    llvm::Value* yPtr = cg.builder.CreateAlloca(f64Ty, nullptr, "ode.y");
    llvm::Value* iPtr = cg.builder.CreateAlloca(i32Ty, nullptr, "ode.i");
    cg.builder.CreateStore(t0, tPtr);
    cg.builder.CreateStore(y0, yPtr);
    cg.builder.CreateStore(llvm::ConstantInt::get(i32Ty, 0), iPtr);

    auto* condBB = llvm::BasicBlock::Create(cg.context, "ode.euler.cond", function);
    auto* bodyBB = llvm::BasicBlock::Create(cg.context, "ode.euler.body", function);
    auto* endBB = llvm::BasicBlock::Create(cg.context, "ode.euler.end", function);

    cg.builder.CreateBr(condBB);
    cg.builder.SetInsertPoint(condBB);
    llvm::Value* i = cg.builder.CreateLoad(i32Ty, iPtr, "ode.i.load");
    cg.builder.CreateCondBr(cg.builder.CreateICmpSLT(i, steps, "ode.keep"), bodyBB, endBB);

    cg.builder.SetInsertPoint(bodyBB);
    llvm::Value* t = cg.builder.CreateLoad(f64Ty, tPtr, "ode.t.load");
    llvm::Value* y = cg.builder.CreateLoad(f64Ty, yPtr, "ode.y.load");
    llvm::Value* fCode = cg.builder.CreateLoad(opaquePtr,
        cg.builder.CreateStructGEP(closureStructType, derivative, 0, "ode.f.code"), "ode.f.code.fn");
    llvm::Value* fEnv = cg.builder.CreateLoad(opaquePtr,
        cg.builder.CreateStructGEP(closureStructType, derivative, 1, "ode.f.env"), "ode.f.env.ptr");
    llvm::Value* dy = cg.builder.CreateCall(closureFnType, fCode, {fEnv, t, y}, "ode.dy");
    llvm::Value* nextY = cg.builder.CreateFAdd(y, cg.builder.CreateFMul(h, dy, "ode.delta"), "ode.next.y");
    llvm::Value* nextT = cg.builder.CreateFAdd(t, h, "ode.next.t");
    cg.builder.CreateStore(nextY, yPtr);
    cg.builder.CreateStore(nextT, tPtr);
    cg.builder.CreateStore(cg.builder.CreateAdd(i, llvm::ConstantInt::get(i32Ty, 1), "ode.next.i"), iPtr);
    cg.builder.CreateBr(condBB);

    cg.builder.SetInsertPoint(endBB);
    return cg.builder.CreateLoad(f64Ty, yPtr, "ode.result");
}

void FunctionCallNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// Helper: substitute TypeVariableType in a FunctionDeclarationNode clone
static void substituteTypeReference(std::unique_ptr<Type>& t,
                                    const std::vector<std::string>& typeParams,
                                    const std::vector<std::unique_ptr<Type>>& concreteTypes) {
    if (!t) {
        return;
    }

    if (t->getKind() == Type::Kind::VARIABLE) {
        for (size_t i = 0; i < typeParams.size() && i < concreteTypes.size(); ++i) {
            if (t->getName() == typeParams[i] && concreteTypes[i]) {
                t = concreteTypes[i]->clone();
                return;
            }
        }
    }

    if (t->getKind() == Type::Kind::GENERIC) {
        auto* generic = dynamic_cast<GenericType*>(t.get());
        if (!generic) {
            return;
        }
        substituteTypeReference(generic->baseType, typeParams, concreteTypes);
        for (auto& arg : generic->typeArguments) {
            substituteTypeReference(arg, typeParams, concreteTypes);
        }
    }
}

static void substituteTypesInAst(ASTNode* node,
                                 const std::vector<std::string>& typeParams,
                                 const std::vector<std::unique_ptr<Type>>& concreteTypes) {
    if (!node) {
        return;
    }

    if (auto* block = dynamic_cast<BlockNode*>(node)) {
        for (auto& stmt : block->statements) {
            substituteTypesInAst(stmt.get(), typeParams, concreteTypes);
        }
        return;
    }

    if (auto* returnStmt = dynamic_cast<ReturnStatementNode*>(node)) {
        substituteTypesInAst(returnStmt->expression.get(), typeParams, concreteTypes);
        return;
    }

    if (auto* binary = dynamic_cast<BinaryExpressionNode*>(node)) {
        substituteTypesInAst(binary->left.get(), typeParams, concreteTypes);
        substituteTypesInAst(binary->right.get(), typeParams, concreteTypes);
        return;
    }

    if (auto* ifStmt = dynamic_cast<IfStatementNode*>(node)) {
        substituteTypesInAst(ifStmt->condition.get(), typeParams, concreteTypes);
        substituteTypesInAst(ifStmt->thenBlock.get(), typeParams, concreteTypes);
        substituteTypesInAst(ifStmt->elseBlock.get(), typeParams, concreteTypes);
        return;
    }

    if (auto* varDecl = dynamic_cast<VariableDeclarationNode*>(node)) {
        substituteTypeReference(varDecl->type, typeParams, concreteTypes);
        substituteTypesInAst(varDecl->initializer.get(), typeParams, concreteTypes);
        return;
    }

    if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
        for (auto& typeArg : call->explicitTypeArgs) {
            substituteTypeReference(typeArg, typeParams, concreteTypes);
        }
        for (auto& arg : call->arguments) {
            substituteTypesInAst(arg.get(), typeParams, concreteTypes);
        }
        return;
    }

    if (auto* creation = dynamic_cast<ClassInstanceCreationNode*>(node)) {
        for (auto& typeArg : creation->templateArgs) {
            substituteTypeReference(typeArg, typeParams, concreteTypes);
        }
        for (auto& arg : creation->arguments) {
            substituteTypesInAst(arg.get(), typeParams, concreteTypes);
        }
        return;
    }
}

static void substituteTypes(FunctionDeclarationNode* funcDecl,
                           const std::vector<std::string>& typeParams,
                           const std::vector<std::unique_ptr<Type>>& concreteTypes) {
    // Substitute parameter types
    for (auto& param : funcDecl->parameters) {
        auto* paramNode = dynamic_cast<ParameterNode*>(param.get());
        if (paramNode) {
            substituteTypeReference(paramNode->type, typeParams, concreteTypes);
        }
    }

    // Substitute return type
    substituteTypeReference(funcDecl->returnType, typeParams, concreteTypes);
    substituteTypesInAst(funcDecl->body.get(), typeParams, concreteTypes);
}

static void substituteTemplateConstants(ASTNode* node,
                                        const std::unordered_map<std::string, std::string>& constantValues) {
    if (!node) {
        return;
    }

    if (auto* identifier = dynamic_cast<IdentifierNode*>(node)) {
        auto it = constantValues.find(identifier->value);
        if (it != constantValues.end()) {
            identifier->value = it->second;
        }
        return;
    }

    if (auto* block = dynamic_cast<BlockNode*>(node)) {
        for (auto& stmt : block->statements) {
            substituteTemplateConstants(stmt.get(), constantValues);
        }
        return;
    }

    if (auto* returnStmt = dynamic_cast<ReturnStatementNode*>(node)) {
        substituteTemplateConstants(returnStmt->expression.get(), constantValues);
        return;
    }

    if (auto* binary = dynamic_cast<BinaryExpressionNode*>(node)) {
        substituteTemplateConstants(binary->left.get(), constantValues);
        substituteTemplateConstants(binary->right.get(), constantValues);
        return;
    }

    if (auto* ifStmt = dynamic_cast<IfStatementNode*>(node)) {
        substituteTemplateConstants(ifStmt->condition.get(), constantValues);
        substituteTemplateConstants(ifStmt->thenBlock.get(), constantValues);
        substituteTemplateConstants(ifStmt->elseBlock.get(), constantValues);
        return;
    }

    if (auto* varDecl = dynamic_cast<VariableDeclarationNode*>(node)) {
        substituteTemplateConstants(varDecl->initializer.get(), constantValues);
        return;
    }

    if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
        for (auto& arg : call->arguments) {
            substituteTemplateConstants(arg.get(), constantValues);
        }
        return;
    }
}

// Helper: infer concrete type from llvm::Value
static std::unique_ptr<Type> inferTypeFromLLVM(llvm::Value* val) {
    if (!val) return std::make_unique<UnknownType>();
    llvm::Type* t = val->getType();
    if (t->isIntegerTy(32)) return std::make_unique<BasicType>("Int");
    if (t->isIntegerTy(64)) return std::make_unique<BasicType>("Long");
    if (t->isIntegerTy(16)) return std::make_unique<BasicType>("Short");
    if (t->isIntegerTy(8)) return std::make_unique<BasicType>("Byte");
    if (t->isIntegerTy(1)) return std::make_unique<BasicType>("Boolean");
    if (t->isFloatTy()) return std::make_unique<BasicType>("Float");
    if (t->isDoubleTy()) return std::make_unique<BasicType>("Double");
    if (t->isPointerTy()) return std::make_unique<BasicType>("String");
    return std::make_unique<UnknownType>();
}

static llvm::Value* coerceArgumentToParameterType(llvm::Value* value, llvm::Type* targetType) {
    // An integer argument passed where a pointer parameter is expected means a Nat parameter
    // (`gcd(12, 18)` with `def gcd(a: Nat, b: Nat)`): promote it to an arbitrary-precision handle.
    // Any other integer->pointer argument would be a type error regardless, so this is safe.
    if (value && value->getType()->isIntegerTy() && targetType && targetType->isPointerTy()) {
        auto& cg = CodeGenerator::getInstance();
        auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
        auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
        llvm::Value* wide = value->getType()->isIntegerTy(64)
            ? value
            : cg.builder.CreateSExt(value, i64Ty, "nat.arg.sext");
        llvm::Function* fn = cg.module->getFunction("csec_bigint_from_i64");
        if (!fn) {
            auto* fnTy = llvm::FunctionType::get(i8PtrTy, {i64Ty}, false);
            fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "csec_bigint_from_i64", cg.module.get());
        }
        return cg.builder.CreateCall(fn, {wide}, "nat.arg");
    }
    return coerceValueToLLVMType(value, targetType);
}

llvm::Value* codegenNativeProcessCall(
    const std::string& functionName,
    const std::vector<llvm::Value*>& argValues) {
    auto& cg = CodeGenerator::getInstance();
    auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());

    if (functionName == "commandLineArgCount") {
        if (!argValues.empty()) {
            std::cerr << "Error: commandLineArgCount() does not take arguments" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_command_line_arg_count", i32Ty, {}),
            {},
            "argv.count");
    }

    if (functionName == "commandLineArg") {
        if (argValues.size() != 1) {
            std::cerr << "Error: commandLineArg(index) requires one argument" << std::endl;
            return nullptr;
        }
        return cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_command_line_arg", i8PtrTy, {i32Ty}),
            {cg.builder.CreateTrunc(coerceMathValueToI64(cg, argValues[0]), i32Ty, "argv.index")},
            "argv.value");
    }

    return nullptr;
}

static llvm::Value* defaultValueForReturnType(llvm::Type* returnType) {
    auto& cg = CodeGenerator::getInstance();
    if (!returnType || returnType->isVoidTy()) {
        return nullptr;
    }
    if (returnType->isIntegerTy()) {
        return llvm::ConstantInt::get(returnType, 0, true);
    }
    if (returnType->isFloatingPointTy()) {
        return llvm::ConstantFP::get(returnType, 0.0);
    }
    if (returnType->isPointerTy()) {
        return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(returnType));
    }
    return llvm::Constant::getNullValue(returnType);
}

static llvm::Value* createCallWithAdjustedArgs(llvm::Function* function, const std::vector<llvm::Value*>& argValues) {
    auto& cg = CodeGenerator::getInstance();
    if (!function || argValues.size() != function->arg_size()) {
        return function ? defaultValueForReturnType(function->getReturnType()) : nullptr;
    }

    std::vector<llvm::Value*> adjustedArgs;
    adjustedArgs.reserve(argValues.size());
    for (size_t i = 0; i < argValues.size(); ++i) {
        llvm::Type* paramType = function->getFunctionType()->getParamType(static_cast<unsigned>(i));
        llvm::Value* adjusted = coerceArgumentToParameterType(argValues[i], paramType);
        if (!adjusted || adjusted->getType() != paramType) {
            return defaultValueForReturnType(function->getReturnType());
        }
        adjustedArgs.push_back(adjusted);
    }

    return cg.builder.CreateCall(function, adjustedArgs, function->getReturnType()->isVoidTy() ? "" : "calltmp");
}

static llvm::Value* createTypedCallWithAdjustedArgs(
    llvm::FunctionType* functionType,
    llvm::Value* callee,
    const std::vector<llvm::Value*>& argValues) {
    auto& cg = CodeGenerator::getInstance();
    if (!functionType || !callee || argValues.size() != functionType->getNumParams()) {
        return functionType ? defaultValueForReturnType(functionType->getReturnType()) : nullptr;
    }

    std::vector<llvm::Value*> adjustedArgs;
    adjustedArgs.reserve(argValues.size());
    for (size_t i = 0; i < argValues.size(); ++i) {
        llvm::Type* paramType = functionType->getParamType(static_cast<unsigned>(i));
        llvm::Value* adjusted = coerceArgumentToParameterType(argValues[i], paramType);
        if (!adjusted || adjusted->getType() != paramType) {
            return defaultValueForReturnType(functionType->getReturnType());
        }
        adjustedArgs.push_back(adjusted);
    }

    return cg.builder.CreateCall(
        functionType,
        callee,
        adjustedArgs,
        functionType->getReturnType()->isVoidTy() ? "" : "calltmp");
}

llvm::Value* FunctionCallNode::codegen() {
    auto& cg = CodeGenerator::getInstance();

    // Evaluate arguments first
    std::vector<llvm::Value*> argValues;
    std::vector<std::unique_ptr<Type>> argTypes;
    for (auto& arg : arguments) {
        llvm::Value* argValue = arg->codegen();
        if (!argValue) {
            auto argType = arg->getType();
            if (argType && argType->isVoidTy()) {
                argValue = llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), 0);
            }
            else if (argType && (argType->isFloatTy() || argType->isDoubleTy())) {
                argValue = llvm::ConstantFP::get(llvm::Type::getDoubleTy(cg.context), 0.0);
            }
            else {
                argValue = llvm::ConstantFP::get(llvm::Type::getDoubleTy(cg.context), 0.0);
            }
        }
        argValues.push_back(argValue);
        auto argType = arg->getType();
        argTypes.push_back(argType ? argType->clone() : std::make_unique<UnknownType>());
    }

    if (functionName == "__token_matches") {
        if (argValues.size() != 4) {
            std::cerr << "Error: internal token matcher requires four arguments" << std::endl;
            return nullptr;
        }
        auto* i8Ty = llvm::Type::getInt8Ty(cg.context);
        auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
        auto* i8PtrTy = llvm::PointerType::getUnqual(i8Ty->getContext());
        llvm::Value* ordinal = argValues[1];
        if (!ordinal->getType()->isIntegerTy(32)) ordinal = cg.builder.CreateIntCast(ordinal, i32Ty, true, "token.pattern.ordinal");
        llvm::Value* kind = argValues[2];
        if (!kind->getType()->isIntegerTy(8)) kind = cg.builder.CreateTrunc(kind, i8Ty, "token.pattern.kind");
        llvm::Value* raw = cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_token_is", i32Ty, {i8PtrTy, i32Ty, i8Ty, i8PtrTy}),
            {asCStringPointer(cg, argValues[0]), ordinal, kind, asCStringPointer(cg, argValues[3])},
            "token.pattern.raw");
        return cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Ty, 0), "token.pattern");
    }

    // free(x): release a heap allocation made by `new`, an array literal, a class instance, or a
    // box. The type checker limits this consuming operation to a named owned value.
    if (functionName == "free") {
        if (!argValues.empty() && argValues[0] && argValues[0]->getType()->isPointerTy()) {
            // Prevent the lexical owner cleanup from freeing the same allocation a second time.
            cg.forgetCleanup(argValues[0]);
            if (!argTypes.empty() && argTypes[0] && TensorRuntime::isTensorTypeName(argTypes[0]->getName())) {
                auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
                llvm::Value* dims = TensorRuntime::loadDims(cg, argValues[0]);
                llvm::Value* data = TensorRuntime::loadData(cg, argValues[0]);
                cg.builder.CreateCall(cg.freeFunction, {cg.builder.CreateBitCast(data, i8PtrTy, "tensor.data.free")});
                cg.builder.CreateCall(cg.freeFunction, {cg.builder.CreateBitCast(dims, i8PtrTy, "tensor.dims.free")});
            }
            llvm::Value* raw = cg.builder.CreateBitCast(
                argValues[0], llvm::PointerType::getUnqual(cg.context), "free.ptr");
            cg.builder.CreateCall(cg.freeFunction, { raw });
        }
        else {
            std::cerr << "Error: free() expects a heap pointer argument" << std::endl;
        }
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), 0);
    }

    // clone(x): produce a fresh, independently-owned box holding a copy of x's payload. The source
    // is read (not moved), so `b = clone(a)` leaves `a` usable — this is the explicit-clone escape
    // hatch from the ownership model. MVP semantics are a shallow copy of the boxed payload; a deep
    // clone of nested heap references is a future refinement.
    if (functionName == "clone") {
        BoxType* boxType = (!argTypes.empty() && argTypes[0])
            ? dynamic_cast<BoxType*>(argTypes[0].get()) : nullptr;
        llvm::Type* payloadType = (boxType && boxType->baseType)
            ? cg.getLLVMType(boxType->baseType.get()) : nullptr;
        if (!payloadType || argValues.empty() || !argValues[0] || !argValues[0]->getType()->isPointerTy()) {
            std::cerr << "Error: clone() expects an owned 'box T' value" << std::endl;
            return argValues.empty() ? nullptr : argValues[0];
        }
        const llvm::DataLayout& dl = cg.module->getDataLayout();
        llvm::Value* allocSize = llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(cg.context), dl.getTypeAllocSize(payloadType));
        llvm::Value* rawPtr = cg.builder.CreateCall(cg.mallocFunction, allocSize, "clone.malloc");
        llvm::Value* newBox = cg.builder.CreateBitCast(
            rawPtr, llvm::PointerType::getUnqual(payloadType->getContext()), "clone.box");
        // Copy the payload; the destination binding registers the cleanup, exactly as `new` does.
        cg.builder.CreateMemCpy(newBox, llvm::MaybeAlign(), argValues[0], llvm::MaybeAlign(), allocSize);
        return newBox;
    }

    // Shared(x): explicit shared ownership. Allocate a control block { i64 strong, payload }, set
    // strong = 1, store the payload, and return the block pointer (the Shared<T> value). Duplicate
    // it with `.clone()` (retain); it is released at each owner's scope exit and freed at zero.
    if (functionName == "Shared") {
        if (argValues.empty() || !argValues[0]) {
            std::cerr << "Error: Shared() expects one value" << std::endl;
            return nullptr;
        }
        llvm::Value* payload = argValues[0];
        auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
        // Control block { strong, weak, payload }. strong keeps the payload alive; weak keeps only
        // the block alive (for Weak<T> handles), so a cycle with a Weak edge is still reclaimed.
        llvm::StructType* blockType = llvm::StructType::get(cg.context, { i64Ty, i64Ty, payload->getType() });
        const llvm::DataLayout& dl = cg.module->getDataLayout();
        llvm::Value* size = llvm::ConstantInt::get(i64Ty, dl.getTypeAllocSize(blockType));
        llvm::Value* raw = cg.builder.CreateCall(cg.mallocFunction, size, "shared.malloc");
        llvm::Value* block = cg.builder.CreateBitCast(raw, llvm::PointerType::getUnqual(blockType->getContext()), "shared.block");
        cg.builder.CreateStore(llvm::ConstantInt::get(i64Ty, 1),
            cg.builder.CreateStructGEP(blockType, block, 0, "shared.strong.ptr"));
        cg.builder.CreateStore(llvm::ConstantInt::get(i64Ty, 0),
            cg.builder.CreateStructGEP(blockType, block, 1, "shared.weak.ptr"));
        cg.builder.CreateStore(payload,
            cg.builder.CreateStructGEP(blockType, block, 2, "shared.payload.ptr"));
        return block;
    }

    // Weak(s): a non-owning handle into a Shared<T> control block. Bumps the weak count (which does
    // not keep the payload alive) and returns the same block pointer as a Weak<T> value.
    if (functionName == "Weak") {
        if (argValues.empty() || !argValues[0] || !argValues[0]->getType()->isPointerTy()) {
            std::cerr << "Error: Weak() expects a Shared value" << std::endl;
            return argValues.empty() ? nullptr : argValues[0];
        }
        auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
        llvm::Value* block = argValues[0];
        llvm::Value* weakPtr = cg.builder.CreateGEP(i64Ty, block, llvm::ConstantInt::get(i64Ty, 1), "weak.count.ptr");
        llvm::Value* weak = cg.builder.CreateLoad(i64Ty, weakPtr, "weak.count");
        cg.builder.CreateStore(cg.builder.CreateAdd(weak, llvm::ConstantInt::get(i64Ty, 1), "weak.inc"), weakPtr);
        return block;
    }

    if (functionName == "assert" && !argValues.empty()) {
        // assert(condition[, message]): checked statically by --verify; at run time a false condition
        // prints "AssertionError: file:line: condition [message]" and exits with status 1.
        auto* i8PtrTy = llvm::PointerType::getUnqual(cg.context);
        llvm::Value* condition = argValues[0];
        if (!condition->getType()->isIntegerTy(1)) {
            condition = condition->getType()->isIntegerTy()
                ? cg.builder.CreateICmpNE(condition, llvm::ConstantInt::get(condition->getType(), 0), "assert.cond")
                : nullptr;
        }
        if (!condition) {
            std::cerr << "Type error: assert() needs a Bool condition" << std::endl;
            return nullptr;
        }
        const SourceLocation location = sourceLocationOf(line);
        std::string where = location.file.empty() ? "<input>" : location.file;
        const size_t slash = where.find_last_of("/\\");
        if (slash != std::string::npos) where = where.substr(slash + 1);
        where += ":" + std::to_string(location.line);
        std::string text = verifierExpressionText(arguments[0].get());
        llvm::Value* message = cg.builder.CreateGlobalString(text, "assert.text");
        if (arguments.size() >= 2 && argValues[1]->getType()->isPointerTy()) {
            message = cg.builder.CreateCall(
                getOrCreateRuntimeFunction(cg, "csec_string_concat", i8PtrTy, {i8PtrTy, i8PtrTy}),
                {cg.builder.CreateGlobalString(text + ": ", "assert.prefix"), argValues[1]}, "assert.message");
        }
        llvm::Function* function = cg.builder.GetInsertBlock()->getParent();
        auto* failBB = llvm::BasicBlock::Create(cg.context, "assert.fail", function);
        auto* okBB = llvm::BasicBlock::Create(cg.context, "assert.ok", function);
        cg.builder.CreateCondBr(condition, okBB, failBB);
        cg.builder.SetInsertPoint(failBB);
        cg.builder.CreateCall(
            getOrCreateRuntimeFunction(cg, "csec_assert_fail", llvm::Type::getVoidTy(cg.context), {i8PtrTy, i8PtrTy}),
            {cg.builder.CreateGlobalString(where, "assert.where"), message});
        cg.builder.CreateBr(okBB);
        cg.builder.SetInsertPoint(okBB);
        return nullptr;
    }
    if (isNativeIOFunction(functionName)) {
        return codegenNativeIOCall(functionName, argValues, argTypes);
    }
    if (isNativeProcessFunction(functionName)) {
        return codegenNativeProcessCall(functionName, argValues);
    }
    if (isNativeNetworkFunction(functionName)) {
        return codegenNativeNetworkCall(functionName, argValues, argTypes);
    }
    if (isNativePosixFunction(functionName)) {
        return codegenNativePosixCall(functionName, argValues);
    }
    if (isNativeSystemFileFunction(functionName)) {
        return codegenNativeSystemFileCall(functionName, argValues);
    }
    if (resolvesToMicrothreadBuiltin(functionName)) {
        return codegenNativeMicrothreadCall(functionName, arguments, argValues);
    }
    if (isNativeParallelFunction(functionName)) {
        return codegenNativeParallelCall(functionName, argValues);
    }
    if (isNativeDynamicLibraryFunction(functionName)) {
        return codegenNativeDynamicLibraryCall(functionName, argValues, argTypes);
    }
    if (isNativeSimulationFunction(functionName)) {
        return codegenNativeSimulationCall(functionName, argValues);
    }
    if (isBuiltinOdeFunction(functionName)) {
        return codegenBuiltinOdeCall(functionName, argValues);
    }

    // Check if the function name refers to a template symbol
    auto* symbol = cg.symbolTable.lookup(functionName);
    if (symbol && symbol->symbolType == SymbolType::TEMPLATE) {
        auto* tmplSymbol = dynamic_cast<TemplateSymbol*>(symbol);
        if (!tmplSymbol) {
            std::cerr << "Error: Invalid template symbol '" << functionName << "'" << std::endl;
            return nullptr;
        }

        auto* funcDecl = dynamic_cast<FunctionDeclarationNode*>(tmplSymbol->declaration.get());
        if (!funcDecl) {
            std::cerr << "Error: Template '" << functionName << "' is not a function template" << std::endl;
            return nullptr;
        }

        const bool hasFullTemplateParams = !tmplSymbol->templateParams.empty();
        const size_t templateParamCount = hasFullTemplateParams
            ? tmplSymbol->templateParams.size()
            : tmplSymbol->typeParameters.size();

        // Use explicit template arguments if provided, otherwise infer
        std::vector<std::unique_ptr<Type>> concreteTypes(templateParamCount);
        if (!explicitTypeArgs.empty()) {
            // Explicit template arguments: identity<Int>(42)
            for (size_t i = 0; i < explicitTypeArgs.size() && i < concreteTypes.size(); ++i) {
                concreteTypes[i] = explicitTypeArgs[i]->clone();
            }
        } else {
            // Infer type arguments from call argument types
            for (size_t pi = 0; pi < funcDecl->parameters.size() && pi < argValues.size(); ++pi) {
                auto* paramNode = dynamic_cast<ParameterNode*>(funcDecl->parameters[pi].get());
                if (!paramNode || !paramNode->type) continue;
                if (paramNode->type->getKind() == Type::Kind::VARIABLE) {
                    for (size_t ti = 0; ti < tmplSymbol->typeParameters.size(); ++ti) {
                        if (paramNode->type->getName() == tmplSymbol->typeParameters[ti] && !concreteTypes[ti]) {
                            // Prefer AST-level type, fall back to LLVM inference
                            if (argTypes[pi] && argTypes[pi]->getKind() != Type::Kind::UNKNOWN) {
                                concreteTypes[ti] = argTypes[pi]->clone();
                            } else {
                                concreteTypes[ti] = inferTypeFromLLVM(argValues[pi]);
                            }
                        }
                    }
                }
            }
        }

        // Build mangled name
        std::string mangledName = functionName;
        for (auto& ct : concreteTypes) {
            mangledName += "$" + (ct ? ct->getName() : "Unknown");
        }

        // Check instantiation cache
        auto cacheIt = tmplSymbol->instantiations.find(mangledName);
        if (cacheIt != tmplSymbol->instantiations.end()) {
            llvm::Function* function = cacheIt->second;
            std::vector<llvm::Value*> adjustedArgs;
            adjustedArgs.reserve(argValues.size());
            for (size_t i = 0; i < argValues.size(); ++i) {
                llvm::Type* paramType = i < function->getFunctionType()->getNumParams()
                    ? function->getFunctionType()->getParamType(static_cast<unsigned>(i))
                    : nullptr;
                adjustedArgs.push_back(coerceArgumentToParameterType(argValues[i], paramType));
            }
            llvm::Value* result = createCallWithAdjustedArgs(function, adjustedArgs);

            // Set type from the function return
            auto* funcType = dynamic_cast<FunctionType*>(
                cg.symbolTable.lookupFunction(mangledName, concreteTypes) ?
                cg.symbolTable.lookupFunction(mangledName, concreteTypes)->type.get() : nullptr);
            if (funcType && funcType->returnType) {
                type = funcType->returnType->clone();
            }
            return result;
        }

        // Clone the template AST and substitute types
        auto clonedDecl = funcDecl->clone();
        auto* clonedFunc = dynamic_cast<FunctionDeclarationNode*>(clonedDecl.get());
        if (!clonedFunc) {
            std::cerr << "Error: Failed to clone template function" << std::endl;
            return nullptr;
        }

        clonedFunc->name = mangledName;
        substituteTypes(clonedFunc, tmplSymbol->typeParameters, concreteTypes);
        if (hasFullTemplateParams) {
            std::unordered_map<std::string, std::string> constantValues;
            for (size_t i = 0; i < tmplSymbol->templateParams.size() && i < concreteTypes.size(); ++i) {
                if (!tmplSymbol->templateParams[i].isType && concreteTypes[i]) {
                    constantValues[tmplSymbol->templateParams[i].name] = concreteTypes[i]->getName();
                }
            }
            substituteTemplateConstants(clonedFunc->body.get(), constantValues);
        }

        // Save codegen state
        auto* savedBlock = cg.builder.GetInsertBlock();
        auto savedPoint = cg.builder.GetInsertPoint();
        auto* savedCurrentSymbol = cg.symbolTable.getCurrentSymbol();
        cg.symbolTable.saveCurrentSymbol();

        // Codegen the instantiated function (at top level)
        cg.symbolTable.setCurrentSymbol(nullptr);
        llvm::Value* funcVal = clonedFunc->codegen();
        if (!funcVal) {
            std::cerr << "Error: Failed to instantiate template function '" << mangledName << "'" << std::endl;
            cg.symbolTable.popCurrentSymbol();
            cg.symbolTable.setCurrentSymbol(savedCurrentSymbol);
            cg.builder.SetInsertPoint(savedBlock, savedPoint);
            return nullptr;
        }

        // Restore codegen state
        cg.symbolTable.popCurrentSymbol();
        cg.symbolTable.setCurrentSymbol(savedCurrentSymbol);
        cg.builder.SetInsertPoint(savedBlock, savedPoint);

        llvm::Function* function = llvm::dyn_cast<llvm::Function>(funcVal);
        if (!function) {
            std::cerr << "Error: Template instantiation did not produce a function" << std::endl;
            return nullptr;
        }

        // Cache the instantiation
        tmplSymbol->instantiations[mangledName] = function;

        // Call the instantiated function
        std::vector<llvm::Value*> adjustedArgs;
        adjustedArgs.reserve(argValues.size());
        for (size_t i = 0; i < argValues.size(); ++i) {
            llvm::Type* paramType = i < function->getFunctionType()->getNumParams()
                ? function->getFunctionType()->getParamType(static_cast<unsigned>(i))
                : nullptr;
            adjustedArgs.push_back(coerceArgumentToParameterType(argValues[i], paramType));
        }
        llvm::Value* result = createCallWithAdjustedArgs(function, adjustedArgs);

        // Infer return type
        if (function->getReturnType()->isIntegerTy(32)) {
            type = std::make_unique<BasicType>("Int");
        } else if (function->getReturnType()->isDoubleTy()) {
            type = std::make_unique<BasicType>("Double");
        } else if (function->getReturnType()->isFloatTy()) {
            type = std::make_unique<BasicType>("Float");
        } else if (function->getReturnType()->isVoidTy()) {
            type = std::make_unique<BasicType>("Unit");
        } else {
            type = std::make_unique<UnknownType>();
        }

        return result;
    }

    // Normal function lookup
    auto functionSymbolOpt = cg.symbolTable.lookupFunction(functionName, argTypes);
    if (functionSymbolOpt) {
        auto* functionSymbol = functionSymbolOpt;
        auto* funcType = dynamic_cast<FunctionType*>(functionSymbol->type.get());
        llvm::Function* function = static_cast<llvm::Function*>(functionSymbol->value);
        if (!funcType || !function) {
            std::cerr << "Error: Invalid function '" << functionName << "'" << std::endl;
            return nullptr;
        }

        // Constexpr function with constant args: try compile-time evaluation
        if (function->hasFnAttribute(llvm::Attribute::AlwaysInline)) {
            bool allConstant = true;
            for (auto& av : argValues) {
                if (!llvm::isa<llvm::Constant>(av)) {
                    allConstant = false;
                    break;
                }
            }
            if (allConstant && !function->getReturnType()->isVoidTy()) {
                // Generate the call with AlwaysInline - LLVM optimizer will constant-fold
                llvm::Value* result = createCallWithAdjustedArgs(function, argValues);
                type = funcType->returnType ? funcType->returnType->clone() : std::make_unique<UnknownType>();
                return result;
            }
        }

        llvm::Value* result = createCallWithAdjustedArgs(function, argValues);

        type = funcType->returnType ? funcType->returnType->clone() : std::make_unique<UnknownType>();
        return result;
    }
    else if (auto* callableSymbol = cg.symbolTable.lookup(functionName)) {
        auto* funcType = dynamic_cast<FunctionType*>(callableSymbol->type.get());
        if (funcType && callableSymbol->value) {
            std::vector<llvm::Type*> paramLLVMTypes;
            for (const auto& parameterType : funcType->parameterTypes) {
                auto* llvmParamType = getABIStorageType(parameterType.get());
                if (!llvmParamType) {
                    std::cerr << "Error: Invalid parameter type in callable symbol '" << functionName << "'" << std::endl;
                    return nullptr;
                }
                paramLLVMTypes.push_back(llvmParamType);
            }
            auto* llvmReturnType = getABIStorageType(funcType->returnType.get());
            if (!llvmReturnType) {
                std::cerr << "Error: Invalid callable symbol '" << functionName << "'" << std::endl;
                return nullptr;
            }

            // A capturing lambda is a { code, env } closure pointer rather than a bare function.
            // Load its code and environment and call code(env, args).
            if (!llvm::isa<llvm::Function>(callableSymbol->value)) {
                auto* opaquePtr = llvm::PointerType::getUnqual(cg.context);
                auto* closureType = llvm::StructType::get(cg.context, { opaquePtr, opaquePtr });
                llvm::Value* closure = callableSymbol->value;
                llvm::Value* code = cg.builder.CreateLoad(opaquePtr,
                    cg.builder.CreateStructGEP(closureType, closure, 0, "closure.code"), "closure.code.fn");
                llvm::Value* environment = cg.builder.CreateLoad(opaquePtr,
                    cg.builder.CreateStructGEP(closureType, closure, 1, "closure.env"), "closure.env.ptr");
                std::vector<llvm::Type*> closureParamTypes;
                closureParamTypes.push_back(opaquePtr);
                for (auto* paramType : paramLLVMTypes) closureParamTypes.push_back(paramType);
                auto* closureFuncType = llvm::FunctionType::get(llvmReturnType, closureParamTypes, false);
                std::vector<llvm::Value*> closureArgs;
                closureArgs.push_back(environment);
                for (size_t i = 0; i < argValues.size(); ++i) {
                    // Coerce each argument to the closure's declared parameter type (the params begin
                    // after the env slot). An argument's value can have a different-but-compatible
                    // scalar type — e.g. a Float literal passed where the parameter is Double — and
                    // the raw value would otherwise trip LLVM's call-signature check. The direct-call
                    // path adjusts args the same way.
                    llvm::Value* argValue = argValues[i];
                    if (i < paramLLVMTypes.size()) {
                        argValue = coerceValueToLLVMType(argValue, paramLLVMTypes[i]);
                    }
                    closureArgs.push_back(argValue);
                }
                llvm::Value* result = cg.builder.CreateCall(closureFuncType, code, closureArgs,
                    llvmReturnType->isVoidTy() ? "" : "closure.call");
                type = funcType->returnType ? funcType->returnType->clone() : std::make_unique<UnknownType>();
                return result;
            }

            auto* llvmFuncType = llvm::FunctionType::get(llvmReturnType, paramLLVMTypes, false);
            llvm::Value* result = createTypedCallWithAdjustedArgs(llvmFuncType, callableSymbol->value, argValues);
            type = funcType->returnType ? funcType->returnType->clone() : std::make_unique<UnknownType>();
            return result;
        }
    }
    else {
        llvm::Function* function = cg.module->getFunction(functionName);
        if (function) {
            llvm::Value* result = createCallWithAdjustedArgs(function, argValues);
            return result;
        }
        else {
            if (isMathFallbackFunction(functionName)) {
                if (functionName == "transpose" || functionName == "conjugate" || functionName == "adjoint") {
                    if (argValues.size() != 1 || argTypes.empty() || !TensorRuntime::isTensorTypeName(argTypes[0]->getName())) {
                        std::cerr << "Error: Tensor fallback '" << functionName << "' requires one tensor argument" << std::endl;
                        return nullptr;
                    }
                    if (functionName == "transpose" || functionName == "adjoint") {
                        return TensorRuntime::transposeTensor(cg, argValues[0]);
                    }
                    return TensorRuntime::cloneTensor(cg, argValues[0]);
                }
                if (functionName == "relu" || functionName == "softmax") {
                    if (argValues.size() != 1 || argTypes.empty() || !TensorRuntime::isTensorTypeName(argTypes[0]->getName())) {
                        std::cerr << "Error: Tensor fallback '" << functionName << "' requires one tensor argument" << std::endl;
                        return nullptr;
                    }
                    if (functionName == "relu") {
                        return TensorRuntime::mapTensor(cg, argValues[0], "relu");
                    }
                    return TensorRuntime::softmaxTensor(cg, argValues[0]);
                }
                if (functionName == "sum" || functionName == "mean" || functionName == "norm") {
                    if (argValues.size() != 1 || argTypes.empty() || !TensorRuntime::isTensorTypeName(argTypes[0]->getName())) {
                        std::cerr << "Error: Tensor fallback '" << functionName << "' requires one tensor argument" << std::endl;
                        return nullptr;
                    }
                    if (functionName == "sum") return TensorRuntime::sumTensor(cg, argValues[0]);
                    if (functionName == "mean") return TensorRuntime::meanTensor(cg, argValues[0]);
                    return TensorRuntime::normTensor(cg, argValues[0]);
                }
                if (functionName == "mse") {
                    if (argValues.size() != 2 ||
                        argTypes.size() != 2 ||
                        !TensorRuntime::isTensorTypeName(argTypes[0]->getName()) ||
                        !TensorRuntime::isTensorTypeName(argTypes[1]->getName())) {
                        std::cerr << "Error: Tensor fallback 'mse' requires two tensor arguments" << std::endl;
                        return nullptr;
                    }
                    return TensorRuntime::mseTensor(cg, argValues[0], argValues[1]);
                }
                if (functionName == "innerProduct" || functionName == "outerProduct" || functionName == "tensorProduct") {
                    if (argValues.size() != 2 ||
                        argTypes.size() != 2 ||
                        !TensorRuntime::isTensorTypeName(argTypes[0]->getName()) ||
                        !TensorRuntime::isTensorTypeName(argTypes[1]->getName())) {
                        std::cerr << "Error: Tensor fallback '" << functionName << "' requires two tensor arguments" << std::endl;
                        return nullptr;
                    }
                    if (functionName == "innerProduct") {
                        return TensorRuntime::innerProduct(cg, argValues[0], argValues[1]);
                    }
                    return TensorRuntime::outerProduct(cg, argValues[0], argValues[1]);
                }
                if (isNativeScalarMathFunction(functionName)) {
                    return codegenNativeScalarMathCall(functionName, argValues, argTypes);
                }
                return llvm::ConstantFP::get(llvm::Type::getDoubleTy(cg.context), 0.0);
            }
            std::cerr << "Error: Not found function name '" << functionName << "'" << std::endl;
            return nullptr;
        }
    }

    return nullptr;
}

std::unique_ptr<Type> FunctionCallNode::getType() {
    if (type) return type->clone();

    // free(x) is a statement-like builtin that releases a heap allocation; it yields no value.
    if (functionName == "free" || functionName == "assert") {
        return std::make_unique<BasicType>("Unit");
    }
    if (functionName == "__token_matches") {
        return std::make_unique<BasicType>("Boolean");
    }

    // clone(x) yields a fresh independently-owned value of the same type as x; it does not move x.
    if (functionName == "clone") {
        if (!arguments.empty() && arguments[0]) {
            return arguments[0]->getType();
        }
        return std::make_unique<UnknownType>();
    }

    // Shared(x) : Shared<typeof x>
    if (functionName == "Shared") {
        std::vector<std::unique_ptr<Type>> args;
        args.push_back(!arguments.empty() && arguments[0] ? arguments[0]->getType()
                                                          : std::make_unique<UnknownType>());
        return std::make_unique<GenericType>(std::make_unique<BasicType>("Shared"), args);
    }

    // Weak(s) : Weak<T> where s : Shared<T>
    if (functionName == "Weak") {
        std::vector<std::unique_ptr<Type>> args;
        auto argType = !arguments.empty() && arguments[0] ? arguments[0]->getType() : std::make_unique<UnknownType>();
        if (auto* gen = dynamic_cast<GenericType*>(argType.get())) {
            if (!gen->typeArguments.empty() && gen->typeArguments[0]) {
                args.push_back(gen->typeArguments[0]->clone());
            }
        }
        if (args.empty()) args.push_back(std::make_unique<UnknownType>());
        return std::make_unique<GenericType>(std::make_unique<BasicType>("Weak"), args);
    }

    std::vector<std::unique_ptr<Type>> argTypes;
    for (auto& arg : arguments) {
        auto argType = arg->getType();
        argTypes.push_back(argType ? argType->clone() : std::make_unique<UnknownType>());
    }

    if (isNativeIOFunction(functionName)) {
        if (functionName == "print" || functionName == "println") {
            return std::make_unique<BasicType>("Unit");
        }
        if (functionName == "readLine") {
            return std::make_unique<BasicType>("String");
        }
        if (functionName == "readChar") {
            return std::make_unique<BasicType>("Char");
        }
        if (functionName == "readInt") {
            return std::make_unique<BasicType>("Int");
        }
        if (functionName == "readDouble") {
            return std::make_unique<BasicType>("Double");
        }
    }
    if (isNativeProcessFunction(functionName)) {
        if (functionName == "commandLineArgCount") {
            return std::make_unique<BasicType>("Int");
        }
        return std::make_unique<BasicType>("String");
    }
    if (isNativeNetworkFunction(functionName)) {
        if (functionName == "tcpConnect" || functionName == "tcpListen" || functionName == "tcpAccept") {
            return std::make_unique<BasicType>("Long");
        }
        if (functionName == "tcpRecv") {
            return std::make_unique<BasicType>("String");
        }
        return std::make_unique<BasicType>("Int");
    }
    if (isNativePosixFunction(functionName)) {
        if (functionName == "posixRead" || functionName == "posixGetcwd" || functionName == "posixGetenv") {
            return std::make_unique<BasicType>("String");
        }
        if (functionName == "posixLseek" || functionName == "posixTime") {
            return std::make_unique<BasicType>("Long");
        }
        return std::make_unique<BasicType>("Int");
    }
    if (isNativeSystemFileFunction(functionName)) {
        if (functionName == "systemFileReadAllText") {
            return std::make_unique<BasicType>("String");
        }
        if (functionName == "systemFileExists") {
            return std::make_unique<BasicType>("Boolean");
        }
        return std::make_unique<BasicType>("Int");
    }
    if (resolvesToMicrothreadBuiltin(functionName)) {
        if (functionName == "isAlive") return std::make_unique<BasicType>("Boolean");
        if (functionName == "threadStart") return std::make_unique<BasicType>("Long");
        return std::make_unique<BasicType>("Int");
    }
    if (isNativeParallelFunction(functionName)) {
        return std::make_unique<BasicType>("Int");
    }
    if (isNativeDynamicLibraryFunction(functionName)) {
        if (functionName == "closeLibrary") {
            return std::make_unique<BasicType>("Int");
        }
        if (functionName == "callNativeDouble0" || functionName == "callNativeDouble1" ||
            functionName == "callNativeDouble2") {
            return std::make_unique<BasicType>("Double");
        }
        return std::make_unique<BasicType>("Long");
    }
    if (isNativeSimulationFunction(functionName)) {
        return std::make_unique<BasicType>("Double");
    }
    if (isBuiltinOdeFunction(functionName)) {
        return std::make_unique<BasicType>("Double");
    }

    // Check if this is a template function - return type is unknown until instantiation
    auto& cg = CodeGenerator::getInstance();
    auto* symbol = cg.symbolTable.lookup(functionName);
    if (symbol && symbol->symbolType == SymbolType::TEMPLATE) {
        auto* tmplSymbol = dynamic_cast<TemplateSymbol*>(symbol);
        auto* funcDecl = tmplSymbol ? dynamic_cast<FunctionDeclarationNode*>(tmplSymbol->declaration.get()) : nullptr;
        if (!tmplSymbol || !funcDecl || !funcDecl->returnType) {
            return std::make_unique<UnknownType>();
        }

        const bool hasFullTemplateParams = !tmplSymbol->templateParams.empty();
        const size_t templateParamCount = hasFullTemplateParams
            ? tmplSymbol->templateParams.size()
            : tmplSymbol->typeParameters.size();
        std::vector<std::unique_ptr<Type>> concreteTypes(templateParamCount);

        if (!explicitTypeArgs.empty()) {
            for (size_t i = 0; i < explicitTypeArgs.size() && i < concreteTypes.size(); ++i) {
                concreteTypes[i] = explicitTypeArgs[i]->clone();
            }
        } else {
            for (size_t pi = 0; pi < funcDecl->parameters.size() && pi < argTypes.size(); ++pi) {
                auto* paramNode = dynamic_cast<ParameterNode*>(funcDecl->parameters[pi].get());
                if (!paramNode || !paramNode->type || paramNode->type->getKind() != Type::Kind::VARIABLE) {
                    continue;
                }
                for (size_t ti = 0; ti < tmplSymbol->typeParameters.size(); ++ti) {
                    if (paramNode->type->getName() == tmplSymbol->typeParameters[ti] && ti < concreteTypes.size() && !concreteTypes[ti]) {
                        concreteTypes[ti] = argTypes[pi] ? argTypes[pi]->clone() : std::make_unique<UnknownType>();
                    }
                }
            }
        }

        if (funcDecl->returnType->getKind() == Type::Kind::VARIABLE) {
            for (size_t i = 0; i < tmplSymbol->typeParameters.size(); ++i) {
                if (funcDecl->returnType->getName() == tmplSymbol->typeParameters[i] &&
                    i < concreteTypes.size() && concreteTypes[i]) {
                    return concreteTypes[i]->clone();
                }
            }
        }

        return funcDecl->returnType->clone();
    }

    auto functionSymbolOpt = cg.symbolTable.lookupFunction(functionName, argTypes);
    if (!functionSymbolOpt) {
        if (isMathFallbackFunction(functionName)) {
            if ((functionName == "transpose" || functionName == "conjugate" || functionName == "adjoint") &&
                argTypes.size() == 1 &&
                TensorRuntime::isTensorTypeName(argTypes[0]->getName())) {
                return argTypes[0]->clone();
            }
            if ((functionName == "relu" || functionName == "softmax") &&
                argTypes.size() == 1 &&
                TensorRuntime::isTensorTypeName(argTypes[0]->getName())) {
                return argTypes[0]->clone();
            }
            if ((functionName == "sum" || functionName == "mean" || functionName == "norm") &&
                argTypes.size() == 1 &&
                TensorRuntime::isTensorTypeName(argTypes[0]->getName())) {
                return std::make_unique<BasicType>("Real");
            }
            if (functionName == "mse" &&
                argTypes.size() == 2 &&
                TensorRuntime::isTensorTypeName(argTypes[0]->getName()) &&
                TensorRuntime::isTensorTypeName(argTypes[1]->getName())) {
                return std::make_unique<BasicType>("Real");
            }
            if (functionName == "approxEq" || functionName == "between" || functionName == "disjoint") {
                return std::make_unique<BasicType>("Boolean");
            }
            if (functionName == "emptySet" || functionName == "singleton" ||
                functionName == "cardinality" || functionName == "complement") {
                return std::make_unique<BasicType>("Long");
            }
            if ((functionName == "outerProduct" || functionName == "tensorProduct") &&
                argTypes.size() == 2 &&
                TensorRuntime::isTensorTypeName(argTypes[0]->getName()) &&
                TensorRuntime::isTensorTypeName(argTypes[1]->getName())) {
                return argTypes[0]->clone();
            }
            if (functionName == "innerProduct" &&
                argTypes.size() == 2 &&
                TensorRuntime::isTensorTypeName(argTypes[0]->getName()) &&
                TensorRuntime::isTensorTypeName(argTypes[1]->getName())) {
                return std::make_unique<BasicType>("Real");
            }
            return std::make_unique<BasicType>("Real");
        }
        auto* fallbackSymbol = cg.symbolTable.lookup(functionName);
        if (fallbackSymbol && fallbackSymbol->type && fallbackSymbol->type->getKind() == Type::Kind::FUNCTION) {
            auto* fallbackFuncType = dynamic_cast<FunctionType*>(fallbackSymbol->type.get());
            if (fallbackFuncType && fallbackFuncType->returnType) {
                return fallbackFuncType->returnType->clone();
            }
        }
        return std::make_unique<UnknownType>();
    }

    auto* functionSymbol = functionSymbolOpt;

    auto* funcType = dynamic_cast<FunctionType*>(functionSymbol->type.get());
    if (funcType && funcType->returnType) {
        return funcType->returnType->clone();
    }

    return std::make_unique<UnknownType>();
}

bool isNativeMicrothreadFunction(const std::string& name) {
    return resolvesToMicrothreadBuiltin(name);
}
