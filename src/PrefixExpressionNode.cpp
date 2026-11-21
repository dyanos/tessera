#include "codegen.h"

#include "PrefixExpressionNode.h"
#include "ASTVisitor.h"
#include "IdentifierNode.h"

#include "FunctionCallNode.h"
#include "symbol.h"
#include "type_checker.h"
#include "type_utils.h"

#include <iostream>
#include <llvm/IR/Constants.h>

namespace {

auto* opaquePointer(CodeGenerator& cg) { return llvm::PointerType::getUnqual(cg.context); }

llvm::Function* runtimeFunction(CodeGenerator& cg, const std::string& name, llvm::Type* ret, const std::vector<llvm::Type*>& params) {
    cg.requireSystemNative();
    if (auto* existing = cg.module->getFunction(name)) return existing;
    return llvm::Function::Create(llvm::FunctionType::get(ret, params, false),
        llvm::Function::ExternalLinkage, name, cg.module.get());
}

// Future payloads are 64-bit: integers widen, Double is bit-cast, pointers become addresses.
llvm::Value* toFuturePayload(CodeGenerator& cg, llvm::Value* value) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    if (!value || value->getType()->isVoidTy()) return llvm::ConstantInt::get(i64Ty, 0);
    llvm::Type* type = value->getType();
    if (type->isIntegerTy(1)) return cg.builder.CreateZExt(value, i64Ty, "future.payload");
    if (type->isIntegerTy()) {
        return type->getIntegerBitWidth() == 64 ? value : cg.builder.CreateSExtOrTrunc(value, i64Ty, "future.payload");
    }
    if (type->isFloatTy()) value = cg.builder.CreateFPExt(value, llvm::Type::getDoubleTy(cg.context));
    if (value->getType()->isDoubleTy()) return cg.builder.CreateBitCast(value, i64Ty, "future.payload");
    if (type->isPointerTy()) return cg.builder.CreatePtrToInt(value, i64Ty, "future.payload");
    std::cerr << "Error: async result of this type cannot be carried by a Future" << std::endl;
    return llvm::ConstantInt::get(i64Ty, 0);
}

llvm::Value* fromFuturePayload(CodeGenerator& cg, llvm::Value* raw, llvm::Type* target) {
    if (!target || target->isVoidTy()) return llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), 0);
    if (target->isIntegerTy()) return cg.builder.CreateTrunc(raw, target, "future.value");
    if (target->isDoubleTy()) return cg.builder.CreateBitCast(raw, target, "future.value");
    if (target->isFloatTy()) {
        return cg.builder.CreateFPTrunc(cg.builder.CreateBitCast(raw, llvm::Type::getDoubleTy(cg.context)), target, "future.value");
    }
    if (target->isPointerTy()) return cg.builder.CreateIntToPtr(raw, target, "future.value");
    return raw;
}

Type* futureValueType(const Type* type) {
    auto* generic = dynamic_cast<const GenericType*>(type);
    if (!generic || !generic->baseType || generic->baseType->getName() != "Future") return nullptr;
    return generic->typeArguments.empty() ? nullptr : generic->typeArguments[0].get();
}

llvm::Function* taskResultDrop(CodeGenerator& cg, const Type* type, llvm::Value* value) {
    if (!type || !value || !value->getType()->isPointerTy()) return nullptr;
    if (cg.isRefcountedType(type)) return cg.rcReleaseFunction();
    if (typeIsCopy(type)) return nullptr;
    if (auto* drop = cg.ownedValueDropFunction(type)) return drop;
    if (auto* drop = cg.arrayDropFunction(type)) return drop;
    const std::string name = "__csec_task_result_drop." + type->getName();
    auto* drop = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(cg.context), {opaquePointer(cg)}, false),
        llvm::Function::InternalLinkage, name, cg.module.get());
    llvm::IRBuilderBase::InsertPointGuard guard(cg.builder);
    cg.builder.SetCurrentDebugLocation(llvm::DebugLoc());
    cg.builder.SetInsertPoint(llvm::BasicBlock::Create(cg.context, "entry", drop));
    cg.emitOwnedValueDrop(drop->getArg(0), type);
    cg.builder.CreateRetVoid();
    return drop;
}

// `async f(args)` / `go f(args)`: evaluate the arguments now (the task shares nothing with the
// caller), then run the call in a new microthread. The call is re-generated inside a thunk
// `void thunk(ptr env)` whose arguments are bound to the env copies, so overload resolution and
// argument coercion are exactly those of an ordinary call. `async` fills a Future with the result.
llvm::Value* lowerConcurrentCall(FunctionCallNode* call, bool wantFuture) {
    auto& cg = CodeGenerator::getInstance();
    auto* ptrTy = opaquePointer(cg);
    auto* i32Ty = llvm::Type::getInt32Ty(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* voidTy = llvm::Type::getVoidTy(cg.context);

    std::vector<llvm::Value*> values;
    std::vector<std::unique_ptr<Type>> types;
    std::vector<llvm::Type*> fields{ ptrTy };
    for (auto& arg : call->arguments) {
        llvm::Value* value = arg ? arg->codegen() : nullptr;
        if (!value) {
            std::cerr << "Error: failed to evaluate argument of concurrent call to '" << call->functionName << "'" << std::endl;
            return nullptr;
        }
        auto type = arg->getType();
        if (isStructClassType(type.get())) {
            if (value->getType()->isPointerTy()) value = cg.builder.CreateLoad(getABIStorageType(type.get()), value, "task.struct");
        }
        else {
            value = cg.copyStackBufferToHeap(value);
        }
        value = cg.takeOwned(value, type.get());
        values.push_back(value);
        types.push_back(type ? type->clone() : std::make_unique<UnknownType>());
        fields.push_back(value->getType());
    }
    llvm::StructType* envType = llvm::StructType::get(cg.context, fields);

    llvm::Value* future = llvm::ConstantPointerNull::get(ptrTy);
    if (wantFuture) {
        auto* handle = cg.builder.CreateCall(runtimeFunction(cg, "csec_future_new", ptrTy, {}), {}, "future");
        cg.registerOwnedTemporary(handle, cg.rcReleaseFunction());
        future = handle;
    }
    llvm::Value* env = cg.createEntryAlloca(envType, "task.env");
    cg.builder.CreateStore(future, cg.builder.CreateStructGEP(envType, env, 0));
    for (size_t i = 0; i < values.size(); ++i) {
        cg.builder.CreateStore(values[i], cg.builder.CreateStructGEP(envType, env, static_cast<unsigned>(i + 1)));
    }

    static int taskCounter = 0;
    llvm::Function* thunk = llvm::Function::Create(
        llvm::FunctionType::get(voidTy, { ptrTy }, false), llvm::Function::InternalLinkage,
        "__task_" + call->functionName + "_" + std::to_string(taskCounter++), cg.module.get());

    llvm::BasicBlock* savedBlock = cg.builder.GetInsertBlock();
    auto savedPoint = cg.builder.GetInsertPoint();
    cg.builder.SetInsertPoint(llvm::BasicBlock::Create(cg.context, "entry", thunk));
    cg.symbolTable.enterScope();
    cg.enterTemporaryFrame();
    llvm::Value* taskEnv = thunk->getArg(0);
    llvm::Value* taskFuture = cg.builder.CreateLoad(ptrTy, cg.builder.CreateStructGEP(envType, taskEnv, 0), "task.future");
    auto synthetic = std::make_unique<FunctionCallNode>();
    synthetic->functionName = call->functionName;
    for (const auto& typeArg : call->explicitTypeArgs) {
        synthetic->explicitTypeArgs.push_back(typeArg ? typeArg->clone() : nullptr);
    }
    std::vector<llvm::Value*> taskArguments;
    for (size_t i = 0; i < values.size(); ++i) {
        const std::string name = "__task_arg_" + std::to_string(i);
        llvm::Value* argument = cg.builder.CreateLoad(fields[i + 1],
            cg.builder.CreateStructGEP(envType, taskEnv, static_cast<unsigned>(i + 1)), name + ".value");
        taskArguments.push_back(argument);
        const Type* argType = types[i].get();
        const bool boundDirectly = argType && (argType->getName() == "Array" || argType->getName() == "Vector" ||
                                               dynamic_cast<const ArrayType*>(argType) != nullptr ||
                                               argType->getKind() == Type::Kind::FUNCTION);
        llvm::Value* binding = argument;
        if (!boundDirectly) {
            binding = cg.createEntryAlloca(fields[i + 1], name);
            cg.builder.CreateStore(argument, binding);
        }
        cg.symbolTable.addSymbol(name, std::make_unique<Symbol>(name, types[i]->clone(), binding, false, SymbolType::VARIABLE));
        synthetic->arguments.push_back(std::make_unique<IdentifierNode>(name));
    }
    llvm::Value* result = synthetic->codegen();
    auto resultType = synthetic->getType();
    llvm::Function* drop = taskResultDrop(cg, resultType.get(), result);
    if (drop) result = cg.takeOwned(result, resultType.get());
    for (size_t i = 0; i < taskArguments.size(); ++i) {
        if (cg.isRefcountedType(types[i].get())) cg.releaseString(taskArguments[i]);
        else if (cg.structHoldsCounted(types[i].get())) cg.emitStructCount(taskArguments[i], types[i].get(), false);
    }
    if (wantFuture) {
        if (drop) {
            cg.builder.CreateCall(runtimeFunction(cg, "csec_future_complete_managed", voidTy, { ptrTy, i64Ty, ptrTy, i32Ty }),
                { taskFuture, toFuturePayload(cg, result), drop, llvm::ConstantInt::get(i32Ty, !cg.isRefcountedType(resultType.get())) });
        }
        else {
            cg.builder.CreateCall(runtimeFunction(cg, "csec_future_complete", voidTy, { ptrTy, i64Ty }),
                { taskFuture, toFuturePayload(cg, result) });
        }
        cg.releaseString(taskFuture);
    }
    else if (drop) cg.builder.CreateCall(drop, { result });
    cg.exitTemporaryFrame();
    cg.builder.CreateRetVoid();
    cg.symbolTable.exitScope();
    cg.builder.SetInsertPoint(savedBlock, savedPoint);

    // sizeof(env) as a constant expression so it follows the target data layout.
    llvm::Constant* envSize = llvm::ConstantExpr::getPtrToInt(llvm::ConstantExpr::getGetElementPtr(envType,
        llvm::ConstantPointerNull::get(ptrTy), llvm::ArrayRef<llvm::Constant*>{ llvm::ConstantInt::get(i32Ty, 1) }), i64Ty);
    llvm::Value* pid = wantFuture
        ? cg.builder.CreateCall(runtimeFunction(cg, "csec_future_start", i32Ty, { ptrTy, ptrTy, ptrTy, i64Ty }),
            { future, thunk, env, envSize }, "task.pid")
        : cg.builder.CreateCall(runtimeFunction(cg, "csec_process_spawn", i32Ty, { ptrTy, ptrTy, i64Ty }),
            { thunk, env, envSize }, "task.pid");
    if (!values.empty()) {
        auto* function = cg.builder.GetInsertBlock()->getParent();
        auto* failed = llvm::BasicBlock::Create(cg.context, "task.start.failed", function);
        auto* done = llvm::BasicBlock::Create(cg.context, "task.start.done", function);
        cg.builder.CreateCondBr(cg.builder.CreateICmpEQ(pid, llvm::ConstantInt::get(i32Ty, 0)), failed, done);
        cg.builder.SetInsertPoint(failed);
        for (size_t i = 0; i < values.size(); ++i) {
            if (cg.structHoldsCounted(types[i].get())) cg.emitStructCount(values[i], types[i].get(), false);
            else cg.emitOwnedValueDrop(values[i], types[i].get());
        }
        cg.builder.CreateBr(done);
        cg.builder.SetInsertPoint(done);
    }
    return wantFuture ? future : pid;
}

} // namespace



void PrefixExpressionNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* PrefixExpressionNode::codegen() {
    if (op == "async" || op == "go") {
        auto* call = dynamic_cast<FunctionCallNode*>(expression.get());
        if (!call) {
            std::cerr << "Error: '" << op << "' expects a function call" << std::endl;
            return nullptr;
        }
        return lowerConcurrentCall(call, op == "async");
    }
    llvm::Value* value = expression->codegen();
    if (!value) {
        std::cerr << "Error: Unary expression failed" << std::endl;
        return nullptr;
    }

    if (op == "&" || op == "&mut") {
        if (auto* id = dynamic_cast<IdentifierNode*>(expression.get())) {
            auto* symbol = CodeGenerator::getInstance().symbolTable.lookup(id->value);
            if (symbol && symbol->value) {
                // An immutable borrow of a pointer-valued local kept in a slot (String, array, …)
                // yields the loaded pointer, so the reference is the underlying object — matching how
                // class objects (bound as direct pointers) already behave, and letting member access
                // through a `&T` parameter dispatch correctly. A `&mut`, or a borrow of a by-value
                // primitive/aggregate slot, keeps the slot address so the callee can read/mutate it.
                if (op == "&" && llvm::isa<llvm::AllocaInst>(symbol->value) &&
                    value && value->getType()->isPointerTy()) {
                    return value;
                }
                return symbol->value;
            }
        }
        return value;
    }
    else if (op == "<-") {
        auto& cg = CodeGenerator::getInstance();
        cg.noteMoved(value);
        if (auto* id = dynamic_cast<IdentifierNode*>(expression.get())) {
            if (Symbol* symbol = cg.symbolTable.lookup(id->value)) {
                if (symbol->value && llvm::isa<llvm::AllocaInst>(symbol->value)) cg.noteMoved(symbol->value);
            }
        }
        return value;
    }
    else if (op == "await") {
        // Awaiting a Future<T> (from `async f(...)`) blocks this microthread until the task has
        // produced its result. Awaiting any other value (e.g. a call of an `async def`, which still
        // runs synchronously) yields the value itself.
        auto exprType = expression->getType();
        if (Type* valueType = futureValueType(exprType.get())) {
            auto& cg = CodeGenerator::getInstance();
            llvm::Value* raw = cg.builder.CreateCall(
                runtimeFunction(cg, "csec_future_await", llvm::Type::getInt64Ty(cg.context), { opaquePointer(cg) }),
                { value }, "future.raw");
            llvm::Value* result = fromFuturePayload(cg, raw, getABIStorageType(valueType));
            if (auto* drop = taskResultDrop(cg, valueType, result)) cg.registerOwnedTemporary(result, drop);
            return result;
        }
        return value;
    }
    else if (op == "++") {
        // increment
        // byte, char, word, short, int, long, long long, float, double, long double만 가능하도록 변경
        if (value->getType()->isIntegerTy(1)) { // byte

            return CodeGenerator::getInstance().builder.CreateAdd(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(1, 1)), "inc");
        }
        else if (value->getType()->isIntegerTy(8)) { // char
            return CodeGenerator::getInstance().builder.CreateAdd(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(8, 1)), "inc");
        }
        else if (value->getType()->isIntegerTy(16)) { // word, short
            return CodeGenerator::getInstance().builder.CreateAdd(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(16, 1)), "inc");
        }
        else if (value->getType()->isIntegerTy(32)) { // int
            return CodeGenerator::getInstance().builder.CreateAdd(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(32, 1)), "inc");
        }
        else if (value->getType()->isIntegerTy(64)) { // long, long long
            return CodeGenerator::getInstance().builder.CreateAdd(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(64, 1)), "inc");
        }
        else if (value->getType()->isFloatTy()) { // float
            return CodeGenerator::getInstance().builder.CreateFAdd(value, llvm::ConstantFP::get(CodeGenerator::getInstance().context, llvm::APFloat(1.0)), "inc");
        }
        else if (value->getType()->isDoubleTy()) { // double
            return CodeGenerator::getInstance().builder.CreateFAdd(value, llvm::ConstantFP::get(CodeGenerator::getInstance().context, llvm::APFloat(1.0)), "inc");
        }
        else if (value->getType()->isStructTy()) {
            // 구조체일 경우 구조체 안에 '++' 연산자가 정의되어 있는지 확인 후 사용할 수 있다면, 사용
            auto classType = llvm::cast<llvm::StructType>(value->getType());
            auto classSymbolOpt = CodeGenerator::getInstance().symbolTable.lookupClass(classType->getName().str());

            if (classSymbolOpt) {
                auto* classSymbol = classSymbolOpt;
                auto method = classSymbol->getMethod("operator--");
                if (method) {
                    // '++' 연산자 메서드 호출
                    std::vector<llvm::Value*> args;
                    return CodeGenerator::getInstance().builder.CreateCall(llvm::FunctionCallee(method->function), args, "inc");
                }
                else {
                    std::cerr << "Error: '++' operator not defined for class " << classType->getName().str() << std::endl;
                    return nullptr;
                }
            }
            else {
                std::cerr << "Error: Class symbol not found for type " << classType->getName().str() << std::endl;
                return nullptr;
            }
        }
        else {
            std::cerr << "Error: Increment operator not applicable to type" << std::endl;
            return nullptr;
        }
    }
    else if (op == "--") {
        // decrement
        // byte, char, word, short, int, long, long long, float, double, long double만 가능하도록 변경
        if (value->getType()->isIntegerTy(1)) { // byte
            return CodeGenerator::getInstance().builder.CreateSub(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(1, 1)), "dec");
        }
        else if (value->getType()->isIntegerTy(8)) { // char
            return CodeGenerator::getInstance().builder.CreateSub(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(8, 1)), "dec");
        }
        else if (value->getType()->isIntegerTy(16)) { // word, short
            return CodeGenerator::getInstance().builder.CreateSub(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(16, 1)), "dec");
        }
        else if (value->getType()->isIntegerTy(32)) { // int
            return CodeGenerator::getInstance().builder.CreateSub(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(32, 1)), "dec");
        }
        else if (value->getType()->isIntegerTy(64)) { // long, long long
            return CodeGenerator::getInstance().builder.CreateSub(value, llvm::ConstantInt::get(CodeGenerator::getInstance().context, llvm::APInt(64, 1)), "dec");
        }
        else if (value->getType()->isFloatTy()) { // float
            return CodeGenerator::getInstance().builder.CreateFSub(value, llvm::ConstantFP::get(CodeGenerator::getInstance().context, llvm::APFloat(1.0)), "dec");
        }
        else if (value->getType()->isDoubleTy()) { // double
            return CodeGenerator::getInstance().builder.CreateFSub(value, llvm::ConstantFP::get(CodeGenerator::getInstance().context, llvm::APFloat(1.0)), "dec");
        }
        else if (value->getType()->isStructTy()) {
            // 구조체일 경우 구조체 안에 '++' 연산자가 정의되어 있는지 확인 후 사용할 수 있다면, 사용
            auto classType = llvm::cast<llvm::StructType>(value->getType());
            auto classSymbolOpt = CodeGenerator::getInstance().symbolTable.lookupClass(classType->getName().str());

            if (classSymbolOpt) {
                auto* classSymbol = classSymbolOpt;
                auto method = classSymbol->getMethod("operator++");
                if (method) {
                    // '++' 연산자 메서드 호출
                    std::vector<llvm::Value*> args;
                    return CodeGenerator::getInstance().builder.CreateCall(llvm::FunctionCallee(method->function), args, "inc");
                }
                else {
                    std::cerr << "Error: '++' operator not defined for class " << classType->getName().str() << std::endl;
                    return nullptr;
                }
            }
            else {
                std::cerr << "Error: Class symbol not found for type " << classType->getName().str() << std::endl;
                return nullptr;
            }
        }
        else {
            std::cerr << "Error: Increment operator not applicable to type" << std::endl;
            return nullptr;
        }
    }
    else {
        std::cerr << "Unsupported unary operator: " << op << std::endl;
    }

    return nullptr;
}

std::unique_ptr<Type> PrefixExpressionNode::getType() {
    if (!expression) {
        return std::make_unique<UnknownType>();
    }
    if (op == "go") {
        return std::make_unique<BasicType>("Int");
    }
    auto exprType = expression->getType();
    if (op == "async") {
        std::vector<std::unique_ptr<Type>> args;
        args.push_back(exprType ? exprType->clone() : std::make_unique<UnknownType>());
        return std::make_unique<GenericType>(std::make_unique<BasicType>("Future"), args);
    }
    if (op == "await") {
        if (Type* valueType = futureValueType(exprType.get())) {
            return valueType->clone();
        }
        return exprType;
    }
    if (op == "&") {
        return std::make_unique<BorrowType>(exprType, false);
    }
    if (op == "&mut") {
        return std::make_unique<BorrowType>(exprType, true);
    }
    return exprType;
}
