#pragma once
#include "ast.h"

#include <string>
#include <vector>
#include <memory>

struct TemplateParam {
    bool isType;           // true: typename, false: non-type
    std::string name;      // "T" or "N"
    std::vector<std::string> bounds;  // `T: Shape + Show`: interfaces the type argument must implement
    std::unique_ptr<Type> valueType;  // non-type일 때 "Int" 등

    TemplateParam() : isType(true) {}
    TemplateParam(const std::string& name) : isType(true), name(name) {}
    TemplateParam(const std::string& name, std::unique_ptr<Type> vt)
        : isType(false), name(name), valueType(std::move(vt)) {}
    TemplateParam(const TemplateParam& other)
        : isType(other.isType), name(other.name), bounds(other.bounds),
          valueType(other.valueType ? other.valueType->clone() : nullptr) {}
    TemplateParam& operator=(const TemplateParam& other) {
        if (this != &other) {
            isType = other.isType;
            name = other.name;
            bounds = other.bounds;
            valueType = other.valueType ? other.valueType->clone() : nullptr;
        }
        return *this;
    }
};

class TemplateDeclarationNode : public ASTNode {
public:
    TemplateDeclarationNode() {
        nodeType = ASTNodeType::TEMPLATE_DECLARATION;
    }
    TemplateDeclarationNode(const TemplateDeclarationNode& other) {
        nodeType = ASTNodeType::TEMPLATE_DECLARATION;
        typeParameters = other.typeParameters;
        for (auto& p : other.templateParams) templateParams.push_back(TemplateParam(p));
        declaration = other.declaration ? other.declaration->clone() : nullptr;
    }
    TemplateDeclarationNode& operator=(const TemplateDeclarationNode& other) {
        if (this != &other) {
            nodeType = ASTNodeType::TEMPLATE_DECLARATION;
            typeParameters = other.typeParameters;
            templateParams.clear();
            for (auto& p : other.templateParams) templateParams.push_back(TemplateParam(p));
            declaration = other.declaration ? other.declaration->clone() : nullptr;
        }
        return *this;
    }

    std::vector<std::string> typeParameters;       // e.g. ["T", "U"] - backward compat
    std::vector<TemplateParam> templateParams;     // full param info including non-type
    std::unique_ptr<ASTNode> declaration;          // FunctionDeclarationNode or ClassDeclarationNode

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;

    std::unique_ptr<Type> getType() override {
        return std::make_unique<UnknownType>();
    }
    std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<TemplateDeclarationNode>(*this);
    }
};
