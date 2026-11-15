// enum_nodes.h -- AST nodes for enums (algebraic data types), patterns and the `?` operator.
//
//   enum Shape { Circle(radius: Double), Rect(w: Double, h: Double), Empty }
//   enum Option<T> { Some(value: T), None }
//
// An enum is lowered before type checking (enums.cpp) to a struct -- a class when a variant refers to
// the enum itself -- whose first field is the variant tag and whose other fields are the payloads of
// every variant. EnumConstructNode builds a value of one variant; PatternNode is a resolved `case`
// pattern; TryNode is `expr?`.
#pragma once
#include "ast.h"
#include "ParameterNode.h"
#include "FunctionDeclarationNode.h"

#include <string>
#include <vector>

// `enum Name<T...> { Variant(field: Type, ...), ... def method() ... }` as parsed. Replaced by a class
// declaration before type checking, so it never reaches the type checker or code generation.
class EnumDeclarationNode : public ASTNode {
public:
    struct Variant {
        std::string name;
        std::vector<std::unique_ptr<ParameterNode>> fields;
        int line = 0;
    };
    std::string name;
    std::vector<std::string> typeParameters;
    std::vector<Variant> variants;
    std::vector<std::unique_ptr<ASTNode>> methods;  // FunctionDeclarationNode
    std::vector<std::string> interfaces;            // `implements A, B`

    EnumDeclarationNode() { nodeType = ASTNodeType::ENUM_DECLARATION; }
    EnumDeclarationNode(const EnumDeclarationNode& other);

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;
    std::unique_ptr<Type> getType() override { return std::make_unique<UnknownType>(); }
    std::unique_ptr<ASTNode> clone() override { return std::make_unique<EnumDeclarationNode>(*this); }
};

// A value of one variant: `Shape.Circle(1.0)`, `Circle(1.0)`, `None`, `Option<Int>.None`.
class EnumConstructNode : public ASTNode {
public:
    std::string enumName;                           // the enum (template name for a generic enum)
    std::string variantName;
    std::vector<std::unique_ptr<Type>> typeArguments;  // explicit or inferred (generic enums)
    std::vector<std::unique_ptr<ASTNode>> arguments;
    bool qualified = false;  // written with the enum name (`Shape.Circle`), not bare (`Circle`)

    EnumConstructNode() { nodeType = ASTNodeType::ENUM_CONSTRUCT; }
    EnumConstructNode(const EnumConstructNode& other);

    // The class the enum was lowered to: `Shape`, or the instance `Option$Int`.
    std::string instanceName() const;

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;
    std::unique_ptr<Type> getType() override;
    std::unique_ptr<ASTNode> clone() override { return std::make_unique<EnumConstructNode>(*this); }
};

// A `case` pattern after the type checker resolved it against the matched value's type.
class PatternNode : public ASTNode {
public:
    enum class Kind { Wildcard, Binding, Literal, Variant };
    Kind kind = Kind::Wildcard;
    std::string name;                    // Binding: the bound variable
    std::unique_ptr<ASTNode> literal;    // Literal: compared with ==, or a regex literal
    // Variant:
    std::string enumInstance;            // class the enum value has (`Shape`, `Option$Int`)
    std::string variantName;
    int tag = 0;
    std::vector<std::string> fieldNames;               // struct fields holding the payload
    std::vector<std::unique_ptr<Type>> fieldTypes;
    std::vector<std::unique_ptr<PatternNode>> subpatterns;  // one per payload field
    std::unique_ptr<Type> valueType;     // type of the value this pattern matches

    PatternNode() { nodeType = ASTNodeType::PATTERN; }
    PatternNode(const PatternNode& other);

    // Variables bound by this pattern, with their types.
    void collectBindings(std::vector<std::pair<std::string, const Type*>>& out) const;
    bool isIrrefutable() const { return kind == Kind::Wildcard || kind == Kind::Binding; }

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override { return nullptr; }
    std::unique_ptr<Type> getType() override {
        return valueType ? valueType->clone() : std::make_unique<UnknownType>();
    }
    std::unique_ptr<ASTNode> clone() override { return std::make_unique<PatternNode>(*this); }
};

// `expr?`: the payload of an Option/Result, or an early return of its None/Err from the function.
class TryNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> expression;
    std::unique_ptr<Type> functionReturnType;  // set by the type checker

    TryNode() { nodeType = ASTNodeType::TRY_EXPRESSION; }
    TryNode(const TryNode& other);

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;
    std::unique_ptr<Type> getType() override;
    std::unique_ptr<ASTNode> clone() override { return std::make_unique<TryNode>(*this); }
};
