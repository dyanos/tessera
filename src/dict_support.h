#pragma once

// Code generation helpers for dictionaries: the `{key: value, ...}` literal and the `Dict<K, V>`
// type. A dictionary is an opaque runtime handle (a pointer); keys and values cross into the runtime
// (src/native_runtime/dict_runtime.inc) as 64-bit payloads.

#include "type.h"

#include <memory>
#include <string>
#include <vector>

namespace llvm {
class Value;
}

class ASTNode;

// Type::clone() is not const; dictionary helpers work on const Type* views.
inline std::unique_ptr<Type> cloneType(const Type* type) {
    return type ? const_cast<Type*>(type)->clone() : nullptr;
}

// Dict<K, V> (also through a borrow) -> true.
bool isDictType(const Type* type);
std::unique_ptr<Type> makeDictType(const Type* keyType, const Type* valueType);
// The K / V of a Dict type (borrows are looked through); nullptr when `type` is not a dictionary.
const Type* dictKeyType(const Type* type);
const Type* dictValueType(const Type* type);
bool dictCanCopy(const Type* type);

// Keys must be hashable scalars; values must fit one 64-bit slot (no value structs or tuples).
bool isValidDictKeyType(const Type* type);
bool isValidDictValueType(const Type* type);

// The common type of two dictionary keys/values/list elements, or nullptr if they do not unify.
// Int/Long widen to Long, integers mix with Float/Double as Double, arrays of one element type but
// different lengths become Vector[T], nested dictionaries unify entry-wise, and anything else
// (class types included) must match exactly.
std::unique_ptr<Type> unifyDictElementTypes(const Type* a, const Type* b);

// Convert a value of `type` to its 64-bit dictionary payload, and back.
llvm::Value* dictPayloadFromValue(llvm::Value* value, const Type* type);
llvm::Value* dictValueFromPayload(llvm::Value* payload, const Type* type);

// Evaluate `expr` as a value of `targetType` (an array literal becomes a heap copy so it can outlive
// the current function) and return its 64-bit payload. `lengthOut` receives the element count of an
// array literal, or -1.
llvm::Value* codegenDictPayload(ASTNode* expr, const Type* targetType, long long* lengthOut);

// Load the dictionary handle produced by evaluating `dictExpr`.
llvm::Value* codegenDictHandle(ASTNode* dictExpr);

// dict[key] (KeyError at runtime when missing) and dict[key] = value.
llvm::Value* codegenDictGet(ASTNode* dictExpr, ASTNode* keyExpr);
llvm::Value* codegenDictSet(ASTNode* dictExpr, ASTNode* keyExpr, ASTNode* valueExpr, const std::string& op);
// key in dict
llvm::Value* codegenDictContains(ASTNode* dictExpr, ASTNode* keyExpr);
// A new, empty dictionary for `dictType`.
llvm::Value* codegenDictNew(const Type* dictType);
// dict.toString() for printing and String concatenation.
llvm::Value* codegenDictToString(llvm::Value* handle);

// ---- Sets: the `{a, b, c}` literal and Set<T> (a dictionary that only uses its keys) ----

bool isSetType(const Type* type);
std::unique_ptr<Type> makeSetType(const Type* elementType);
const Type* setElementType(const Type* type);
// A new, empty set for `setType`; a set holding the evaluated `elements`.
llvm::Value* codegenSetNew(const Type* setType);
llvm::Value* codegenSetLiteral(const Type* setType, std::vector<std::unique_ptr<ASTNode>>& elements);
// element in set
llvm::Value* codegenSetContains(ASTNode* setExpr, ASTNode* elementExpr);
// Set algebra for `|`/union, `&`/intersect, `-`/diff/without, `^`/symdiff (a new set), and
// `==`, `!=`, `<=`/subset, `>=`/superset, `<`/properSubset, `>`/properSuperset (a Bool).
// nullptr when `op` is not a set operator.
bool isSetBinaryOperator(const std::string& op);
std::unique_ptr<Type> setBinaryOperatorType(const std::string& op, const Type* leftType, const Type* rightType);
llvm::Value* codegenSetBinary(const std::string& op, ASTNode* left, ASTNode* right);
// add, remove/discard, contains/has, size/length/count, isEmpty, clear, copy, toString, elementAt,
// union, intersection, difference, symmetricDifference, isSubset, isSuperset, isDisjoint.
llvm::Value* codegenSetMethod(ASTNode* setExpr, const std::string& method,
                              std::vector<std::unique_ptr<ASTNode>>& arguments, bool& handled);
std::unique_ptr<Type> setMethodType(const Type* setType, const std::string& method, size_t argumentCount);

// Methods: size/length/count, isEmpty, contains/has/containsKey, get(key[, default]), remove, clear,
// copy, toString, keyAt(i), valueAt(i). Returns nullptr and sets `handled` false for other names.
llvm::Value* codegenDictMethod(ASTNode* dictExpr, const std::string& method,
                               std::vector<std::unique_ptr<ASTNode>>& arguments, bool& handled);
std::unique_ptr<Type> dictMethodType(const Type* dictType, const std::string& method, size_t argumentCount);
