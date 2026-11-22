#pragma once
#include "ast.h"

#include "ParameterNode.h"
#include "BlockNode.h"

class CodeGenerator;
void markOwnership(CodeGenerator& cg, const std::string& functionName, const Type* returnType,
                   const std::vector<std::unique_ptr<ASTNode>>& parameters, bool hasReceiver,
                   const std::string& className, const std::string& methodName);

class FunctionDeclarationNode : public ASTNode {
public:
    FunctionDeclarationNode() {
        nodeType = ASTNodeType::FUNCTION_DECLARATION;
        name = "";
        parameters.clear();
        returnType = nullptr;
        body = nullptr;
    }
    FunctionDeclarationNode(const std::string& name,
        const std::vector<std::unique_ptr<ParameterNode>>& parameters,
        std::unique_ptr<Type> returnType,
        std::unique_ptr<BlockNode> body)
        : name(name) {
        nodeType = ASTNodeType::FUNCTION_DECLARATION;
        this->returnType = std::move(returnType);
        this->body = std::move(body);
        for (const auto& param : parameters) {
            this->parameters.push_back(param->clone());
        }
    }
    FunctionDeclarationNode(const FunctionDeclarationNode& other) {
        nodeType = ASTNodeType::FUNCTION_DECLARATION;
        name = other.name;
        for (const auto& param : other.parameters) {
            parameters.push_back(param ? param->clone() : nullptr);
        }
        returnType = other.returnType ? other.returnType->clone() : nullptr;
        body = other.body ? std::make_unique<BlockNode>(*other.body) : nullptr;
        isExternal = other.isExternal;
        isConstexpr = other.isConstexpr;
        isUnsafe = other.isUnsafe;
        isAsync = other.isAsync;
        isIntrinsic = other.isIntrinsic;
        isOverride = other.isOverride;
        isSpecification = other.isSpecification;
        isLemma = other.isLemma;
        dllImportLibrary = other.dllImportLibrary;
        externalSymbolName = other.externalSymbolName;
    }
    FunctionDeclarationNode& operator=(const FunctionDeclarationNode& other) {
        if (this != &other) {
            nodeType = ASTNodeType::FUNCTION_DECLARATION;
            name = other.name;
            parameters.clear();
            parameters.reserve(other.parameters.size());
            for (const auto& param : other.parameters) {
                parameters.push_back(param ? param->clone() : nullptr);
            }
            returnType = other.returnType ? other.returnType->clone() : nullptr;
            body = other.body ? std::make_unique<BlockNode>(*other.body) : nullptr;
            isExternal = other.isExternal;
            isConstexpr = other.isConstexpr;
            isUnsafe = other.isUnsafe;
            isOverride = other.isOverride;
            isSpecification = other.isSpecification;
            isLemma = other.isLemma;
            dllImportLibrary = other.dllImportLibrary;
            externalSymbolName = other.externalSymbolName;
        }
        return *this;
    }

    bool isExternal = false;
    bool isSpecification = false;
    bool isLemma = false;
    bool isConstexpr = false;
    bool isUnsafe = false;
    bool isAsync = false;
    // Declared with [@Intrinsic]: names a compiler intrinsic; emits no code (see intrinsics.h).
    bool isIntrinsic = false;
    bool isOverride = false;
    std::string dllImportLibrary;
    std::string externalSymbolName;

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<Type> returnType;
    std::unique_ptr<BlockNode> body;

    void accept(ASTVisitor& visitor) override;

    llvm::Value* codegen() override;

    // Creates the LLVM function prototype and registers its symbol without emitting the body.
    // Idempotent: reuses an existing prototype of the same mangled name. Running this for every
    // top-level function before any body is emitted lets a function call another that is declared
    // later in the file (forward references, mutual recursion).
    llvm::Function* declarePrototype();
    std::unique_ptr<Type> getType() override {
        auto ft = std::make_unique<FunctionType>();
        ft->returnType = returnType ? returnType->clone() : std::make_unique<UnknownType>();
        for (auto& param : parameters) {
            ft->parameterTypes.push_back(param->getType() ? param->getType()->clone() : std::make_unique<UnknownType>());
        }
        return ft;
    }
    std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<FunctionDeclarationNode>(*this);
    }
};
