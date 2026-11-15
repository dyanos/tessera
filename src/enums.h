// enums.h -- enum (algebraic data type) registry and the lowering pass (see enum_nodes.h).
#pragma once

#include "type.h"

#include <memory>
#include <string>
#include <vector>

class ASTNode;

struct EnumVariantInfo {
    std::string name;
    int tag = 0;
    std::vector<std::string> fieldNames;     // as declared: Circle(radius: Double) -> "radius"
    std::vector<std::string> storageNames;   // the lowered struct's fields: "__Circle_radius"
    std::vector<std::unique_ptr<Type>> fieldTypes;  // declared types (may name the type parameters)
};

struct EnumInfo {
    std::string name;
    std::vector<std::string> typeParameters;
    std::vector<EnumVariantInfo> variants;
    bool isReference = false;  // a variant refers to the enum itself: lowered to a class, not a struct
    bool hasPayload = false;
    const EnumVariantInfo* variant(const std::string& name) const;
};

// The tag field every lowered enum starts with.
inline const char* enumTagField() { return "__tag"; }

const EnumInfo* findEnum(const std::string& name);
// Some enum that declares a variant named `variant` (null when none does).
const EnumInfo* enumDeclaringVariant(const std::string& variant);
// The enum whose lowered class `type` is (`Shape`, or the generic instance `Option$Int`), and the
// instance's type arguments.
const EnumInfo* enumOfType(const Type* type, std::vector<std::unique_ptr<Type>>* typeArguments = nullptr);
// `type` with the enum's type parameters replaced by `arguments` (applied enum types resolved).
std::unique_ptr<Type> substituteEnumTypeParameters(const Type* type, const EnumInfo& info,
                                                   const std::vector<std::unique_ptr<Type>>& arguments);
// The lowered class of `info` instantiated with `arguments` (instantiating a generic enum).
std::string enumInstanceClass(const EnumInfo& info, const std::vector<std::unique_ptr<Type>>& arguments);

// Replaces every `enum` declaration of the parsed program by its lowered class and rewrites the
// variant constructions (`Shape.Circle(1.0)`, `Circle(1.0)`, `None`) into EnumConstructNode.
// Also adds the built-in Option/Result enums when the program uses them. Prints diagnostics and
// returns false on an invalid enum.
bool lowerEnums(ASTNode* program);
