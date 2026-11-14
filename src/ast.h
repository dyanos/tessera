// ast.h
#pragma once
#include <string>
#include <vector>

#include "type.h"

#include <llvm/IR/Value.h>


enum class ASTNodeType {
    NONE,
    PROGRAM,
    IMPORT,
    OBJECT_DECLARATION,
    CLASS_DECLARATION,
    FUNCTION_DECLARATION,
    PARAMETER,
    BLOCK,
    EXPRESSION,
    VARIABLE_DECLARATION,
    CALL_EXPRESSION,
    METHOD_CALL,
    CLASS_INSTANCE_CREATION,
    ARRAY_CREATION_EXPRESSION,
    ASSIGNMENT_EXPRESSION,
    UNARY_EXPRESSION,
    CASTING_EXPRESSION,
    POSTFIX_EXPRESSION,
    PREFIX_EXPRESSION,
    FUNCTION_CALL,
    ARRAY_CREATION,
    ARRAY_ACCESS,
    ACCESS_FIELD,
    IF_STATEMENT,
    FOR_STATEMENT,
    MATCH_EXPRESSION,
    RANGE_EXPRESSION,
    BINARY_EXPRESSION,
    RETURN_STATEMENT,
    UNIT,
    IDENTIFIER,
    VALUE,
    ASSIGNMENT,
    CLASS_BODY,
    ATTRIBUTE,
    ARRAY_LITERAL,
    TUPLE_EXPRESSION,
    DESTRUCTURING_ASSIGNMENT,
    MAP_STATEMENT,
    PMAP_STATEMENT,
    REDUCE_STATEMENT,
    FILTER_STATEMENT,
    WHILE_STATEMENT,
    LAMBDA_EXPRESSION,
    TEMPLATE_DECLARATION,
    DICT_LITERAL,
};

class ASTVisitor;
class ASTNode;

// Specification clauses for static verification (`csec++ --verify`): `requires`/`ensures` on a
// function, `invariant`/`decreases` on a loop. They are checked by the verifier and never compiled.
struct ContractClause {
    std::unique_ptr<ASTNode> expression;
    int line = 0;
    std::string text;  // the clause as written, for diagnostics
};

struct ContractClauses {
    std::vector<ContractClause> requiresClauses;
    std::vector<ContractClause> ensuresClauses;
    std::vector<ContractClause> invariants;
    std::vector<ContractClause> decreases;
    bool empty() const {
        return requiresClauses.empty() && ensuresClauses.empty() && invariants.empty() && decreases.empty();
    }
};

class ASTNode {
public:
    ASTNodeType nodeType = ASTNodeType::NONE;
    std::unique_ptr<Type> type;
    // Source line in the import-expanded program (0 when unknown); see sourceLocationOf in main.cpp.
    int line = 0;
    std::shared_ptr<ContractClauses> contracts;

    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
    virtual llvm::Value* codegen() = 0;
    virtual std::unique_ptr<Type> getType() = 0;
    virtual std::unique_ptr<ASTNode> clone() = 0;
};
