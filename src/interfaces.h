// interfaces.h -- interfaces, `implements`, interface-typed values, and template bounds.
//
//   interface Shape {
//       def area(): Double;
//       def describe(): String = "shape with area " + area()   // default implementation
//   }
//   class Circle(r: Double) implements Shape { def area(): Double = 3.14159e0 * r * r }
//   def total(shapes: Array<Shape>): Double ...            // dynamic dispatch
//   template<typename T: Shape> def biggest(a: T, b: T): T ...   // bound checked at instantiation
//
// An interface value is a pointer to { object, vtable }: the object (a heap copy for a struct) and
// one thunk per interface method that calls the implementing class's method.
#pragma once
#include "ast.h"
#include "FunctionDeclarationNode.h"

#include <functional>
#include <set>
#include <string>
#include <vector>

class MethodCallNode;

// `interface Name { def m(...): T; ... }` as parsed; removed by lowerInterfaces.
class InterfaceDeclarationNode : public ASTNode {
public:
    std::string name;
    std::vector<std::unique_ptr<FunctionDeclarationNode>> methods;  // body == null: abstract

    InterfaceDeclarationNode() { nodeType = ASTNodeType::INTERFACE_DECLARATION; }
    InterfaceDeclarationNode(const InterfaceDeclarationNode& other);
    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override { return nullptr; }
    std::unique_ptr<Type> getType() override { return std::make_unique<UnknownType>(); }
    std::unique_ptr<ASTNode> clone() override { return std::make_unique<InterfaceDeclarationNode>(*this); }
};

// A class (or struct) value used as a value of an interface it implements.
class InterfaceWrapNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> expression;
    std::string className;
    std::string interfaceName;

    InterfaceWrapNode() { nodeType = ASTNodeType::INTERFACE_WRAP; }
    InterfaceWrapNode(const InterfaceWrapNode& other);
    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;
    std::unique_ptr<Type> getType() override { return std::make_unique<ClassType>(interfaceName); }
    std::unique_ptr<ASTNode> clone() override { return std::make_unique<InterfaceWrapNode>(*this); }
};

struct InterfaceMethod {
    std::string name;
    std::vector<std::string> parameterNames;
    std::vector<std::unique_ptr<Type>> parameterTypes;
    std::unique_ptr<Type> returnType;
};

struct InterfaceInfo {
    std::string name;
    std::vector<InterfaceMethod> methods;
    int methodIndex(const std::string& name, size_t arity) const;
};

const InterfaceInfo* findInterface(const std::string& name);
// Every method declaration of every interface (with its parameters and their defaults).
void forEachInterfaceMethod(const std::function<void(const std::string&, FunctionDeclarationNode*)>& visit);
bool isInterfaceType(const Type* type);
// Whether values of `type` may be used as `interfaceName` (the class or a parent implements it).
bool implementsInterface(const Type* type, const std::string& interfaceName);
// The `implements` lists of every class, by class name (template classes by template name).
const std::vector<std::string>* declaredInterfaces(const std::string& className);

// Checks `implements` clauses, copies default methods into the classes that do not define them, and
// removes the interface declarations. Prints diagnostics and returns false on an error.
bool lowerInterfaces(ASTNode* program);

// Code for `receiver.method(args)` on an interface value (dynamic dispatch through the vtable).
llvm::Value* codegenInterfaceCall(MethodCallNode& call);
