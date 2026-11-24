// type_checker.h

#pragma once

#include "ast.h"

#include "ASTVisitor.h"
#include "enum_nodes.h"
#include "default_arguments.h"
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// Opt-in strict-ownership mode (M2). Off by default so the existing corpus is unaffected; when
// enabled (via `--strict-ownership`) move-checking generalizes from `box` to every non-copy owned
// type (classes, arrays, String, owned container/smart-pointer generics). See docs/memory-model-design.md.
void setStrictOwnership(bool enabled);
bool isStrictOwnership();
bool typeIsCopy(const Type* type);
struct Symbol;
struct FunctionSymbol;

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
    void narrowFloatOperands(BinaryExpressionNode& node);
    void visitBinaryExpression(BinaryExpressionNode& node);

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
	bool checkClassOperatorOperand(const std::string& op, const Type* leftType, const Type* rightType);
	void checkDictKey(const Type* dictType, ASTNode* keyExpr);
	void checkDictValue(const Type* valueType, ASTNode* valueExpr, const std::string& context);
	static std::string typeDisplayName(const Type* type);
	void visit(TupleExpressionNode& node) override;
	void visit(DestructuringAssignmentNode& node) override;
	void visit(WhileStatementNode& node) override;
	void visit(TemplateDeclarationNode& node) override;
	void visit(LoopControlNode& node) override;
	void visit(EnumDeclarationNode& node) override;
	void visit(EnumConstructNode& node) override;
	void visit(TryNode& node) override;
	// Visits `node` with `expected` as the type its context wants (lets `None`, `Ok(1)` and other
	// generic enum values take their type arguments from a declaration, parameter or return type).
	void visitExpecting(ASTNode* node, const Type* expected);
	std::unique_ptr<PatternNode> resolvePattern(ASTNode* raw, const Type* valueType);
	// A class value used where an interface it implements is expected becomes an interface value.
	void coerceToInterface(std::unique_ptr<ASTNode>& slot, const Type* target);
	// Template arguments must implement the interfaces their parameter is bounded by.
	void checkTemplateBounds(const std::string& templateName, const std::vector<const Type*>& arguments);
	// Default and named arguments of a call whose target is now known (default_arguments.cpp).
	bool expandArguments(std::vector<std::unique_ptr<ASTNode>>& args, CallTarget kind, const std::string& owner,
	                     const std::string& name, int line, ASTNode* receiver = nullptr);
	CallReport callReport();

private:
    struct Borrow {
        size_t scopeDepth;
        bool isMutable;
        const ASTNode* expression;
    };

    struct OwnershipState {
        bool moved = false;
        bool owned = false;
        std::vector<Borrow> borrows;
        bool mutableBinding = false;
        bool addressExposed = false;
        const Symbol* shapeBinding = nullptr;
        std::shared_ptr<const Type> tensorShape;
        std::shared_ptr<const Type> integerValue;
        const ASTNode* tensorLifetime = nullptr;
    };

    int errorCount = 0;
    std::set<std::string> reportedCallErrors;  // callReport(): each message (with its location) once
    int unsafeContextDepth = 0;
    int loopDepth = 0;
    const Type* currentReturnType = nullptr;  // declared return type of the function being checked
    const Type* requiredReturnShape = nullptr;
    const Type* expectedType = nullptr;       // see visitExpecting
    int currentLine = 0;                      // line of the statement being checked
    const Type* blockResultExpected = nullptr;  // expected type of the next block's last statement  // enclosing while/for loops of the current function (break/continue targets)
    // True while checking the body of an `async def`. `await` is only valid here, and the M8
    // no-borrow-across-await rule is enforced at each await point.
    bool inAsyncFunction = false;
    std::vector<std::unordered_map<std::string, OwnershipState>> ownershipScopes;
    std::unordered_map<const ASTNode*, std::shared_ptr<const Type>> tensorShapes;
    std::unordered_map<const ASTNode*, const ASTNode*> tensorLifetimeUses;
    std::unordered_map<const ASTNode*, const FunctionSymbol*> tensorBorrowCalls;
    std::unordered_map<const FunctionSymbol*, const FunctionDeclarationNode*> tensorBorrowTargets;
    std::set<const ASTNode*> tensorLifetimeOperators;
    std::set<const ASTNode*> unsafeTensorLifetimes;
    const ClassInstanceCreationNode* allowedStackTensor = nullptr;
    bool checkingProgram = false;
    void checkTensorLifetimes(ASTNode& root);
    size_t nextIntegerValue = 0;
    size_t shapeScopeStart = 0;
    std::shared_ptr<const Type> provenTensor(const ASTNode* expression) const;
    void rememberTensor(const ASTNode* expression, std::shared_ptr<const Type> shape);
    std::shared_ptr<const Type> freshIntegerValue(const std::string& name);
    void setShapeBinding(const std::string& name, const Type* type, ASTNode* initializer, bool mutableBinding);
    void changeShapeBinding(const std::string& name, ASTNode* value, bool expose = false);
    void forgetMutableShapes(bool expose = false);
    OwnershipState* findShapeBinding(const std::string& name);
    void checkTensorConstructor(ClassInstanceCreationNode& node);
    void checkTensorReturn(ASTNode* expression);
    void checkTensorCall(FunctionCallNode& node, const FunctionType& signature);
    std::vector<std::vector<std::vector<std::unordered_map<std::string, OwnershipState>>>> loopExitStates;
    void visitLoopBody(ASTNode* body, size_t outerScopes);
    void reportError(const std::string& message);
    void checkTypeResolved(const std::unique_ptr<Type>& type, const std::string& context);
    void enterOwnershipScope();
    void exitOwnershipScope();
    void declareOwnership(const std::string& name, const std::unique_ptr<Type>& type);
    OwnershipState* findOwnership(const std::string& name);
    void checkIdentifierUse(const std::string& name);
    void markMoved(const std::string& name);
    bool isFieldPath(ASTNode* node);
    void checkOwnedValueUse(ASTNode* node, const std::string& context);
    void markBorrowed(const std::string& name, bool isMutableBorrow, const ASTNode* expression = nullptr);
    void releaseBorrow(ASTNode* expression);
    // Path-sensitive join (M3/M4): OR the `moved` flags from `source` into the current ownership
    // state (per scope, by name). Used to merge the fall-through paths of an `if` so moves made only
    // on a diverging branch are not counted after it.
    void mergeMovedFrom(const std::vector<std::unordered_map<std::string, OwnershipState>>& source);
    void checkFunctionArguments(const std::vector<std::unique_ptr<ASTNode>>& arguments,
                                const std::vector<std::unique_ptr<Type>>& parameterTypes,
                                const std::string& callName);
};
