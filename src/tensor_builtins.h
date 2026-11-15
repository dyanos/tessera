// tensor_builtins.h -- axis reductions, argmax/argmin, *Like and random fills (see the .cpp).
#pragma once
#include "ast.h"

class FunctionCallNode;

// The result type when `node` calls one of these builtins on a tensor; null otherwise.
std::unique_ptr<Type> tensorBuiltinType(FunctionCallNode& node);
// Code for such a call; sets *handled to false (and returns null) for any other call.
llvm::Value* codegenTensorBuiltin(FunctionCallNode& node, bool* handled);
