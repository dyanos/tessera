#pragma once

#include "type.h"

#include <charconv>
#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>

namespace TensorShape {
using Bindings = std::unordered_map<std::string, const Type*>;

inline const GenericType* tensor(const Type* type) {
    if (const auto* borrow = dynamic_cast<const BorrowType*>(type)) {
        type = borrow->baseType.get();
    }
    const auto* generic = dynamic_cast<const GenericType*>(type);
    return generic && generic->getName() == "Tensor" ? generic : nullptr;
}

inline bool isSymbol(const Type* type) {
    const auto* axis = dynamic_cast<const TensorAxisType*>(type);
    return axis && axis->form == TensorAxisType::Form::Symbol;
}

inline std::set<std::string> variables(const Type* type) {
    std::set<std::string> names;
    if (const auto* shaped = tensor(type)) {
        for (size_t i = 1; i < shaped->typeArguments.size(); ++i) {
            const auto* axis = shaped->typeArguments[i].get();
            if (isSymbol(axis)) names.insert(axis->getName());
        }
    }
    return names;
}

inline bool symbolic(const Type* type) {
    if (const auto* shaped = tensor(type)) {
        for (size_t i = 1; i < shaped->typeArguments.size(); ++i) {
            if (isSymbol(shaped->typeArguments[i].get())) return true;
        }
    }
    return false;
}

inline bool hasSignature(const FunctionType& type) {
    if (symbolic(type.returnType.get())) return true;
    for (const auto& parameter : type.parameterTypes) {
        if (symbolic(parameter.get())) return true;
    }
    return false;
}

inline std::optional<int64_t> literal(const Type* type) {
    if (!type || type->getKind() != Type::Kind::VARIABLE) return std::nullopt;
    const std::string name = type->getName();
    const char* end = name.data() + name.size();
    int64_t size = 0;
    const auto [ptr, error] = std::from_chars(name.data(), end, size);
    if (error != std::errc{} || ptr != end) return std::nullopt;
    return size;
}

inline bool proven(const Type* type) {
    if (const auto* axis = dynamic_cast<const TensorAxisType*>(type)) {
        return axis->form != TensorAxisType::Form::RuntimeReference;
    }
    const auto size = literal(type);
    return size && *size > 0;
}

inline bool sameAxis(const Type* left, const Type* right) {
    if (!proven(left) || !proven(right)) return false;
    const auto leftSize = literal(left);
    const auto rightSize = literal(right);
    if (leftSize && rightSize) return leftSize == rightSize;
    return left->equals(*right);
}

inline size_t rank(const GenericType& type) {
    const auto& arguments = type.typeArguments;
    if (arguments.empty() || (arguments.size() == 2 && arguments[1]->getName() == "0" && literal(arguments[1].get()) == 0)) {
        return 0;
    }
    return arguments.size() - 1;
}

inline std::string match(const Type* expected, const Type* provenActual, Bindings* bindings = nullptr) {
    const auto* required = tensor(expected);
    const auto* actual = tensor(provenActual);
    if (!required || !actual || required->typeArguments.size() < 2 || actual->typeArguments.size() < 2) {
        return "cannot prove the tensor shape";
    }
    const auto& element = required->typeArguments.front();
    const auto& actualElement = actual->typeArguments.front();
    if (!element->equals(*actualElement)) {
        return "expected tensor element type '" + element->getName() + "', got '" + actualElement->getName() + "'";
    }
    const size_t expectedRank = rank(*required);
    const size_t actualRank = rank(*actual);
    if (expectedRank != actualRank) {
        return "expected tensor rank " + std::to_string(expectedRank) + ", got " + std::to_string(actualRank);
    }
    for (size_t i = 1; i <= expectedRank; ++i) {
        const Type* axis = required->typeArguments[i].get();
        const Type* actualAxis = actual->typeArguments[i].get();
        const std::string name = axis->getName();
        if (bindings && isSymbol(axis) && proven(actualAxis)) {
            const auto [entry, inserted] = bindings->try_emplace(name, actualAxis);
            if (inserted || sameAxis(entry->second, actualAxis)) continue;
            if (literal(entry->second) && literal(actualAxis)) {
                return "tensor dimension '" + name + "' has conflicting sizes " + entry->second->getName() +
                    " and " + actualAxis->getName() + " at axis " + std::to_string(i);
            }
            return "cannot prove equal sizes for tensor dimension '" + name + "' at axis " + std::to_string(i);
        }
        if (!sameAxis(axis, actualAxis)) {
            if (literal(axis) && literal(actualAxis)) {
                return "expected size " + name + " at tensor axis " + std::to_string(i) + ", got " + actualAxis->getName();
            }
            return "cannot prove tensor axis " + std::to_string(i) + " matches '" + name + "'";
        }
    }
    return {};
}

inline std::unique_ptr<Type> substitute(const Type* type, const Bindings& bindings) {
    if (!type) return nullptr;
    if (const auto* borrow = dynamic_cast<const BorrowType*>(type)) {
        return std::make_unique<BorrowType>(substitute(borrow->baseType.get(), bindings), borrow->isMutableBorrow);
    }
    if (const auto* shaped = tensor(type)) {
        auto result = std::make_unique<GenericType>(*shaped);
        for (size_t i = 1; i < result->typeArguments.size(); ++i) {
            auto& axis = result->typeArguments[i];
            if (!isSymbol(axis.get())) continue;
            const auto entry = bindings.find(axis->getName());
            if (entry != bindings.end()) axis = const_cast<Type*>(entry->second)->clone();
        }
        return result;
    }
    return const_cast<Type*>(type)->clone();
}
}
