#pragma once

class ProgramNode;
class AttributeNode;
class ImportNode;
class ClassDeclarationNode;
class ObjectDeclarationNode;
class FunctionDeclarationNode;
class ParameterNode;
class BlockNode;
class ExpressionNode;
class VariableDeclarationNode;
class CallExpressionNode;
class MethodCallNode;
class IfStatementNode;
class ForStatementNode;
class WhileStatementNode;
class MapStatementNode;
class PMapStatementNode;
class ReduceStatementNode;
class FilterStatementNode;
class LambdaExpressionNode;
class MatchExpressionNode;
class RangeExpressionNode;
class BinaryExpressionNode;
class AssignmentNode;
class ClassBodyNode;
class ReturnStatementNode;
class ClassInstanceCreationNode;
class ArrayCreationExpressionNode;
class AssignmentExpressionNode;
class UnaryExpressionNode;
class CastingExpressionNode;
class PostfixExpressionNode;
class PrefixExpressionNode;
class ArrayLiteralNode;
class DictLiteralNode;
class TupleExpressionNode;
class DestructuringAssignmentNode;
class UnitNode;
class IdentifierNode;
class ValueNode;
class FunctionCallNode;
class ArrayCreationNode;
class ArrayAccessNode;
class AccessFieldNode;
class TemplateDeclarationNode;
class LoopControlNode;
class EnumDeclarationNode;
class EnumConstructNode;
class PatternNode;
class TryNode;
class InterfaceDeclarationNode;
class InterfaceWrapNode;

class ASTVisitor {
public:
    virtual void visit(ProgramNode& node) = 0;

    virtual void visit(AttributeNode& node) = 0;
    virtual void visit(ImportNode& node) = 0;
    virtual void visit(ClassDeclarationNode& node) = 0;
    virtual void visit(ObjectDeclarationNode& node) = 0;
    virtual void visit(FunctionDeclarationNode& node) = 0;

    virtual void visit(ParameterNode& node) = 0;
    virtual void visit(BlockNode& node) = 0;
    virtual void visit(ExpressionNode& node) = 0;

    // 오브젝트 멤버 변수 방문
    virtual void visit(VariableDeclarationNode& node) = 0;
    virtual void visit(CallExpressionNode& node) = 0;
    virtual void visit(MethodCallNode& node) = 0;
    virtual void visit(IfStatementNode& node) = 0;
    virtual void visit(ForStatementNode& node) = 0;
	virtual void visit(WhileStatementNode& node) = 0;
    virtual void visit(MapStatementNode& node) = 0;
    virtual void visit(PMapStatementNode& node) = 0;
    virtual void visit(ReduceStatementNode& node) = 0;
    virtual void visit(FilterStatementNode& node) = 0;
    virtual void visit(LambdaExpressionNode& node) = 0;
    virtual void visit(MatchExpressionNode& node) = 0;
    virtual void visit(RangeExpressionNode& node) = 0;
    virtual void visit(BinaryExpressionNode& node) = 0;
    virtual void visit(AssignmentNode& node) = 0;
    virtual void visit(ClassBodyNode& node) = 0;
    virtual void visit(ReturnStatementNode& node) = 0;
    virtual void visit(ClassInstanceCreationNode& node) = 0;
    virtual void visit(ArrayCreationExpressionNode& node) = 0;
    virtual void visit(AssignmentExpressionNode& node) = 0;
    virtual void visit(UnaryExpressionNode& node) = 0;
    virtual void visit(CastingExpressionNode& node) = 0;
    virtual void visit(PostfixExpressionNode& node) = 0;
    virtual void visit(PrefixExpressionNode& node) = 0;
	virtual void visit(ArrayLiteralNode& node) = 0;
	virtual void visit(DictLiteralNode& node) = 0;
	virtual void visit(TupleExpressionNode& node) = 0;
	virtual void visit(DestructuringAssignmentNode& node) = 0;

    virtual void visit(UnitNode& node) = 0;
    virtual void visit(IdentifierNode& node) = 0;
    virtual void visit(ValueNode& node) = 0;
    virtual void visit(FunctionCallNode& node) = 0;
    virtual void visit(ArrayCreationNode& node) = 0;
    virtual void visit(ArrayAccessNode& node) = 0;
    virtual void visit(AccessFieldNode& node) = 0;
    virtual void visit(TemplateDeclarationNode& node) = 0;
    virtual void visit(LoopControlNode&) {}
    virtual void visit(EnumDeclarationNode&) {}
    virtual void visit(EnumConstructNode&) {}
    virtual void visit(PatternNode&) {}
    virtual void visit(TryNode&) {}
    virtual void visit(InterfaceDeclarationNode&) {}
    virtual void visit(InterfaceWrapNode&) {}
};