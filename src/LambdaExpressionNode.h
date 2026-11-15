#pragma once
#include "ast.h"

class LambdaExpressionNode : public ASTNode {
public:
    LambdaExpressionNode() {
        nodeType = ASTNodeType::EXPRESSION;
    }
    LambdaExpressionNode(const LambdaExpressionNode& other) {
        this->capturesByReference = other.capturesByReference;
        this->escapes = other.escapes;
        for (const auto& v : other.captureVariables) {
            this->captureVariables.push_back(v);
        }
        for (const auto& arg : other.arguments) {
            this->arguments.push_back(arg ? arg->clone() : nullptr);
        }
        this->body = other.body ? other.body->clone() : nullptr;
        nodeType = ASTNodeType::EXPRESSION;
    }
    LambdaExpressionNode& operator=(const LambdaExpressionNode& other) {
        if (this != &other) {
            capturesByReference = other.capturesByReference;
            escapes = other.escapes;
            captureVariables = other.captureVariables;
            arguments.clear();
            arguments.reserve(other.arguments.size());
            for (const auto& arg : other.arguments) {
                arguments.push_back(arg ? arg->clone() : nullptr);
            }
            body = other.body ? other.body->clone() : nullptr;
            nodeType = ASTNodeType::EXPRESSION;
        }
        return *this;
    }

    bool capturesByReference = false;
    // The closure outlives the frame that creates it (it is returned or stored in a value), so its
    // environment is allocated on the heap. Set by markEscapingLambdas.
    bool escapes = false;
    std::vector<std::string> captureVariables;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    std::unique_ptr<ASTNode> body;
    // The captured environment's LLVM type, recorded by codegen(). spawn()/threadStart() snapshot that
    // many bytes into the new task. The size is emitted as a sizeof constant expression so it follows
    // the final target data layout (the module has no data layout yet during codegen).
    llvm::Type* environmentType = nullptr;

    void accept(ASTVisitor& visitor) override;

    llvm::Value* codegen() override;
    std::unique_ptr<Type> getType() override {
        std::vector<std::unique_ptr<Type>> parameterTypes;
        for (const auto& arg : arguments) {
            if (!arg) {
                parameterTypes.push_back(std::make_unique<UnknownType>());
                continue;
            }
            auto argType = arg->getType();
            if (!argType || argType->getKind() == Type::Kind::UNKNOWN) {
                parameterTypes.push_back(std::make_unique<BasicType>("Int"));
            }
            else {
                parameterTypes.push_back(argType->clone());
            }
        }
        auto returnType = body ? body->getType() : nullptr;
        if (!returnType || returnType->getKind() == Type::Kind::UNKNOWN) {
            returnType = std::make_unique<BasicType>("Int");
        }
        return std::make_unique<FunctionType>(parameterTypes, returnType);
    }
    std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<LambdaExpressionNode>(*this);
    }
};

// Marks lambda literals that outlive their creating frame (returned, directly or through a local;
// stored into an object, enum value, array or field). Reports a `[&]` lambda that escapes, whose
// references would dangle. Returns false on such an error.
bool markEscapingLambdas(ASTNode* program);
