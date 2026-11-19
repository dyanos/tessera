#pragma once
#include "ast.h"


class ClassInstanceCreationNode : public ASTNode {
public:
    std::string className;
    bool defaultValue = false;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    std::vector<std::unique_ptr<Type>> templateArgs;  // e.g. new Stack<Int>()

    ClassInstanceCreationNode() {
        nodeType = ASTNodeType::CLASS_INSTANCE_CREATION;
        arguments.clear();
        className = "";
    }
    ClassInstanceCreationNode(const std::string& className, std::vector<std::unique_ptr<ASTNode>> arguments) {
        this->className = className;
        for (const auto& arg : arguments) {
            this->arguments.push_back(arg ? arg->clone() : nullptr);
        }
        nodeType = ASTNodeType::CLASS_INSTANCE_CREATION;
    }
    ClassInstanceCreationNode(const std::string& className) {
        this->className = className;
        arguments.clear();
        nodeType = ASTNodeType::CLASS_INSTANCE_CREATION;
    }
    ClassInstanceCreationNode(const ClassInstanceCreationNode& other) {
        nodeType = other.nodeType;
        className = other.className;
        defaultValue = other.defaultValue;
        for (const auto& arg : other.arguments) {
            arguments.push_back(arg ? arg->clone() : nullptr);
        }
        for (const auto& t : other.templateArgs) {
            templateArgs.push_back(t ? t->clone() : nullptr);
        }
    }
    ClassInstanceCreationNode& operator=(const ClassInstanceCreationNode& other) {
        if (this != &other) {
            nodeType = other.nodeType;
            className = other.className;
            defaultValue = other.defaultValue;
            arguments.clear();
            arguments.reserve(other.arguments.size());
            for (const auto& arg : other.arguments) {
                arguments.push_back(arg ? arg->clone() : nullptr);
            }
            templateArgs.clear();
            for (const auto& t : other.templateArgs) {
                templateArgs.push_back(t ? t->clone() : nullptr);
            }
        }
        return *this;
    }

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;
    std::unique_ptr<Type> getType() override;
    std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<ClassInstanceCreationNode>(*this);
    }
};
