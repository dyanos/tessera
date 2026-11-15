#pragma once

#include "type.h"
#include <vector>
#include <memory>

namespace llvm {
class Type;
class Value;
}

class ASTNode;

bool areTypesCompatible(const std::vector<std::unique_ptr<Type>>& paramTypes, const std::vector<std::unique_ptr<Type>>& argTypes);
bool isStructClassType(const Type* type);
// The name a template argument contributes to an instance name: `Int`, or `Opt$Int` for the applied
// template type Opt<Int> (so Opt<Opt<Int>> and its resolved form name the same instance).
std::string templateArgumentName(const Type* type);
// Auto-deref: if `type` is a borrow (`&T`/`&mut T`), return a clone of the borrowed `T`; otherwise a
// clone of `type` itself. Lets member access / indexing treat a borrowed receiver like the value.
std::unique_ptr<Type> stripBorrow(const std::unique_ptr<Type>& type);
llvm::Type* getABIStorageType(const Type* type);
llvm::Value* coerceValueToLLVMType(llvm::Value* value, llvm::Type* targetType);

// Where a Nat (arbitrary-precision natural number) is expected but an integer value/literal is given,
// promote it to a Nat handle: an integer *literal* becomes an exact big value via from_decimal (so it
// is never truncated to 64 bits), a runtime integer via from_i64, and an existing Nat handle passes
// through. Returns `value` unchanged when `declaredType` is not Nat. `sourceExpr` may be null.
llvm::Value* coerceToNat(llvm::Value* value, const Type* declaredType, ASTNode* sourceExpr);
