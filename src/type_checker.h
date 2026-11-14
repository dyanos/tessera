// type_checker.h

#pragma once

#include "ast.h"

#include "ASTVisitor.h"
#include <string>
#include <unordered_map>
#include <vector>

// Opt-in strict-ownership mode (M2). Off by default so the existing corpus is unaffected; when
// enabled (via `--strict-ownership`) move-checking generalizes from `box` to every non-copy owned
// type (classes, arrays, String, owned container/smart-pointer generics). See docs/memory-model-design.md.
void setStrictOwnership(bool enabled);
bool isStrictOwnership();

// TypeChecker 클래스는 AST를 방문하여 타입 검사를 수행합니다.
class TypeChecker : public ASTVisitor {
public:
    bool hasErrors() const { return errorCount > 0; }
    int getErrorCount() const { return errorCount; }

    // 프로그램 노드를 방문합니다.
    void visit(ProgramNode& node) override;

    // 임포트 노드를 방문합니다.
    void visit(ImportNode& node) override;

    // 클래스 선언 노드를 방문합니다.
    void visit(ClassDeclarationNode& node) override;

    // 객체 선언 노드를 방문합니다.
    void visit(ObjectDeclarationNode& node) override;

    // 함수 선언 노드를 방문합니다.
    void visit(FunctionDeclarationNode& node) override;

    // 파라미터 노드를 방문합니다.
    void visit(ParameterNode& node) override;

    // 블록 노드를 방문합니다.
    void visit(BlockNode& node) override;

    // 표현식 노드를 방문합니다.
    void visit(ExpressionNode& node) override;

    // 변수 선언 노드를 방문합니다.
    void visit(VariableDeclarationNode& node) override;

    // 호출 표현식 노드를 방문합니다.
    void visit(CallExpressionNode& node) override;

	void visit(MethodCallNode& node) override;

    // if 문 노드를 방문합니다.
    void visit(IfStatementNode& node) override;

    // for 문 노드를 방문합니다.
    void visit(ForStatementNode& node) override;

    // match 표현식 노드를 방문합니다.
    void visit(MatchExpressionNode& node) override;

    // 범위 표현식 노드를 방문합니다.
    void visit(RangeExpressionNode& node) override;

    // 이항 표현식 노드를 방문합니다.
    void visit(BinaryExpressionNode& node) override;

    // 할당 노드를 방문합니다.
    void visit(AssignmentNode& node) override;

    // 클래스 본문 노드를 방문합니다.
    void visit(ClassBodyNode& node) override;

    // 반환 문 노드를 방문합니다.
    void visit(ReturnStatementNode& node) override;

    // 새로운 호출 표현식 노드를 방문합니다.
    void visit(ClassInstanceCreationNode& node) override;

    // 새로운 배열 표현식 노드를 방문합니다.
    void visit(ArrayCreationExpressionNode& node) override;

    // 할당 표현식 노드를 방문합니다.
    void visit(AssignmentExpressionNode& node) override;

    // 단항 표현식 노드를 방문합니다.
    void visit(UnaryExpressionNode& node) override;
    void visit(CastingExpressionNode& node) override;

    // 후위 표현식 노드를 방문합니다.
    void visit(PostfixExpressionNode& node) override;

    // 전위 표현식 노드를 방문합니다.
    void visit(PrefixExpressionNode& node) override;

    // 단위 노드를 방문합니다.
    void visit(UnitNode& node) override;

    // 식별자 노드를 방문합니다.
    void visit(IdentifierNode& node) override;

    // 값 노드를 방문합니다.
    void visit(ValueNode& node) override;

	void visit(FunctionCallNode& node) override;

	void visit(ArrayAccessNode& node) override;
	void visit(ArrayCreationNode& node) override;

	void visit(AccessFieldNode& node) override;
	void visit(AttributeNode& node) override;

	void visit(LambdaExpressionNode& node) override;
    void visit(MapStatementNode& node) override;
	void visit(PMapStatementNode& node) override;
	void visit(ReduceStatementNode& node) override;
	void visit(FilterStatementNode& node) override;
	void visit(ArrayLiteralNode& node) override;
	void visit(DictLiteralNode& node) override;
	void checkContractClause(ContractClause& clause, const std::string& kind, bool boolean);
	void visitLoopContracts(ASTNode& loop);
	void checkDictKey(const Type* dictType, ASTNode* keyExpr);
	void checkDictValue(const Type* valueType, ASTNode* valueExpr, const std::string& context);
	static std::string typeDisplayName(const Type* type);
	void visit(TupleExpressionNode& node) override;
	void visit(DestructuringAssignmentNode& node) override;
	void visit(WhileStatementNode& node) override;
	void visit(TemplateDeclarationNode& node) override;

private:
    struct OwnershipState {
        bool moved = false;
        int immutableBorrows = 0;
        bool mutableBorrowed = false;
        bool owned = false;
    };

    int errorCount = 0;
    int unsafeContextDepth = 0;
    // True while checking the body of an `async def`. `await` is only valid here, and the M8
    // no-borrow-across-await rule is enforced at each await point.
    bool inAsyncFunction = false;
    std::vector<std::unordered_map<std::string, OwnershipState>> ownershipScopes;
    void reportError(const std::string& message);
    void checkTypeResolved(const std::unique_ptr<Type>& type, const std::string& context);
    void enterOwnershipScope();
    void exitOwnershipScope();
    void declareOwnership(const std::string& name, const std::unique_ptr<Type>& type);
    OwnershipState* findOwnership(const std::string& name);
    void checkIdentifierUse(const std::string& name);
    void markMoved(const std::string& name);
    void markBorrowed(const std::string& name, bool isMutableBorrow);
    void releaseBorrow(const std::string& name, bool isMutableBorrow);
    // Path-sensitive join (M3/M4): OR the `moved` flags from `source` into the current ownership
    // state (per scope, by name). Used to merge the fall-through paths of an `if` so moves made only
    // on a diverging branch are not counted after it.
    void mergeMovedFrom(const std::vector<std::unordered_map<std::string, OwnershipState>>& source);
    void checkFunctionArguments(const std::vector<std::unique_ptr<ASTNode>>& arguments,
                                const std::vector<std::unique_ptr<Type>>& parameterTypes,
                                const std::string& callName);
};
