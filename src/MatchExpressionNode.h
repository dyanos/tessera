#pragma once
#include "ast.h"


// `value match { case p1 | p2 if guard => result; ... }`
//
// cases[i] = (first pattern, result). alternatives[i] holds the further `|` patterns of case i and
// guards[i] its `if` condition (null when absent); both vectors are kept parallel to `cases`.
// The type checker resolves raw patterns (parsed as expressions) into PatternNode.
class MatchExpressionNode : public ASTNode {
public:
    MatchExpressionNode() {
        nodeType = ASTNodeType::MATCH_EXPRESSION;
    }
    MatchExpressionNode(std::unique_ptr<ASTNode> expr,
        const std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>& caseList) {
        nodeType = ASTNodeType::MATCH_EXPRESSION;
        expression = std::move(expr);
        for (const auto& casePair : caseList) {
            cases.push_back({ casePair.first->clone(), casePair.second->clone()});
        }
        normalizeCaseData();
    }
    MatchExpressionNode(const MatchExpressionNode& other) {
        copyFrom(other);
    }
    MatchExpressionNode& operator=(const MatchExpressionNode& other) {
        if (this != &other) {
            cases.clear();
            guards.clear();
            alternatives.clear();
            copyFrom(other);
        }
        return *this;
    }

    std::unique_ptr<ASTNode> expression;
    std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>> cases;
    std::vector<std::unique_ptr<ASTNode>> guards;
    std::vector<std::vector<std::unique_ptr<ASTNode>>> alternatives;
    bool patternsResolved = false;  // set once the type checker turned the patterns into PatternNode

    // Keeps `guards` and `alternatives` the same length as `cases`.
    void normalizeCaseData() {
        guards.resize(cases.size());
        alternatives.resize(cases.size());
    }

    void accept(ASTVisitor& visitor) override;
    llvm::Value* codegen() override;
    llvm::Value* codegenResolved();

    std::unique_ptr<Type> getType() override {
        if (type) {
            return type->clone();
        }

        for (const auto& casePair : cases) {
            if (casePair.second) {
                auto caseType = casePair.second->getType();
                if (caseType) {
                    return caseType->clone();
                }
            }
        }

        return std::make_unique<UnknownType>();
    }
    std::unique_ptr<ASTNode> clone() override {
        return std::make_unique<MatchExpressionNode>(*this);
    }

private:
    void copyFrom(const MatchExpressionNode& other) {
        nodeType = ASTNodeType::MATCH_EXPRESSION;
        line = other.line;
        type = other.type ? other.type->clone() : nullptr;
        expression = other.expression ? other.expression->clone() : nullptr;
        patternsResolved = other.patternsResolved;
        for (const auto& casePair : other.cases) {
            cases.push_back({ casePair.first ? casePair.first->clone() : nullptr, casePair.second ? casePair.second->clone() : nullptr });
        }
        for (const auto& guard : other.guards) guards.push_back(guard ? guard->clone() : nullptr);
        for (const auto& list : other.alternatives) {
            alternatives.emplace_back();
            for (const auto& p : list) alternatives.back().push_back(p ? p->clone() : nullptr);
        }
        normalizeCaseData();
    }
};
