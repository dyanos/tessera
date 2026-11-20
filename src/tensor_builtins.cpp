// tensor_builtins.cpp -- reductions along an axis, extrema, argmax/argmin, *Like constructors and
// random fills for tensors. Implemented by the runtime (native_runtime/tensor_runtime.inc), which
// shares tess's tensor layout { i64 rank, i64* dims, i64 count, double* data }.
//
//   sum(t, 0)  mean(t, 1)  max(t, -1)  min(t, 0)  prod(t, 0)   -- a tensor without that axis
//   max(t)  min(t)  prod(t)                                    -- a Real
//   argmax(t)  argmin(t)                                       -- the flat (row-major) index, a Long
//   zerosLike(t)  onesLike(t)  fullLike(t, v)                  -- a tensor of t's shape
//   fillUniform(t, low, high[, rng])  fillNormal(t, mean, sd[, rng])   -- in place
#include "codegen.h"

#include "tensor_builtins.h"
#include "FunctionCallNode.h"
#include "PrefixExpressionNode.h"
#include "UnaryExpressionNode.h"
#include "ValueNode.h"
#include "TensorRuntime.h"
#include "type_utils.h"

#include <iostream>

namespace {

bool isTensor(const Type* type) {
    return type && TensorRuntime::isTensorTypeName(type->getName());
}

// The axis argument as a constant: `1`, `-1`.
bool constantAxis(ASTNode* node, long long* axis) {
    if (auto* value = dynamic_cast<ValueNode*>(node)) {
        if (value->valueType != TokenType::INTEGER_LITERAL) return false;
        *axis = std::stoll(value->value);
        return true;
    }
    auto negate = [&](ASTNode* inner) {
        long long v = 0;
        if (!constantAxis(inner, &v)) return false;
        *axis = -v;
        return true;
    };
    if (auto* unary = dynamic_cast<UnaryExpressionNode*>(node)) { if (unary->op == "-") return negate(unary->expression.get()); }
    if (auto* prefix = dynamic_cast<PrefixExpressionNode*>(node)) { if (prefix->op == "-") return negate(prefix->expression.get()); }
    return false;
}

int reductionOp(const std::string& name) {
    if (name == "sum") return 0;
    if (name == "mean") return 1;
    if (name == "max") return 2;
    if (name == "min") return 3;
    if (name == "prod") return 4;
    return -1;
}

llvm::FunctionCallee runtime(CodeGenerator& cg, const char* name, llvm::Type* ret, std::vector<llvm::Type*> params) {
    cg.requireSystemNative();
    return cg.module->getOrInsertFunction(name, llvm::FunctionType::get(ret, params, false));
}

}  // namespace

std::unique_ptr<Type> tensorBuiltinType(FunctionCallNode& node) {
    const std::string& name = node.functionName;
    const size_t n = node.arguments.size();
    if (n == 0 || !node.arguments[0]) return nullptr;
    auto first = node.arguments[0]->getType();
    if (!isTensor(first.get())) return nullptr;
    const int op = reductionOp(name);
    if (op >= 0 && n == 1 && name != "sum" && name != "mean") return std::make_unique<BasicType>("Real");
    if (op >= 0 && n == 2) {
        long long axis = 0;
        auto* generic = dynamic_cast<GenericType*>(first.get());
        if (!generic || !constantAxis(node.arguments[1].get(), &axis)) return nullptr;
        const long long rank = static_cast<long long>(generic->typeArguments.size()) - 1;
        if (axis < 0) axis += rank;
        if (rank <= 0 || axis < 0 || axis >= rank) return nullptr;
        std::vector<std::unique_ptr<Type>> args;
        args.push_back(generic->typeArguments[0]->clone());
        for (long long i = 0; i < rank; ++i) {
            if (i != axis) args.push_back(generic->typeArguments[static_cast<size_t>(i + 1)]->clone());
        }
        if (args.size() == 1) args.push_back(std::make_unique<TypeVariableType>("1"));
        return std::make_unique<GenericType>(std::make_unique<BasicType>("Tensor"), args);
    }
    if ((name == "argmax" || name == "argmin") && n == 1) return std::make_unique<BasicType>("Long");
    if ((name == "zerosLike" || name == "onesLike") && n == 1) return first;
    if (name == "fullLike" && n == 2) return first;
    if ((name == "fillUniform" || name == "fillNormal") && (n == 3 || n == 4)) return std::make_unique<BasicType>("Unit");
    return nullptr;
}

llvm::Value* codegenTensorBuiltin(FunctionCallNode& node, bool* handled) {
    *handled = false;
    auto resultType = tensorBuiltinType(node);
    if (!resultType) return nullptr;
    *handled = true;
    auto& cg = CodeGenerator::getInstance();
    auto* ptrTy = llvm::PointerType::getUnqual(cg.context);
    auto* f64 = llvm::Type::getDoubleTy(cg.context);
    auto* i64 = llvm::Type::getInt64Ty(cg.context);
    auto* i32 = llvm::Type::getInt32Ty(cg.context);
    const std::string& name = node.functionName;
    llvm::Value* tensor = node.arguments[0]->codegen();
    if (!tensor) return nullptr;
    auto toDouble = [&](llvm::Value* v) { return coerceValueToLLVMType(v, f64); };
    auto borrowed = [&](llvm::Value* result) { return cg.borrowTensors({ tensor }, result); };

    const int op = reductionOp(name);
    if (op >= 0 && node.arguments.size() == 1) {
        return borrowed(cg.builder.CreateCall(runtime(cg, "csec_tensor_reduce_all", f64, {ptrTy, i32}),
                                              {tensor, llvm::ConstantInt::get(i32, op)}, name + ".all"));
    }
    if (op >= 0) {
        long long axis = 0;
        constantAxis(node.arguments[1].get(), &axis);
        llvm::Value* result = cg.builder.CreateCall(runtime(cg, "csec_tensor_reduce_axis", ptrTy, {ptrTy, i64, i32}),
                                                    {tensor, llvm::ConstantInt::get(i64, axis, true), llvm::ConstantInt::get(i32, op)},
                                                    name + ".axis");
        cg.markFreshTensor(result);
        return borrowed(result);
    }
    if (name == "argmax" || name == "argmin") {
        return borrowed(cg.builder.CreateCall(runtime(cg, "csec_tensor_argext", i64, {ptrTy, i32}),
                                              {tensor, llvm::ConstantInt::get(i32, name == "argmax" ? 1 : 0)}, name));
    }
    if (name == "zerosLike" || name == "onesLike" || name == "fullLike") {
        llvm::Value* value = name == "zerosLike" ? llvm::ConstantFP::get(f64, 0.0)
                           : name == "onesLike"  ? llvm::ConstantFP::get(f64, 1.0)
                                                 : toDouble(node.arguments[1]->codegen());
        llvm::Value* like = cg.builder.CreateCall(runtime(cg, "csec_tensor_full_like", ptrTy, {ptrTy, f64}), {tensor, value}, name);
        cg.markFreshTensor(like);
        return borrowed(like);
    }
    // fillUniform / fillNormal
    llvm::Value* a = toDouble(node.arguments[1]->codegen());
    llvm::Value* b = toDouble(node.arguments[2]->codegen());
    llvm::Value* rng = node.arguments.size() == 4 ? coerceValueToLLVMType(node.arguments[3]->codegen(), i64)
                                                  : llvm::ConstantInt::get(i64, 0);
    cg.builder.CreateCall(runtime(cg, "csec_tensor_fill_random", llvm::Type::getVoidTy(cg.context), {ptrTy, i64, i32, f64, f64}),
                          {tensor, rng, llvm::ConstantInt::get(i32, name == "fillNormal" ? 1 : 0), a, b});
    return llvm::ConstantInt::get(i32, 0);
}
