// type_checker.cpp

#include "type_checker.h"
#include "codegen.h"
#include "symbol.h"
#include "type_utils.h"
#include "dict_support.h"

#include <iostream>
#include <set>

#include "all_ast.h"
#include "ast_walk.h"
#include "enums.h"
#include "interfaces.h"
#include "default_arguments.h"

namespace {
bool isResolvedType(const std::unique_ptr<Type>& type) {
    return type && type->getKind() != Type::Kind::UNKNOWN;
}

bool isTensorLikeType(const std::unique_ptr<Type>& type) {
    if (!type) return false;
    const std::string name = type->getName();
    return name == "Tensor" || name.rfind("Tensor$", 0) == 0;
}

bool isI64IntegerAlias(const std::unique_ptr<Type>& type) {
    if (!type) return false;
    const std::string name = type->getName();
    return name == "Long" || name == "Natural" || name == "Integer";
}

bool isIntegerTypeName(const std::unique_ptr<Type>& type) {
    if (!type) return false;
    const std::string name = type->getName();
    return name == "Byte" || name == "Short" || name == "Int" ||
           name == "Long" || name == "Natural" || name == "Integer";
}

std::unique_ptr<Type> collectionElementType(const std::unique_ptr<Type>& rawType) {
    // Auto-deref: iterating a borrowed collection (`&Vector[T]`) yields elements of T.
    auto type = stripBorrow(rawType);
    if (!type) return std::make_unique<UnknownType>();
    if (auto* arrType = dynamic_cast<ArrayType*>(type.get())) {
        return arrType->elementType ? arrType->elementType->clone() : std::make_unique<UnknownType>();
    }
    if (auto* genType = dynamic_cast<GenericType*>(type.get())) {
        const std::string base = genType->baseType ? genType->baseType->getName() : "";
        if ((base == "Array" || base == "Vector") && !genType->typeArguments.empty()) {
            return genType->typeArguments[0] ? genType->typeArguments[0]->clone() : std::make_unique<UnknownType>();
        }
    }
    return std::make_unique<UnknownType>();
}

// Vector and Array are interchangeable spellings of the same flat array type. A `new T[n]`
// value is typed Array, so a function declared to return Vector[T] (or the reverse) is not a
// type error as long as the element types agree.
bool isArrayVectorType(const std::unique_ptr<Type>& type) {
    if (!type) return false;
    if (type->getKind() == Type::Kind::ARRAY) return true;
    if (auto* genType = dynamic_cast<GenericType*>(type.get())) {
        const std::string base = genType->baseType ? genType->baseType->getName() : "";
        return base == "Array" || base == "Vector";
    }
    return false;
}

bool isArrayVectorAlias(const std::unique_ptr<Type>& a, const std::unique_ptr<Type>& b) {
    if (!isArrayVectorType(a) || !isArrayVectorType(b)) return false;
    auto elementA = collectionElementType(a);
    auto elementB = collectionElementType(b);
    if (!isResolvedType(elementA) || !isResolvedType(elementB)) return true;
    return elementA->equals(elementB) ||
        (isIntegerTypeName(elementA) && isIntegerTypeName(elementB));
}

FunctionSymbol* ensureFunctionSymbol(FunctionDeclarationNode& node) {
    auto& cg = CodeGenerator::getInstance();
    // One symbol per overload: look the function up by its exact parameter types.
    auto nodeType = node.getType();
    FunctionSymbol* existing = nullptr;
    if (auto* functionType = dynamic_cast<FunctionType*>(nodeType.get())) {
        existing = cg.symbolTable.lookupFunctionExact(node.name, functionType->parameterTypes);
    } else {
        existing = cg.symbolTable.lookupFunction(node.name, {});
    }
    if (existing) {
        return existing;
    }

    auto symbol = std::make_unique<FunctionSymbol>(node.name, std::move(nodeType), nullptr, false, SymbolType::FUNCTION);
    auto* raw = symbol.get();
    if (!cg.symbolTable.addSymbol(node.name, std::move(symbol))) {
        cg.reportError("cannot declare function '" + node.name + "' here");
    }
    return raw;
}

TemplateSymbol* ensureTemplateSymbol(TemplateDeclarationNode& node) {
    auto& cg = CodeGenerator::getInstance();

    std::string symbolName;
    if (auto* functionDecl = dynamic_cast<FunctionDeclarationNode*>(node.declaration.get())) {
        symbolName = functionDecl->name;
    }
    else if (auto* classDecl = dynamic_cast<ClassDeclarationNode*>(node.declaration.get())) {
        symbolName = classDecl->name;
    }
    else {
        return nullptr;
    }

    auto* existing = cg.symbolTable.lookup(symbolName);
    if (existing && existing->symbolType == SymbolType::TEMPLATE) {
        auto* templateSymbol = dynamic_cast<TemplateSymbol*>(existing);
        if (templateSymbol) {
            templateSymbol->typeParameters = node.typeParameters;
            templateSymbol->templateParams.clear();
            for (const auto& param : node.templateParams) {
                templateSymbol->templateParams.push_back(TemplateParam(param));
            }
            templateSymbol->declaration = node.declaration ? node.declaration->clone() : nullptr;
        }
        return templateSymbol;
    }

    auto symbol = std::make_unique<TemplateSymbol>();
    symbol->name = symbolName;
    symbol->typeParameters = node.typeParameters;
    for (const auto& param : node.templateParams) {
        symbol->templateParams.push_back(TemplateParam(param));
    }
    symbol->declaration = node.declaration ? node.declaration->clone() : nullptr;

    auto* raw = symbol.get();
    if (!cg.symbolTable.addSymbol(symbolName, std::move(symbol))) {
        cg.reportError("cannot declare template '" + symbolName + "' here");
    }
    return raw;
}

ClassSymbol* ensureClassSymbol(ClassDeclarationNode& node) {
    auto& cg = CodeGenerator::getInstance();
    auto* existing = cg.symbolTable.lookupClass(node.name);
    if (existing) {
        existing->isStruct = node.isStruct;
        return existing;
    }

    auto* classType = llvm::StructType::create(cg.context, node.name);
    auto symbol = std::make_unique<ClassSymbol>(node.name, classType, node.superClassName);
    symbol->isStruct = node.isStruct;
    auto* raw = symbol.get();
    if (!cg.symbolTable.addSymbol(node.name, std::move(symbol))) {
        cg.reportError("cannot declare class '" + node.name + "' here");
    }
    return raw;
}

void bindVariable(const std::string& name, const std::unique_ptr<Type>& type, bool isMutable, SymbolType symbolType = SymbolType::VARIABLE) {
    auto& cg = CodeGenerator::getInstance();
    cg.symbolTable.addSymbol(name, std::make_unique<Symbol>(name, type ? type->clone() : std::make_unique<UnknownType>(), nullptr, isMutable, symbolType));
}

bool isOwnedMoveCheckedType(const Type* type) {
    return type && type->getKind() == Type::Kind::BOX;
}

// Strict-ownership mode (M2) is now ON by default: `=`/argument passing of any non-copy owned type
// (reference classes, arrays, box, owned container/smart-pointer generics) requires an explicit
// `<-` move or `&`/`&mut` borrow. `--no-strict-ownership` opts a compilation out (e.g. legacy code
// not yet migrated, such as the self-host source). The external setter/getter are defined past the
// anonymous namespace below so main.cpp can link against them; they read this flag by TU scope.
bool g_strictOwnership = true;

// A Copy type may be duplicated by a plain `=`/argument pass without an explicit `<-`/`clone`.
// Everything else is an owned (affine) value that must move. Primitives, references (a borrow is a
// copyable handle), function values, raw pointers and plain value-structs are Copy; classes,
// arrays, String, and owned container/smart-pointer generics own a resource and are not.
bool isCopyType(const Type* type) {
    if (!type) return true;
    switch (type->getKind()) {
        case Type::Kind::CLASS:
            // A `struct` is a value type (copied field-by-field, like a C struct), so it is Copy; a
            // reference `class` owns identity/resources and moves. isStructClassType consults the
            // class symbol's `isStruct` flag. Nat is an immutable arbitrary-precision number whose
            // handle is a shared value (like String), so it is Copy too -- `b = a` needs no `<-`.
            // The built-in number structs Complex/Rational/Quaternion are plain value aggregates too.
            // Enum values are immutable, so sharing one (even a heap-allocated recursive enum) is a copy.
            return isStructClassType(type) || enumOfType(type) != nullptr || isInterfaceType(type) ||
                   type->getName() == "Nat" || type->getName() == "Complex" ||
                   type->getName() == "Rational" || type->getName() == "Quaternion";
        case Type::Kind::ARRAY:
        case Type::Kind::BOX:
            return false;
        case Type::Kind::BASIC:
            // All basics are Copy, including String: csec strings are immutable/shared (literals are
            // static data), so copying a String handle is a cheap value copy with no ownership
            // transfer — the affine, move-only types are arrays, reference classes, box and the owned
            // container/smart-pointer generics.
            return true;
        case Type::Kind::GENERIC: {
            const std::string& n = type->getName();
            return !(n == "Array" || n == "Vector" || n == "Map" ||
                     n == "List" || n == "Shared" || n == "Weak" ||
                     n == "Tensor" || n.rfind("Tensor$", 0) == 0 || n == "Dict" || n == "Set");
        }
        case Type::Kind::TUPLE: {
            auto* t = dynamic_cast<const TupleType*>(type);
            if (!t) return true;
            // A tuple is Copy only when every element is Copy; one owned field taints the whole.
            for (const auto& e : t->elementTypes) {
                if (!isCopyType(e.get())) return false;
            }
            return true;
        }
        default:
            // BORROW/MUTABLE_BORROW/FUNCTION/POINTER/UNSAFE_POINTER/STRUCT/VARIABLE/UNKNOWN → Copy.
            return true;
    }
}

// The predicate the move-checker actually consults. `box` is always move-checked (baseline
// ownership, M0/M1). In strict mode every non-copy type joins it; otherwise only `box` moves.
bool isMoveCheckedType(const Type* type) {
    if (isOwnedMoveCheckedType(type)) return true;
    return g_strictOwnership && type && !isCopyType(type);
}

bool isBorrowType(const Type* type) {
    return type && (type->getKind() == Type::Kind::BORROW || type->getKind() == Type::Kind::MUTABLE_BORROW);
}

bool canInitializeOwnershipType(const Type* declaredType, const Type* initType) {
    if (!declaredType || !initType) return false;
    if (auto* boxType = dynamic_cast<const BoxType*>(declaredType)) {
        return boxType->baseType && boxType->baseType->equals(*initType);
    }
    if (auto* borrowType = dynamic_cast<const BorrowType*>(declaredType)) {
        if (initType->getKind() != declaredType->getKind()) return false;
        auto* initBorrow = dynamic_cast<const BorrowType*>(initType);
        return initBorrow && borrowType->baseType && initBorrow->baseType &&
               borrowType->baseType->equals(*initBorrow->baseType);
    }
    return false;
}

std::unique_ptr<Type> unwrappedOwnershipType(const Type* type) {
    if (!type) return std::make_unique<UnknownType>();
    if (auto* boxType = dynamic_cast<const BoxType*>(type)) {
        return boxType->baseType ? boxType->baseType->clone() : std::make_unique<UnknownType>();
    }
    if (auto* borrowType = dynamic_cast<const BorrowType*>(type)) {
        return borrowType->baseType ? borrowType->baseType->clone() : std::make_unique<UnknownType>();
    }
    if (auto* unsafePointerType = dynamic_cast<const UnsafePointerType*>(type)) {
        return unsafePointerType->baseType ? unsafePointerType->baseType->clone() : std::make_unique<UnknownType>();
    }
    return const_cast<Type*>(type)->clone();
}

std::string identifierName(ASTNode* node) {
    if (auto* id = dynamic_cast<IdentifierNode*>(node)) {
        return id->value;
    }
    return "";
}

bool isMoveExpression(ASTNode* node) {
    auto* prefix = dynamic_cast<PrefixExpressionNode*>(node);
    return prefix && prefix->op == "<-";
}

// Future<A> vs Future<B>: compare the payload types by name (GenericType::equals is identity-only).
bool sameFirstTypeArgument(const Type* left, const Type* right) {
    auto* l = dynamic_cast<const GenericType*>(left);
    auto* r = dynamic_cast<const GenericType*>(right);
    if (!l || !r || l->typeArguments.empty() || r->typeArguments.empty()) return false;
    if (!l->typeArguments[0] || !r->typeArguments[0]) return false;
    return l->typeArguments[0]->getName() == r->typeArguments[0]->getName();
}

bool isBorrowExpression(ASTNode* node, bool* isMutableBorrow = nullptr) {
    auto* prefix = dynamic_cast<PrefixExpressionNode*>(node);
    if (!prefix) return false;
    if (prefix->op == "&" || prefix->op == "&mut") {
        if (isMutableBorrow) *isMutableBorrow = prefix->op == "&mut";
        return true;
    }
    return false;
}

std::string movedOrBorrowedIdentifier(ASTNode* node) {
    if (auto* prefix = dynamic_cast<PrefixExpressionNode*>(node)) {
        return identifierName(prefix->expression.get());
    }
    return identifierName(node);
}

// A statement "diverges" when control can never fall through past it — every path returns. Moves
// performed on a diverging path never reach the code after it, so the ownership checker must not let
// them persist (memory model M3/M4: path-sensitive handling of conditionals).
bool statementDiverges(const ASTNode* stmt) {
    if (!stmt) return false;
    if (dynamic_cast<const ReturnStatementNode*>(stmt)) {
        return true;
    }
    if (auto* block = dynamic_cast<const BlockNode*>(stmt)) {
        for (const auto& s : block->statements) {
            if (statementDiverges(s.get())) return true;  // statements after it are unreachable
        }
        return false;
    }
    if (auto* ifn = dynamic_cast<const IfStatementNode*>(stmt)) {
        // An `if` diverges only when both arms exist and both diverge.
        return ifn->thenBlock && ifn->elseBlock &&
               statementDiverges(ifn->thenBlock.get()) && statementDiverges(ifn->elseBlock.get());
    }
    return false;
}
}

// True when `node` contains a `break` that leaves the loop whose body it is (not one of a nested loop).
static bool containsLoopExit(ASTNode* node) {
    if (!node) return false;
    if (auto* control = dynamic_cast<LoopControlNode*>(node)) return control->isBreak;
    if (dynamic_cast<WhileStatementNode*>(node) || dynamic_cast<ForStatementNode*>(node) ||
        dynamic_cast<LambdaExpressionNode*>(node) || dynamic_cast<FunctionDeclarationNode*>(node)) {
        return false;
    }
    bool found = false;
    ast_walk::forEachChild(node, [&](ASTNode* child, std::unique_ptr<ASTNode>*) {
        if (!found && containsLoopExit(child)) found = true;
    });
    return found;
}

static bool statementLeavesLoop(const ASTNode* stmt) {
    if (!stmt) return false;
    if (dynamic_cast<const ReturnStatementNode*>(stmt)) return true;
    if (auto* control = dynamic_cast<const LoopControlNode*>(stmt)) return control->isBreak;
    if (auto* block = dynamic_cast<const BlockNode*>(stmt)) {
        for (const auto& s : block->statements) {
            if (statementLeavesLoop(s.get())) return true;
        }
        return false;
    }
    if (auto* ifn = dynamic_cast<const IfStatementNode*>(stmt)) {
        return ifn->thenBlock && ifn->elseBlock &&
               statementLeavesLoop(ifn->thenBlock.get()) && statementLeavesLoop(ifn->elseBlock.get());
    }
    return false;
}

// A decimal literal is a Double. Where a Float is expected -- a Float variable, parameter, return value,
// or the other operand of a Float -- a literal expression (decimal and integer literals, unary minus,
// + - * /, and operands that are already Float) is evaluated as a Float instead, so `val x: Float = 0.5`
// and `x * 2.5` keep Float precision without an `f` suffix. Returns whether `node` is such an
// expression; with `apply`, retypes its Double literals to Float.
static bool narrowDoubleLiteralsToFloat(ASTNode* node, bool apply) {
    if (!node) return false;
    if (auto* value = dynamic_cast<ValueNode*>(node)) {
        if (value->valueType == TokenType::EXPONENTIAL_LITERAL) {
            if (apply) value->type = std::make_unique<BasicType>("Float");
            return true;
        }
        return value->valueType == TokenType::FLOAT_LITERAL || value->valueType == TokenType::INTEGER_LITERAL;
    }
    if (auto* unary = dynamic_cast<UnaryExpressionNode*>(node)) {
        if (unary->op != "-" && unary->op != "+") return false;
        if (!narrowDoubleLiteralsToFloat(unary->expression.get(), apply)) return false;
        if (apply) unary->type.reset();
        return true;
    }
    if (auto* prefix = dynamic_cast<PrefixExpressionNode*>(node)) {
        if (prefix->op != "-" && prefix->op != "+") return false;
        if (!narrowDoubleLiteralsToFloat(prefix->expression.get(), apply)) return false;
        if (apply) prefix->type.reset();
        return true;
    }
    if (auto* binary = dynamic_cast<BinaryExpressionNode*>(node)) {
        const std::string& op = binary->op;
        if (op != "+" && op != "-" && op != "*" && op != "/") return false;
        auto operandOk = [&](ASTNode* operand) {
            if (narrowDoubleLiteralsToFloat(operand, false)) return true;
            auto t = operand ? operand->getType() : nullptr;
            return t && (t->isFloatTy() || t->isIntegerTy());
        };
        if (!operandOk(binary->left.get()) || !operandOk(binary->right.get())) return false;
        if (apply) {
            narrowDoubleLiteralsToFloat(binary->left.get(), true);
            narrowDoubleLiteralsToFloat(binary->right.get(), true);
            binary->type.reset();
        }
        return true;
    }
    return false;
}

static std::vector<std::vector<const Type*>> methodParameterLists(const std::string& className,
                                                                   const std::string& name, size_t arity);
static std::vector<std::vector<const Type*>> constructorParameterLists(const std::string& className, size_t arity);

// A value of a class with a `toString()` method, shown as text (printed, or joined to a String):
// replaced by the call of that method.
static bool wrapInToString(std::unique_ptr<ASTNode>& slot, TypeChecker& checker) {
    if (!slot) return false;
    auto valueType = stripBorrow(slot->getType());
    if (!valueType || valueType->getKind() != Type::Kind::CLASS) return false;
    if (methodParameterLists(valueType->getName(), "toString", 0).empty()) return false;
    auto call = std::make_unique<MethodCallNode>();
    call->line = slot->line;
    call->object = std::move(slot);
    call->methodName = "toString";
    slot = std::move(call);
    slot->accept(checker);
    return true;
}

// When `expr` has type Double and `target` is Float, narrows its literals (see above).
static void narrowToFloatTarget(const Type* target, ASTNode* expr) {
    if (!target || !expr || !target->isFloatTy()) return;
    auto exprType = expr->getType();
    if (!exprType || !exprType->isDoubleTy()) return;
    if (narrowDoubleLiteralsToFloat(expr, false)) narrowDoubleLiteralsToFloat(expr, true);
}

// Narrows literal arguments at positions where every candidate signature takes a Float.
static void narrowArgumentsToFloat(std::vector<std::unique_ptr<ASTNode>>& arguments,
                                   const std::vector<std::vector<const Type*>>& candidates) {
    if (candidates.empty()) return;
    BasicType floatType("Float");
    for (size_t i = 0; i < arguments.size(); ++i) {
        bool allFloat = true;
        for (const auto& params : candidates) {
            if (i >= params.size() || !params[i] || !params[i]->isFloatTy()) { allFloat = false; break; }
        }
        if (allFloat) narrowToFloatTarget(&floatType, arguments[i].get());
    }
}

// Parameter types (receiver excluded) of the methods `name` of `className` and its parents that take
// `arity` arguments.
static std::vector<std::vector<const Type*>> methodParameterLists(const std::string& className,
                                                                   const std::string& name, size_t arity) {
    std::vector<std::vector<const Type*>> result;
    auto& table = CodeGenerator::getInstance().symbolTable;
    for (const ClassSymbol* cls = table.lookupClass(className); cls; cls = cls->superClassSymbol) {
        for (const auto& entry : cls->methods) {
            const Symbol* candidate = entry.second.get();
            if (!candidate || candidate->name != name) continue;
            auto* funcType = dynamic_cast<const FunctionType*>(candidate->type.get());
            if (!funcType) continue;
            std::vector<const Type*> params;
            for (const auto& p : funcType->parameterTypes) params.push_back(p.get());
            if (params.size() == arity + 1) params.erase(params.begin());  // registered with the receiver
            if (params.size() == arity) result.push_back(std::move(params));
        }
    }
    return result;
}

// Constructor parameter types of `className` when it takes `arity` arguments.
static std::vector<std::vector<const Type*>> constructorParameterLists(const std::string& className, size_t arity) {
    std::vector<std::vector<const Type*>> result;
    auto* cls = CodeGenerator::getInstance().symbolTable.lookupClass(className);
    if (!cls || cls->constructorParamOrder.size() != arity) return result;
    std::vector<const Type*> params;
    for (const auto& name : cls->constructorParamOrder) {
        auto it = cls->constructorParams.find(name);
        params.push_back(it != cls->constructorParams.end() && it->second ? it->second->type.get() : nullptr);
    }
    result.push_back(std::move(params));
    return result;
}

// External linkage (see type_checker.h): main.cpp toggles this from `--strict-ownership`. Defined
// here, past the anonymous namespace, so it references the internal g_strictOwnership by TU scope.
void setStrictOwnership(bool enabled) { g_strictOwnership = enabled; }
bool isStrictOwnership() { return g_strictOwnership; }

void TypeChecker::reportError(const std::string& message) {
    ++errorCount;
    std::cerr << message << std::endl;
}

void TypeChecker::enterOwnershipScope() {
    ownershipScopes.emplace_back();
}

void TypeChecker::exitOwnershipScope() {
    if (!ownershipScopes.empty()) {
        ownershipScopes.pop_back();
    }
}

void TypeChecker::declareOwnership(const std::string& name, const std::unique_ptr<Type>& type) {
    if (ownershipScopes.empty()) {
        enterOwnershipScope();
    }
    OwnershipState state;
    state.owned = type && isMoveCheckedType(type.get());
    ownershipScopes.back()[name] = state;
}

TypeChecker::OwnershipState* TypeChecker::findOwnership(const std::string& name) {
    for (auto it = ownershipScopes.rbegin(); it != ownershipScopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

void TypeChecker::checkIdentifierUse(const std::string& name) {
    auto* state = findOwnership(name);
    if (state && state->owned && state->moved) {
        reportError("Type error: use of moved value '" + name + "'");
    }
}

bool TypeChecker::isFieldPath(ASTNode* node) {
    if (dynamic_cast<AccessFieldNode*>(node)) return true;
    auto* id = dynamic_cast<IdentifierNode*>(node);
    if (!id) return false;
    if (id->value.find('.') != std::string::npos) return true;
    if (findOwnership(id->value)) return false;
    Symbol* symbol = CodeGenerator::getInstance().symbolTable.lookup(id->value);
    return symbol && symbol->symbolType == SymbolType::FIELD;
}

static std::string fieldPathText(ASTNode* node) {
    if (auto* access = dynamic_cast<AccessFieldNode*>(node)) {
        const std::string base = fieldPathText(access->base.get());
        const std::string field = fieldPathText(access->field.get());
        return base.empty() ? field : base + "." + field;
    }
    return identifierName(node);
}

void TypeChecker::checkOwnedValueUse(ASTNode* node, const std::string& context) {
    if (!node || !isStrictOwnership()) return;
    if (isMoveExpression(node) || isBorrowExpression(node, nullptr)) return;
    auto type = node->getType();
    if (!type || !isMoveCheckedType(type.get())) return;
    if (isFieldPath(node)) {
        reportError("Type error: " + context + " cannot take the owned field '" + fieldPathText(node) +
                    "' by value; borrow it with '&' or copy it with clone(...)");
        return;
    }
    if (auto* id = dynamic_cast<IdentifierNode*>(node)) {
        reportError("Type error: " + context + " needs '<-' to move '" + id->value +
                    "' (or '&' to borrow it, clone(...) to copy it)");
    }
}

void TypeChecker::markMoved(const std::string& name) {
    auto* state = findOwnership(name);
    if (!state) return;
    if (state->immutableBorrows > 0 || state->mutableBorrowed) {
        reportError("Type error: cannot move '" + name + "' while it is borrowed");
    }
    if (state->owned) {
        state->moved = true;
    }
}

void TypeChecker::markBorrowed(const std::string& name, bool isMutableBorrow) {
    auto* state = findOwnership(name);
    if (!state) return;
    if (state->owned && state->moved) {
        reportError("Type error: cannot borrow moved value '" + name + "'");
        return;
    }
    if (isMutableBorrow) {
        auto* symbol = CodeGenerator::getInstance().symbolTable.lookup(name);
        if (symbol && !symbol->isMutable) {
            reportError("Type error: cannot mutably borrow immutable value '" + name + "'");
        }
        if (state->immutableBorrows > 0) {
            reportError("Type error: cannot mutably borrow '" + name + "' while immutable borrow exists");
        }
        if (state->mutableBorrowed) {
            reportError("Type error: cannot mutably borrow '" + name + "' more than once");
        }
        state->mutableBorrowed = true;
    }
    else {
        if (state->mutableBorrowed) {
            reportError("Type error: cannot immutably borrow '" + name + "' while mutable borrow exists");
        }
        ++state->immutableBorrows;
    }
}

void TypeChecker::releaseBorrow(const std::string& name, bool isMutableBorrow) {
    auto* state = findOwnership(name);
    if (!state) return;
    if (isMutableBorrow) {
        state->mutableBorrowed = false;
    }
    else if (state->immutableBorrows > 0) {
        --state->immutableBorrows;
    }
}

void TypeChecker::checkFunctionArguments(const std::vector<std::unique_ptr<ASTNode>>& arguments,
                                         const std::vector<std::unique_ptr<Type>>& parameterTypes,
                                         const std::string& callName) {
    for (size_t i = 0; i < arguments.size(); ++i) {
        ASTNode* arg = arguments[i].get();
        Type* paramType = i < parameterTypes.size() && parameterTypes[i] ? parameterTypes[i].get() : nullptr;
        narrowToFloatTarget(paramType, arg);
        bool mutableBorrowArg = false;
        const bool argIsMove = isMoveExpression(arg);
        const bool argIsBorrow = isBorrowExpression(arg, &mutableBorrowArg);
        auto argType = arg ? arg->getType() : std::make_unique<UnknownType>();
        // An explicit `<-` is only required for a NAMED owned value (an identifier bound to an owned
        // local). A temporary — a literal, `new X()`, a call result, a computed value — is an rvalue
        // with no other owner, so it moves implicitly and needs no marker (as in Rust).
        const bool argIsNamedValue = arg && !identifierName(arg).empty();

        if (paramType && isMoveCheckedType(paramType)) {
            if (argIsBorrow) {
                reportError("Type error: argument " + std::to_string(i + 1) + " to '" + callName +
                            "' takes ownership; pass it with '<-', or declare the parameter as a borrow (&T)");
            }
            else if (!argIsMove && !argIsNamedValue && isStrictOwnership() &&
                     (paramType->getKind() == Type::Kind::ARRAY ||
                      (paramType->getKind() == Type::Kind::GENERIC &&
                       (paramType->getName() == "Array" || paramType->getName() == "Vector"))) &&
                     (dynamic_cast<ArrayAccessNode*>(arg) || dynamic_cast<AccessFieldNode*>(arg))) {
                reportError("Type error: argument " + std::to_string(i + 1) + " to '" + callName +
                            "' takes ownership of an array it does not own; pass a new array or move a local with '<-'");
            }
            else if (!argIsMove && argIsNamedValue && isFieldPath(arg)) {
                checkOwnedValueUse(arg, "the call to '" + callName + "'");
            }
            else if (!argIsMove && !argIsBorrow && argIsNamedValue) {
                reportError("Type error: ownership transfer to '" + callName + "' requires '<-'");
            }
        }
        else if (paramType && paramType->getKind() == Type::Kind::BORROW) {
            if (!argIsBorrow || mutableBorrowArg) {
                reportError("Type error: argument " + std::to_string(i + 1) + " to '" + callName + "' requires '&'");
            }
        }
        else if (paramType && paramType->getKind() == Type::Kind::MUTABLE_BORROW) {
            if (!argIsBorrow || !mutableBorrowArg) {
                reportError("Type error: argument " + std::to_string(i + 1) + " to '" + callName + "' requires '&mut'");
            }
        }
        else if (argIsMove) {
            reportError("Type error: '<-' argument to '" + callName + "' requires an owning parameter");
        }
        else if (argType && isMoveCheckedType(argType.get()) && argIsNamedValue) {
            reportError("Type error: ownership transfer to '" + callName + "' requires '<-'");
        }

        if (argIsMove) {
            markMoved(movedOrBorrowedIdentifier(arg));
        }
    }
}

void TypeChecker::checkTypeResolved(const std::unique_ptr<Type>& type, const std::string& context) {
    if (!isResolvedType(type)) {
        reportError("Type error: Failed to resolve type for " + context);
    }
}

void TypeChecker::visit(VariableDeclarationNode& node) {
    if (node.initializer) {
        // `val d: Dict<K, V> = {...}` (including an empty `{}`) takes its type from the annotation.
        if (auto* dictLiteral = dynamic_cast<DictLiteralNode*>(node.initializer.get())) {
            if (node.hasExplicitType && (isDictType(node.type.get()) || isSetType(node.type.get()))) {
                dictLiteral->type = node.type->clone();
            }
        }
        visitExpecting(node.initializer.get(), node.hasExplicitType ? node.type.get() : nullptr);
        if (node.hasExplicitType) coerceToInterface(node.initializer, node.type.get());
        auto initType = node.initializer->getType();
        if (!isResolvedType(initType)) {
            if (auto* ifNode = dynamic_cast<IfStatementNode*>(node.initializer.get())) {
                auto thenType = ifNode->thenBlock ? ifNode->thenBlock->getType() : std::make_unique<UnknownType>();
                auto elseType = ifNode->elseBlock ? ifNode->elseBlock->getType() : std::make_unique<UnknownType>();
                if (isResolvedType(thenType) && isResolvedType(elseType) && thenType->equals(elseType)) {
                    initType = thenType->clone();
                }
                else if (isResolvedType(thenType)) {
                    initType = thenType->clone();
                }
                else if (isResolvedType(elseType)) {
                    initType = elseType->clone();
                }
            }
            else if (auto* matchNode = dynamic_cast<MatchExpressionNode*>(node.initializer.get())) {
                for (const auto& casePair : matchNode->cases) {
                    if (!casePair.second) {
                        continue;
                    }
                    auto caseType = casePair.second->getType();
                    if (isResolvedType(caseType)) {
                        initType = caseType ? caseType->clone() : std::make_unique<UnknownType>();
                        break;
                    }
                }
            }
        }

        if (node.type && node.hasExplicitType && node.initializer) {
            narrowToFloatTarget(node.type.get(), node.initializer.get());
            initType = node.initializer->getType();
        }
        if (node.type) {
            if (!node.hasExplicitType && isResolvedType(initType)) {
                node.type = initType ? initType->clone() : std::make_unique<UnknownType>();
            }
            else if (!isResolvedType(node.type) && isResolvedType(initType)) {
                node.type = initType ? initType->clone() : std::make_unique<UnknownType>();
            }
            else if (isResolvedType(node.type) && isResolvedType(initType) &&
                     !node.type->equals(initType) &&
                     !((node.type->getName() == "Bool" && initType->getName() == "Boolean") ||
                       (node.type->getName() == "Boolean" && initType->getName() == "Bool")) &&
                     !(node.type->isIntegerTy() && initType->isIntegerTy()) &&
                     !(isI64IntegerAlias(node.type) && isIntegerTypeName(initType)) &&
                     !(node.type->isDoubleTy() && initType->isFloatTy()) &&
                     !((node.type->isDoubleTy() || node.type->getName() == "Real") &&
                       (initType->isIntegerTy() || initType->isFloatTy() || initType->isDoubleTy())) &&
                     // Nat (arbitrary-precision natural number) accepts any integer or another Nat.
                     !(node.type->getName() == "Nat" &&
                       (initType->isIntegerTy() || isIntegerTypeName(initType) || initType->getName() == "Nat")) &&
                     !canInitializeOwnershipType(node.type.get(), initType.get()) &&
                     !(node.type->getKind() == Type::Kind::GENERIC && initType->getKind() == Type::Kind::GENERIC &&
                       node.type->getName() == initType->getName() &&
                       (node.type->getName() == "Shared" || node.type->getName() == "Weak" ||
                        (node.type->getName() == "Future" && sameFirstTypeArgument(node.type.get(), initType.get())))) &&
                     !(isTensorLikeType(node.type) && isTensorLikeType(initType)) &&
                     !isArrayVectorAlias(node.type, initType) &&
                     !initType->isSubtypeOf(node.type) &&
                     !node.type->isSubtypeOf(initType)) {
                reportError(
                    "Type error: Variable '" + node.name + "' declared as '" +
                    node.type->getName() + "' but initialized with '" + initType->getName() + "'");
            }
        }
        else {
            node.type = initType ? initType->clone() : std::make_unique<UnknownType>();
        }
    }
    else if (!node.type) {
        reportError("Type error: Variable '" + node.name + "' has no type and no initializer");
        node.type = std::make_unique<UnknownType>();
    }

    // Owned-wrapper globals (box/Shared/Weak declared at module scope) are not yet supported: they
    // need a startup constructor to run their heap initialization and a shutdown pass to release
    // them, and moving a box bound at module scope currently loops in codegen. Reject with a clear
    // diagnostic instead of hanging or silently mis-lowering. Scalar globals (Int, Bool, …) are fine
    // and unaffected; owned values kept in function scope keep their deterministic destruction. No
    // fixture declares an owned global, so this is a pure hang-to-error improvement.
    if (CodeGenerator::getInstance().symbolTable.getCurrentSymbol() == nullptr && !node.isField &&
        node.type) {
        const bool ownedWrapperGlobal =
            node.type->getKind() == Type::Kind::BOX ||
            (node.type->getKind() == Type::Kind::GENERIC &&
             (node.type->getName() == "Shared" || node.type->getName() == "Weak"));
        if (ownedWrapperGlobal) {
            reportError("Type error: owned global '" + node.name + "' of type '" +
                        node.type->getName() + "' is not supported; keep owned values (box/Shared/"
                        "Weak) in function scope where construction and destruction are deterministic");
        }
    }

    if (node.initializer && !node.isField) {
        checkOwnedValueUse(node.initializer.get(), "initializing '" + node.name + "'");
        std::vector<ASTNode*> branches;
        if (auto* ifNode = dynamic_cast<IfStatementNode*>(node.initializer.get())) {
            branches = { ifNode->thenBlock.get(), ifNode->elseBlock.get() };
        }
        else if (auto* match = dynamic_cast<MatchExpressionNode*>(node.initializer.get())) {
            for (auto& item : match->cases) branches.push_back(item.second.get());
        }
        for (ASTNode* branch : branches) {
            ASTNode* yielded = branch;
            if (auto* block = dynamic_cast<BlockNode*>(branch)) {
                yielded = block->statements.empty() ? nullptr : block->statements.back().get();
            }
            auto* id = dynamic_cast<IdentifierNode*>(yielded);
            if (id && (findOwnership(id->value) || isFieldPath(id))) {
                checkOwnedValueUse(id, "initializing '" + node.name + "'");
            }
        }
    }
    bindVariable(node.name, node.type, node.isMutable, node.isField ? SymbolType::FIELD : SymbolType::VARIABLE);
    declareOwnership(node.name, node.type);
}

void TypeChecker::visit(FunctionDeclarationNode& node) {
    auto& cg = CodeGenerator::getInstance();
    auto* functionSymbol = ensureFunctionSymbol(node);
    auto* savedCurrentSymbol = cg.symbolTable.getCurrentSymbol();
    cg.symbolTable.saveCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(functionSymbol);
    cg.symbolTable.enterScope();
    enterOwnershipScope();

    for (auto& param : node.parameters) {
        if (param) {
            param->accept(*this);
            auto* parameterNode = dynamic_cast<ParameterNode*>(param.get());
            if (parameterNode) {
                bindVariable(parameterNode->name, parameterNode->type, false);
                declareOwnership(parameterNode->name, parameterNode->type);
            }
        }
    }

    if (node.contracts) {
        // `requires` sees the parameters; `ensures` also sees `result`, the returned value.
        for (auto& clause : node.contracts->requiresClauses) checkContractClause(clause, "requires", true);
        cg.symbolTable.enterScope();
        if (node.returnType && !node.returnType->isVoidTy() && node.returnType->getName() != "Unit") {
            bindVariable("result", node.returnType, false);
        }
        for (auto& clause : node.contracts->ensuresClauses) checkContractClause(clause, "ensures", true);
        cg.symbolTable.exitScope();
    }

    if (node.isUnsafe) {
        ++unsafeContextDepth;
    }
    const bool savedAsync = inAsyncFunction;
    inAsyncFunction = node.isAsync;
    const int savedLoopDepth = loopDepth;
    loopDepth = 0;
    const Type* savedReturnType = currentReturnType;
    currentReturnType = node.returnType.get();

    if (node.body) {
        blockResultExpected = node.returnType.get();
        node.body->accept(*this);
        blockResultExpected = nullptr;
        // An expression-bodied `def f(): Float = 0.5` returns its last expression.
        if (node.returnType && !node.body->statements.empty()) {
            narrowToFloatTarget(node.returnType.get(), node.body->statements.back().get());
            if (isFieldPath(node.body->statements.back().get())) {
                checkOwnedValueUse(node.body->statements.back().get(), "the result of '" + node.name + "'");
            }
        }
        auto bodyType = node.body->getType();

        if (node.returnType) {
            if (isResolvedType(node.returnType) && isResolvedType(bodyType) &&
                !node.returnType->isVoidTy() &&
                !node.returnType->equals(bodyType) &&
                !(isIntegerTypeName(node.returnType) && isIntegerTypeName(bodyType)) &&
                !(isTensorLikeType(node.returnType) && isTensorLikeType(bodyType)) &&
                !(isI64IntegerAlias(node.returnType) && isI64IntegerAlias(bodyType)) &&
                !(isIntegerTypeName(node.returnType) && bodyType && bodyType->getName() == "Char") &&
                !((node.returnType->isDoubleTy() || node.returnType->getName() == "Real") &&
                  (bodyType->isFloatTy() || bodyType->isDoubleTy() || bodyType->isIntegerTy())) &&
                !(node.returnType->isFloatTy() && bodyType->isFloatTy()) &&
                !isArrayVectorAlias(node.returnType, bodyType)) {
                reportError(
                    "Type error: Function '" + node.name + "' declared to return '" +
                    node.returnType->getName() + "' but returns '" + bodyType->getName() + "'");
            }
        }
        else if (isResolvedType(bodyType)) {
            node.returnType = bodyType->clone();
        }
    }

    if (node.isUnsafe) {
        --unsafeContextDepth;
    }
    inAsyncFunction = savedAsync;
    loopDepth = savedLoopDepth;
    currentReturnType = savedReturnType;

    exitOwnershipScope();
    cg.symbolTable.exitScope();
    cg.symbolTable.popCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(savedCurrentSymbol);
}

void TypeChecker::visit(ExpressionNode& node) {
    checkTypeResolved(node.getType(), "expression");
}

// `a op b` where a's class (or struct) overloads `op`: one of those overloads must accept b's type.
// Template instances are distinct classes, so GF<7> + GF<11> is rejected here.
static std::string displayClassName(const std::string& name) {
    const size_t dollar = name.find('$');
    if (dollar == std::string::npos) return name;
    std::string text = name.substr(0, dollar) + "<";
    std::string rest = name.substr(dollar + 1);
    for (char& c : rest) if (c == '$') c = ',';
    size_t pos = 0;
    while ((pos = rest.find(',', pos)) != std::string::npos) { rest.replace(pos, 1, ", "); pos += 2; }
    return text + rest + ">";
}

bool TypeChecker::checkClassOperatorOperand(const std::string& op, const Type* leftType, const Type* rightType) {
    if (!leftType || !rightType || leftType->getKind() != Type::Kind::CLASS) return true;
    if (rightType->getKind() == Type::Kind::UNKNOWN) return true;
    auto& table = CodeGenerator::getInstance().symbolTable;
    ClassSymbol* classSymbol = table.lookupClass(leftType->getName());
    if (!classSymbol) return true;
    const std::string name = "operator" + op;
    bool overloaded = false;
    for (const ClassSymbol* cls = classSymbol; cls; cls = cls->superClassSymbol) {
        for (const auto& entry : cls->methods) {
            const Symbol* candidate = entry.second.get();
            if (!candidate || candidate->name != name) continue;
            auto* funcType = dynamic_cast<const FunctionType*>(candidate->type.get());
            if (!funcType || funcType->parameterTypes.empty()) continue;  // the unary form
            // Registered without the receiver while type checking, with it once generated.
            const bool generated = llvm::isa_and_nonnull<llvm::Function>(candidate->value);
            if (generated && funcType->parameterTypes.size() != 2) continue;
            if (!generated && funcType->parameterTypes.size() != 1) continue;
            overloaded = true;
            const auto& parameter = funcType->parameterTypes.back();
            if (!parameter || parameter->equals(*rightType) || rightType->isSubtypeOf(*parameter) ||
                isNumericWidening(*rightType, *parameter)) {
                return true;
            }
        }
    }
    if (!overloaded) return true;
    reportError("Type error: no 'operator" + op + "' of " + displayClassName(leftType->getName()) +
                " accepts an operand of type " + displayClassName(rightType->getName()));
    return false;
}

void TypeChecker::visit(BinaryExpressionNode& node) {
    narrowFloatOperands(node);
    visitBinaryExpression(node);
}

void TypeChecker::narrowFloatOperands(BinaryExpressionNode& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
    const std::string& op = node.op;
    const bool numeric = op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "**" ||
        op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=";
    if (op == "+" && node.left && node.right) {
        auto l = node.left->getType();
        auto r = node.right->getType();
        if (l && r && (l->isStringTy() || r->isStringTy())) {
            if (!l->isStringTy() && wrapInToString(node.left, *this)) node.type.reset();
            if (!r->isStringTy() && wrapInToString(node.right, *this)) node.type.reset();
        }
    }
    if (!numeric || !node.left || !node.right) return;
    auto leftType = node.left->getType();
    auto rightType = node.right->getType();
    if ((op == "==" || op == "!=")) {
        const EnumInfo* leftEnum = enumOfType(leftType.get());
        const EnumInfo* rightEnum = enumOfType(rightType.get());
        if (leftEnum || rightEnum) {
            if (leftEnum != rightEnum || leftType->getName() != rightType->getName()) {
                reportError("Type error: cannot compare '" + typeDisplayName(leftType.get()) + "' with '" +
                            typeDisplayName(rightType.get()) + "'");
                return;
            }
            if (leftEnum->hasPayload) {
                reportError("Type error: values of enum '" + leftEnum->name + "' carry data and cannot be compared "
                            "with " + op + "; use match");
                return;
            }
            node.left = std::make_unique<AccessFieldNode>(std::move(node.left), std::make_unique<IdentifierNode>(enumTagField()));
            node.right = std::make_unique<AccessFieldNode>(std::move(node.right), std::make_unique<IdentifierNode>(enumTagField()));
            node.type.reset();
            return;
        }
    }
    if (leftType && leftType->isFloatTy()) narrowToFloatTarget(leftType.get(), node.right.get());
    if (rightType && rightType->isFloatTy()) narrowToFloatTarget(rightType.get(), node.left.get());
    node.type.reset();
}

void TypeChecker::visitBinaryExpression(BinaryExpressionNode& node) {
    // The operands were visited by narrowFloatOperands.
    if (node.left && node.right) {
        auto leftType = node.left->getType();
        auto rightType = node.right->getType();
        if (!checkClassOperatorOperand(node.op, leftType.get(), rightType.get())) {
            return;
        }
        if (leftType && leftType->getKind() == Type::Kind::CLASS && rightType && isMoveCheckedType(rightType.get())) {
            if (isFieldPath(node.right.get())) {
                checkOwnedValueUse(node.right.get(), "the operator '" + node.op + "'");
            }
            else if (auto* id = dynamic_cast<IdentifierNode*>(node.right.get())) {
                markMoved(id->value);
            }
        }
        if ((node.op == "in" || node.op == "notin") && isSetType(rightType.get())) {
            checkDictValue(setElementType(rightType.get()), node.left.get(), "set element");
        }
        else if ((isSetType(leftType.get()) || isSetType(rightType.get())) &&
                 !(node.op == "+" && (leftType->isStringTy() || rightType->isStringTy()))) {  // "s=" + set
            if (!isSetBinaryOperator(node.op)) {
                reportError("Type error: operator '" + node.op + "' is not defined for sets");
                return;
            }
            if (!setBinaryOperatorType(node.op, leftType.get(), rightType.get())) {
                reportError("Type error: set operator '" + node.op + "' needs two sets with the same element type, got '" +
                            typeDisplayName(leftType.get()) + "' and '" + typeDisplayName(rightType.get()) + "'");
                return;
            }
        }
    }
    auto resolved = node.getType();
    if (!isResolvedType(resolved)) {
        node.type = std::make_unique<BasicType>("Real");
        return;
    }
}

void TypeChecker::visit(ProgramNode& node) {
    enterOwnershipScope();
    indexCallSignatures(&node);
    expandTemplateBodies(&node, callReport());
    // Templates and class names first, so that applied template types in signatures (`Opt<Int>`) can
    // be resolved to their instance classes before function signatures are registered.
    for (auto& stmt : node.statements) {
        if (auto* templateNode = dynamic_cast<TemplateDeclarationNode*>(stmt.get())) {
            ensureTemplateSymbol(*templateNode);
        }
        else if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(stmt.get())) {
            ensureClassSymbol(*classNode);
        }
    }
    normalizeAppliedTemplateTypes(&node);
    for (auto& slot : node.statements) {
        if (!slot) {
            continue;
        }
        // `[@Attribute] def f(...)`: register the declaration under its attributes as well.
        ASTNode* stmtNode = slot.get();
        while (auto* attribute = dynamic_cast<AttributeNode*>(stmtNode)) stmtNode = attribute->target.get();
        if (!stmtNode) continue;

        if (auto* functionNode = dynamic_cast<FunctionDeclarationNode*>(stmtNode)) {
            ensureFunctionSymbol(*functionNode);
        }
        else if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(stmtNode)) {
            ensureClassSymbol(*classNode);
        }
        else if (auto* objectNode = dynamic_cast<ObjectDeclarationNode*>(stmtNode)) {
            auto& cg = CodeGenerator::getInstance();
            if (!cg.symbolTable.lookup(objectNode->name)) {
                cg.symbolTable.addSymbol(objectNode->name, std::make_unique<NamespaceSymbol>(objectNode->name));
            }
        }
        else if (auto* templateNode = dynamic_cast<TemplateDeclarationNode*>(stmtNode)) {
            ensureTemplateSymbol(*templateNode);
        }
    }

    for (auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    exitOwnershipScope();
}

void TypeChecker::visit(ClassDeclarationNode& node) {
    auto& cg = CodeGenerator::getInstance();
    auto* classSymbol = ensureClassSymbol(node);
    auto* savedCurrentSymbol = cg.symbolTable.getCurrentSymbol();
    cg.symbolTable.saveCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(classSymbol);
    cg.symbolTable.enterScope();
    enterOwnershipScope();

    // Bind `this` to the class type so field and method accesses through it resolve during type
    // checking (otherwise `this` caches the numeric default and breaks codegen).
    {
        std::unique_ptr<Type> thisType = std::make_unique<ClassType>(node.name);
        bindVariable("this", thisType, false);
    }
    if (node.isStruct && isStrictOwnership()) {
        auto rejectOwned = [&](const std::string& fieldName, const Type* fieldType) {
            if (fieldType && fieldType->getKind() != Type::Kind::UNKNOWN && isMoveCheckedType(fieldType)) {
                reportError("Type error: struct '" + node.name + "' cannot hold the owned field '" + fieldName + "' of type '" +
                            fieldType->getName() + "'; structs are copied, so declare '" + node.name + "' as a class");
            }
        };
        for (auto& param : node.constructorParams) {
            if (auto* p = dynamic_cast<ParameterNode*>(param.get())) rejectOwned(p->name, p->type.get());
        }
        if (auto* body = dynamic_cast<ClassBodyNode*>(node.body.get())) {
            for (auto& field : body->fields) {
                if (auto* f = dynamic_cast<VariableDeclarationNode*>(field.get())) rejectOwned(f->name, f->type.get());
            }
        }
    }

    for (auto& param : node.constructorParams) {
        if (param) {
            param->accept(*this);
            auto* parameterNode = dynamic_cast<ParameterNode*>(param.get());
            if (!parameterNode) {
                continue;
            }
            bindVariable(parameterNode->name, parameterNode->type, false);
            declareOwnership(parameterNode->name, parameterNode->type);
            if (classSymbol && classSymbol->constructorParams.count(parameterNode->name) == 0) {
                classSymbol->constructorParams[parameterNode->name] =
                    std::make_unique<Symbol>(parameterNode->name, parameterNode->type ? parameterNode->type->clone() : std::make_unique<UnknownType>(), nullptr, false, SymbolType::FIELD);
            }
        }
    }
    auto* classBody = dynamic_cast<ClassBodyNode*>(node.body.get());
    if (classBody) {
        for (auto& field : classBody->fields) {
            if (!field) {
                continue;
            }
            auto* fieldNode = dynamic_cast<VariableDeclarationNode*>(field.get());
            if (!fieldNode) {
                continue;
            }
            fieldNode->accept(*this);
            if (classSymbol && classSymbol->fields.count(fieldNode->name) == 0) {
                classSymbol->fields[fieldNode->name] =
                    std::make_unique<Symbol>(fieldNode->name, fieldNode->type ? fieldNode->type->clone() : std::make_unique<UnknownType>(), nullptr, fieldNode->isMutable, SymbolType::FIELD);
            }
        }
        for (auto& method : classBody->methods) {
            if (!method) {
                continue;
            }
            auto* functionNode = dynamic_cast<FunctionDeclarationNode*>(method.get());
            if (functionNode && classSymbol && classSymbol->methods.count(functionNode->name) == 0) {
                classSymbol->methods[functionNode->name] =
                    std::make_unique<FunctionSymbol>(functionNode->name, functionNode->getType(), nullptr, false, SymbolType::METHOD);
            }
        }
        classBody->accept(*this);
    }
    else if (node.body) {
        node.body->accept(*this);
    }

    exitOwnershipScope();
    cg.symbolTable.exitScope();
    cg.symbolTable.popCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(savedCurrentSymbol);
}

void TypeChecker::visit(ObjectDeclarationNode& node) {
    auto& cg = CodeGenerator::getInstance();
    auto* symbol = cg.symbolTable.lookup(node.name);
    if (!symbol) {
        cg.symbolTable.addSymbol(node.name, std::make_unique<NamespaceSymbol>(node.name));
        symbol = cg.symbolTable.lookup(node.name);
    }

    auto* savedCurrentSymbol = cg.symbolTable.getCurrentSymbol();
    cg.symbolTable.saveCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(symbol);
    cg.symbolTable.enterScope();
    enterOwnershipScope();

    if (node.body) {
        node.body->accept(*this);
    }

    exitOwnershipScope();
    cg.symbolTable.exitScope();
    cg.symbolTable.popCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(savedCurrentSymbol);
}

void TypeChecker::mergeMovedFrom(
    const std::vector<std::unordered_map<std::string, OwnershipState>>& source) {
    const size_t n = std::min(ownershipScopes.size(), source.size());
    for (size_t i = 0; i < n; ++i) {
        for (const auto& entry : source[i]) {
            if (!entry.second.moved) continue;
            auto it = ownershipScopes[i].find(entry.first);
            if (it != ownershipScopes[i].end()) {
                it->second.moved = true;
            }
        }
    }
}

void TypeChecker::visit(IfStatementNode& node) {
    if (node.condition) {
        node.condition->accept(*this);
        checkTypeResolved(node.condition->getType(), "if condition");
    }

    // Path-sensitive move analysis (M3/M4). Evaluate both arms from the same pre-branch state, then
    // join only the paths that fall through: a value is moved after the `if` iff it is moved on some
    // surviving path. A branch that diverges (always returns) contributes no post-branch moves, so a
    // `<-` inside `if (c) { return f(<- x); }` no longer falsely marks `x` moved for the code after.
    const auto before = ownershipScopes;

    if (node.thenBlock) {
        node.thenBlock->accept(*this);
    }
    const bool thenDiverges = node.thenBlock && statementDiverges(node.thenBlock.get());
    const bool thenBreaks = !thenDiverges && loopDepth > 0 && !loopExitStates.empty() &&
                            node.thenBlock && statementLeavesLoop(node.thenBlock.get());
    const auto afterThen = ownershipScopes;
    if (thenBreaks) loopExitStates.back().push_back(afterThen);

    // The else arm runs on the false path, from the pre-branch state — not the then arm's state.
    ownershipScopes = before;
    if (node.elseBlock) {
        node.elseBlock->accept(*this);
    }
    const bool elseDiverges = node.elseBlock && statementDiverges(node.elseBlock.get());
    const bool elseBreaks = !elseDiverges && loopDepth > 0 && !loopExitStates.empty() &&
                            node.elseBlock && statementLeavesLoop(node.elseBlock.get());
    const auto afterElse = ownershipScopes;
    if (elseBreaks) loopExitStates.back().push_back(afterElse);

    // Join the surviving paths. With no else arm, the false path falls through with `before` (no
    // extra moves), which is exactly what starting from `before` and skipping the else merge gives.
    ownershipScopes = before;
    if (!thenDiverges && !thenBreaks) {
        mergeMovedFrom(afterThen);
    }
    if (!elseDiverges && !elseBreaks) {
        mergeMovedFrom(afterElse);
    }

    node.type = node.getType();
}

void TypeChecker::visitLoopBody(ASTNode* body, size_t outerScopes) {
    const auto beforeBody = ownershipScopes;
    loopExitStates.emplace_back();
    ++loopDepth;
    body->accept(*this);
    --loopDepth;
    auto exits = std::move(loopExitStates.back());
    loopExitStates.pop_back();
    if (!statementLeavesLoop(body)) {
        const size_t n = std::min({ outerScopes, beforeBody.size(), ownershipScopes.size() });
        for (size_t i = 0; i < n; ++i) {
            for (const auto& entry : ownershipScopes[i]) {
                auto before = beforeBody[i].find(entry.first);
                if (entry.second.moved && before != beforeBody[i].end() && !before->second.moved) {
                    reportError("Type error: '" + entry.first +
                                "' is moved inside a loop body and would be moved again on the next iteration");
                }
            }
        }
    }
    for (const auto& exit : exits) mergeMovedFrom(exit);
}

void TypeChecker::visit(ForStatementNode& node) {
    auto& cg = CodeGenerator::getInstance();
    if (node.iterableExpr) {
        node.iterableExpr->accept(*this);
    }
    cg.symbolTable.enterScope();
    enterOwnershipScope();
    auto iterableType = (node.iterableExpr && !node.isRange) ? node.iterableExpr->getType() : nullptr;
    if (isSetType(iterableType.get())) {
        // `for (x <- s)` binds each element in insertion order.
        if (!node.tokenValue.empty()) {
            reportError("Type error: a set is iterated with one variable: for (x <- s)");
        }
        const Type* elementType = setElementType(iterableType.get());
        bindVariable(node.variable, cloneType(elementType), false);
        declareOwnership(node.variable, cloneType(elementType));
    }
    else if (isDictType(iterableType.get())) {
        // `for (k <- d)` binds each key; `for (k, v <- d)` binds the key and its value.
        const Type* keyType = dictKeyType(iterableType.get());
        const Type* valueType = dictValueType(iterableType.get());
        const std::string keyName = node.tokenValue.empty() ? node.variable : node.tokenValue;
        bindVariable(keyName, cloneType(keyType), false);
        declareOwnership(keyName, cloneType(keyType));
        if (!node.tokenValue.empty()) {
            bindVariable(node.variable, cloneType(valueType), false);
            declareOwnership(node.variable, cloneType(valueType));
        }
    }
    else {
        std::unique_ptr<Type> elementType;
        if (auto* arrayType = dynamic_cast<ArrayType*>(iterableType.get())) {
            if (arrayType->elementType) elementType = arrayType->elementType->clone();
        }
        else if (auto* genericType = dynamic_cast<GenericType*>(iterableType.get())) {
            const std::string base = genericType->baseType ? genericType->baseType->getName() : "";
            if ((base == "Array" || base == "Vector") && !genericType->typeArguments.empty() &&
                genericType->typeArguments[0]) {
                elementType = genericType->typeArguments[0]->clone();
            }
        }
        if (!elementType && !node.isRange && !node.tokenStream && iterableType &&
            isResolvedType(iterableType) && iterableType->getKind() != Type::Kind::UNKNOWN) {
            reportError("Type error: cannot iterate over a value of type '" + typeDisplayName(iterableType.get()) +
                        "' in for (" + node.variable + " <- ...); iterate an array, a range (a to b, a until b), "
                        "a Dict or a Set");
        }
        if (!elementType) elementType = std::make_unique<BasicType>("Int");
        bindVariable(node.variable, elementType, false);
        declareOwnership(node.variable, elementType);
    }
    visitLoopContracts(node);
    if (node.body) {
        visitLoopBody(node.body.get(), ownershipScopes.size() - 1);
    }
    exitOwnershipScope();
    cg.symbolTable.exitScope();
}

void TypeChecker::visit(ReturnStatementNode& node) {
    if (node.expression) {
        visitExpecting(node.expression.get(), currentReturnType);
        coerceToInterface(node.expression, currentReturnType);
        narrowToFloatTarget(currentReturnType, node.expression.get());
        // Borrow escape (memory model §6): a borrow of a local dangles once the function returns.
        // The MVP forbids returning references outright rather than introducing lifetime
        // parameters, so `return &x` / `return &mut x` is always rejected.
        bool isMutableBorrow = false;
        if (isBorrowExpression(node.expression.get(), &isMutableBorrow)) {
            const std::string name = movedOrBorrowedIdentifier(node.expression.get());
            reportError("Type error: reference to local '" + name +
                        "' escapes its function; return the value itself (move it with '<-' at the "
                        "call site) or return an owned copy with clone(" + name + ")");
        }
        if (isFieldPath(node.expression.get())) checkOwnedValueUse(node.expression.get(), "return");
        if (auto* id = dynamic_cast<IdentifierNode*>(node.expression.get())) {
            Symbol* source = CodeGenerator::getInstance().symbolTable.lookup(id->value);
            const bool returnsBorrow = currentReturnType && (currentReturnType->getKind() == Type::Kind::BORROW ||
                                                              currentReturnType->getKind() == Type::Kind::MUTABLE_BORROW);
            if (source && source->type && !returnsBorrow &&
                (source->type->getKind() == Type::Kind::BORROW || source->type->getKind() == Type::Kind::MUTABLE_BORROW)) {
                reportError("Type error: cannot return the borrow '" + id->value + "' as an owned value; return clone(" +
                            id->value + ")");
            }
        }
        checkTypeResolved(node.expression->getType(), "return expression");
    }
}

void TypeChecker::visit(BlockNode& node) {
    auto& cg = CodeGenerator::getInstance();
    cg.symbolTable.enterScope();
    enterOwnershipScope();
    if (node.isUnsafeContext) {
        ++unsafeContextDepth;
    }
    std::unique_ptr<Type> lastType = std::make_unique<UnknownType>();
    const Type* resultExpected = blockResultExpected;
    blockResultExpected = nullptr;
    for (auto& stmt : node.statements) {
        if (stmt) {
            if (stmt->line > 0) currentLine = stmt->line;
            if (resultExpected && &stmt == &node.statements.back()) visitExpecting(stmt.get(), resultExpected);
            else stmt->accept(*this);
            auto stmtType = stmt->getType();
            if (stmtType) {
                lastType = stmtType->clone();
            }
        }
    }
    node.type = lastType ? lastType->clone() : std::make_unique<UnknownType>();
    if (node.isUnsafeContext) {
        --unsafeContextDepth;
    }
    exitOwnershipScope();
    cg.symbolTable.exitScope();
}

static bool copiesIntoTensorElements(ASTNode* target) {
    auto* access = dynamic_cast<ArrayAccessNode*>(target);
    if (!access || !access->array) return false;
    auto container = access->array->getType();
    return container && (container->getName() == "Tensor" || container->getName().rfind("Tensor$", 0) == 0);
}

static bool isRefcountHandleType(const Type* type) {
    return type && type->getKind() == Type::Kind::GENERIC && (type->getName() == "Shared" || type->getName() == "Weak");
}

void TypeChecker::visit(AssignmentNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
    Symbol* target = CodeGenerator::getInstance().symbolTable.lookup(node.name);
    if (target && isRefcountHandleType(target->type.get())) {
        reportError("Type error: a " + target->type->getName() + " handle '" + node.name +
                    "' cannot be reassigned; declare a new binding instead");
    }
}

void TypeChecker::visit(ClassBodyNode& node) {
    for (auto& field : node.fields) {
        if (field) {
            field->accept(*this);
        }
    }
    for (auto& method : node.methods) {
        if (method) {
            method->accept(*this);
        }
    }
}

void TypeChecker::visit(ParameterNode& node) {
    checkTypeResolved(node.getType(), "parameter");
}

// Whether a value of type `source` may be stored where `target` is expected (numeric conversions
// are applied by code generation).
static bool isAssignableType(const Type* target, const Type* source) {
    if (!target || !source || target->getKind() == Type::Kind::UNKNOWN || source->getKind() == Type::Kind::UNKNOWN) return true;
    if (target->getKind() == Type::Kind::VARIABLE || source->getKind() == Type::Kind::VARIABLE) return true;
    if (target->equals(*source) || source->isSubtypeOf(*target)) return true;
    auto numeric = [](const Type* t) { return t->isIntegerTy() || t->isFloatTy() || t->isDoubleTy() || t->getName() == "Real" || t->getName() == "Long"; };
    if (numeric(target) && numeric(source)) return true;
    auto boolean = [](const Type* t) { return t->getName() == "Bool" || t->getName() == "Boolean"; };
    if (boolean(target) && boolean(source)) return true;
    if (target->getName() == source->getName()) return true;
    return false;
}

// Binds the enum's type parameters by matching a declared field type against an argument type.
static void unifyEnumType(const Type* declared, const Type* actual, const EnumInfo& info,
                          std::vector<std::unique_ptr<Type>>& bound) {
    if (!declared || !actual || actual->getKind() == Type::Kind::UNKNOWN) return;
    for (size_t i = 0; i < info.typeParameters.size(); ++i) {
        if (declared->getName() == info.typeParameters[i] &&
            (declared->getKind() == Type::Kind::VARIABLE || declared->getKind() == Type::Kind::CLASS ||
             declared->getKind() == Type::Kind::BASIC)) {
            if (!bound[i]) bound[i] = const_cast<Type*>(actual)->clone();
            return;
        }
    }
    if (auto* generic = dynamic_cast<const GenericType*>(declared)) {
        std::string templateName;
        std::vector<std::unique_ptr<Type>> actualArgs;
        if (actual->getKind() == Type::Kind::CLASS &&
            templateInstanceArguments(actual->getName(), &templateName, &actualArgs) &&
            generic->baseType && templateName == generic->baseType->getName()) {
            for (size_t i = 0; i < generic->typeArguments.size() && i < actualArgs.size(); ++i) {
                unifyEnumType(generic->typeArguments[i].get(), actualArgs[i].get(), info, bound);
            }
        }
        return;
    }
    if (auto* array = dynamic_cast<const ArrayType*>(declared)) {
        if (auto* actualArray = dynamic_cast<const ArrayType*>(actual)) {
            unifyEnumType(array->elementType.get(), actualArray->elementType.get(), info, bound);
        }
    }
}

void TypeChecker::coerceToInterface(std::unique_ptr<ASTNode>& slot, const Type* target) {
    if (!slot || !isInterfaceType(target)) return;
    auto valueType = stripBorrow(slot->getType());
    if (!valueType || valueType->getKind() != Type::Kind::CLASS || valueType->getName() == target->getName()) return;
    if (isInterfaceType(valueType.get())) {
        reportError("Type error: a '" + typeDisplayName(valueType.get()) + "' value is not a '" + target->getName() + "'");
        return;
    }
    if (!implementsInterface(valueType.get(), target->getName())) {
        reportError("Type error: '" + typeDisplayName(valueType.get()) + "' does not implement interface '" +
                    target->getName() + "'");
        return;
    }
    auto wrap = std::make_unique<InterfaceWrapNode>();
    wrap->line = slot->line;
    wrap->className = valueType->getName();
    wrap->interfaceName = target->getName();
    wrap->expression = std::move(slot);
    slot = std::move(wrap);
}

void TypeChecker::checkTemplateBounds(const std::string& templateName, const std::vector<const Type*>& arguments) {
    auto* symbol = dynamic_cast<TemplateSymbol*>(CodeGenerator::getInstance().symbolTable.lookup(templateName));
    if (!symbol) return;
    for (size_t i = 0; i < symbol->templateParams.size() && i < arguments.size(); ++i) {
        const auto& param = symbol->templateParams[i];
        const Type* argument = arguments[i];
        if (!argument || argument->getKind() == Type::Kind::UNKNOWN) continue;
        for (const auto& bound : param.bounds) {
            if (!findInterface(bound)) {
                reportError("Type error: bound '" + bound + "' of " + templateName + "'s parameter " + param.name +
                            " is not an interface");
                continue;
            }
            if (!implementsInterface(argument, bound)) {
                reportError("Type error: " + templateName + "<...> needs its parameter " + param.name + " to implement '" +
                            bound + "', but '" + typeDisplayName(argument) + "' does not");
            }
        }
    }
}

// The class a template instance (`Box$Int`) was made from, or the class itself.
static std::string declaredClassName(const std::string& className) {
    std::string templateName;
    if (templateInstanceArguments(className, &templateName, nullptr)) return templateName;
    return className;
}

CallReport TypeChecker::callReport() {
    auto& table = CodeGenerator::getInstance().symbolTable;
    CallReport report;
    // A template body is checked by the syntactic pass and again in each instance: report a call's
    // error once. The message carries the call's location, so equal errors at other calls stay.
    report.error = [this](const std::string& message) {
        if (reportedCallErrors.insert(message).second) reportError(message);
        else ++errorCount;
    };
    report.isShadowed = [&table](const std::string& id) {
        Symbol* visible = table.lookup(id);
        return visible && visible != table.lookupGlobal(id);
    };
    return report;
}

bool TypeChecker::expandArguments(std::vector<std::unique_ptr<ASTNode>>& args, CallTarget kind, const std::string& owner,
                                  const std::string& name, int line, ASTNode* receiver) {
    return expandCallArguments(args, kind, owner, name, line > 0 ? line : currentLine, callReport(), receiver);
}

void TypeChecker::visitExpecting(ASTNode* node, const Type* expected) {
    if (!node) return;
    const Type* saved = expectedType;
    expectedType = expected;
    node->accept(*this);
    expectedType = saved;
}

void TypeChecker::visit(EnumDeclarationNode& node) {
    reportError("Type error: enum '" + node.name + "' must be declared at the top level");
}

void TypeChecker::visit(EnumConstructNode& node) {
    const Type* expected = expectedType;
    expectedType = nullptr;
    if (hasNamedArguments(node.arguments)) {
        reportError("Type error: the fields of " + node.enumName + "." + node.variantName + " are given in order; "
                    "named arguments are not supported here");
        return;
    }
    const EnumInfo* info = findEnum(node.enumName);
    const EnumVariantInfo* variant = info ? info->variant(node.variantName) : nullptr;
    if (!info || !variant) {
        reportError("Type error: '" + node.enumName + "' has no variant '" + node.variantName + "'");
        return;
    }
    const std::string display = info->name + "." + variant->name;
    if (node.arguments.size() != variant->fieldTypes.size()) {
        reportError("Type error: " + display + " takes " + std::to_string(variant->fieldTypes.size()) +
                    " value(s), got " + std::to_string(node.arguments.size()));
        for (auto& a : node.arguments) if (a) a->accept(*this);
        return;
    }
    const bool generic = !info->typeParameters.empty();
    if (generic && !node.typeArguments.empty() && node.typeArguments.size() != info->typeParameters.size()) {
        reportError("Type error: enum '" + info->name + "' takes " + std::to_string(info->typeParameters.size()) +
                    " type argument(s)");
        return;
    }
    if (generic && node.typeArguments.empty() && expected) {
        std::vector<std::unique_ptr<Type>> expectedArgs;
        if (enumOfType(expected, &expectedArgs) == info) node.typeArguments = std::move(expectedArgs);
    }
    // A generic variant built without type arguments (`Nil` in `Cons(1, Nil)`) takes them from this
    // node's field types, so it is checked after the other arguments have fixed them.
    auto isDeferred = [&](ASTNode* argument) {
        auto* inner = dynamic_cast<EnumConstructNode*>(argument);
        const EnumInfo* innerInfo = inner ? findEnum(inner->enumName) : nullptr;
        return generic && node.typeArguments.empty() && inner && inner->typeArguments.empty() && !inner->type &&
               innerInfo && !innerInfo->typeParameters.empty();
    };
    std::vector<bool> deferred(node.arguments.size(), false);
    auto visitArgument = [&](size_t i) {
        std::unique_ptr<Type> fieldType;
        if (!generic || !node.typeArguments.empty()) {
            fieldType = substituteEnumTypeParameters(variant->fieldTypes[i].get(), *info, node.typeArguments);
        }
        visitExpecting(node.arguments[i].get(), fieldType.get());
        coerceToInterface(node.arguments[i], fieldType.get());
        if (fieldType) narrowToFloatTarget(fieldType.get(), node.arguments[i].get());
    };
    for (size_t i = 0; i < node.arguments.size(); ++i) {
        deferred[i] = isDeferred(node.arguments[i].get());
        if (!deferred[i]) visitArgument(i);
    }
    if (generic && node.typeArguments.empty()) {
        std::vector<std::unique_ptr<Type>> bound(info->typeParameters.size());
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            if (deferred[i]) continue;
            auto argType = node.arguments[i] ? node.arguments[i]->getType() : nullptr;
            unifyEnumType(variant->fieldTypes[i].get(), argType.get(), *info, bound);
        }
        bool allBound = true;
        for (const auto& b : bound) if (!b || !isResolvedType(b)) allBound = false;
        if (allBound) {
            node.typeArguments = std::move(bound);
            for (size_t i = 0; i < node.arguments.size(); ++i) if (deferred[i]) visitArgument(i);
            deferred.assign(deferred.size(), false);
        }
        else {
            // The deferred arguments must then fix the type arguments themselves.
            for (size_t i = 0; i < node.arguments.size(); ++i) {
                if (!deferred[i]) continue;
                visitArgument(i);
                auto argType = node.arguments[i] ? node.arguments[i]->getType() : nullptr;
                unifyEnumType(variant->fieldTypes[i].get(), argType.get(), *info, bound);
            }
        }
        if (!allBound) {
            for (size_t i = 0; i < bound.size(); ++i) {
                if (!bound[i] || !isResolvedType(bound[i])) {
                    reportError("Type error: cannot infer type argument '" + info->typeParameters[i] + "' of " +
                                display + " here; declare the type (val x: " + info->name + "<...> = ...) or write " +
                                info->name + "<...>." + variant->name);
                    return;
                }
            }
            node.typeArguments = std::move(bound);
        }
    }
    for (size_t i = 0; i < node.arguments.size(); ++i) {
        auto fieldType = substituteEnumTypeParameters(variant->fieldTypes[i].get(), *info, node.typeArguments);
        auto argType = node.arguments[i] ? node.arguments[i]->getType() : nullptr;
        if (!isAssignableType(fieldType.get(), argType.get())) {
            reportError("Type error: " + display + " field '" + variant->fieldNames[i] + "' is '" +
                        typeDisplayName(fieldType.get()) + "', got '" + typeDisplayName(argType.get()) + "'");
        }
    }
    node.type = std::make_unique<ClassType>(enumInstanceClass(*info, node.typeArguments));
}

void TypeChecker::visit(TryNode& node) {
    if (node.expression) node.expression->accept(*this);
    auto valueType = node.expression ? node.expression->getType() : nullptr;
    std::vector<std::unique_ptr<Type>> args;
    const EnumInfo* info = enumOfType(valueType.get(), &args);
    if (!info || (info->name != "Option" && info->name != "Result")) {
        reportError("Type error: '?' needs an Option or Result value, got '" + typeDisplayName(valueType.get()) + "'");
        return;
    }
    std::vector<std::unique_ptr<Type>> returnArgs;
    const EnumInfo* returnInfo = enumOfType(currentReturnType, &returnArgs);
    if (returnInfo != info) {
        reportError("Type error: '?' on " + typeDisplayName(valueType.get()) + " needs a function that returns " +
                    (info->name == "Option" ? std::string("an Option") : std::string("a Result")) + ", not '" +
                    typeDisplayName(currentReturnType) + "'");
        return;
    }
    if (info->name == "Result" && args.size() == 2 && returnArgs.size() == 2 && !args[1]->equals(*returnArgs[1])) {
        reportError("Type error: '?' passes on the error of " + typeDisplayName(valueType.get()) +
                    ", but the function returns " + typeDisplayName(currentReturnType) + " (a different error type)");
        return;
    }
    node.functionReturnType = const_cast<Type*>(currentReturnType)->clone();
    node.type = args.empty() ? std::make_unique<UnknownType>() : args[0]->clone();
}

std::unique_ptr<PatternNode> TypeChecker::resolvePattern(ASTNode* raw, const Type* valueType) {
    auto pattern = std::make_unique<PatternNode>();
    pattern->line = raw ? raw->line : 0;
    pattern->valueType = valueType ? const_cast<Type*>(valueType)->clone() : std::make_unique<UnknownType>();
    if (!raw || dynamic_cast<UnitNode*>(raw)) return pattern;  // `_`
    if (auto* resolved = dynamic_cast<PatternNode*>(raw)) return std::make_unique<PatternNode>(*resolved);

    std::vector<std::unique_ptr<Type>> enumArgs;
    const EnumInfo* info = enumOfType(valueType, &enumArgs);

    // The variant a pattern names, and its sub-patterns.
    std::string enumName, variantName;
    std::vector<ASTNode*> subpatterns;
    bool hasArguments = false;
    if (auto* id = dynamic_cast<IdentifierNode*>(raw)) {
        const auto dot = id->value.find('.');
        if (dot != std::string::npos) { enumName = id->value.substr(0, dot); variantName = id->value.substr(dot + 1); }
        else if (info && info->variant(id->value)) variantName = id->value;
        else if (info) {
            // A variant of some other enum is a mistake, not a new variable.
            std::string owner;
            if (const EnumInfo* otherEnum = enumDeclaringVariant(id->value)) owner = otherEnum->name;
            if (!owner.empty()) {
                reportError("Type error: pattern '" + id->value + "' is a variant of enum '" + owner +
                            "', but the matched value is '" + typeDisplayName(valueType) + "'");
                return pattern;
            }
            pattern->kind = PatternNode::Kind::Binding;
            pattern->name = id->value;
            return pattern;
        }
        else {
            // Not an enum: an existing name compares with its value, a new name binds the value.
            if (CodeGenerator::getInstance().symbolTable.lookup(id->value)) {
                pattern->kind = PatternNode::Kind::Literal;
                pattern->literal = raw->clone();
                pattern->literal->accept(*this);
            }
            else {
                pattern->kind = PatternNode::Kind::Binding;
                pattern->name = id->value;
            }
            return pattern;
        }
    }
    else if (auto* access = dynamic_cast<AccessFieldNode*>(raw)) {
        auto* base = dynamic_cast<IdentifierNode*>(access->base.get());
        auto* field = dynamic_cast<IdentifierNode*>(access->field.get());
        if (base && field) { enumName = base->value; variantName = field->value; }
    }
    else if (auto* call = dynamic_cast<FunctionCallNode*>(raw)) {
        variantName = call->functionName;
        hasArguments = true;
        for (auto& a : call->arguments) subpatterns.push_back(a.get());
    }
    else if (auto* method = dynamic_cast<MethodCallNode*>(raw)) {
        if (auto* object = dynamic_cast<IdentifierNode*>(method->object.get())) enumName = object->value;
        variantName = method->methodName;
        hasArguments = true;
        for (auto& a : method->arguments) subpatterns.push_back(a.get());
    }
    else if (auto* construct = dynamic_cast<EnumConstructNode*>(raw)) {
        enumName = construct->enumName;
        variantName = construct->variantName;
        hasArguments = !construct->arguments.empty();
        for (auto& a : construct->arguments) subpatterns.push_back(a.get());
    }
    else if (auto* value = dynamic_cast<ValueNode*>(raw)) {
        pattern->kind = PatternNode::Kind::Literal;
        pattern->literal = raw->clone();
        auto literalType = value->getType();
        const bool isRegex = value->valueType == TokenType::REGEX_LITERAL;
        if (info) {
            reportError("Type error: a literal pattern cannot match a value of enum '" + info->name + "'");
        }
        else if (!(isRegex && valueType && valueType->isStringTy()) && !isAssignableType(valueType, literalType.get()) &&
                 !isAssignableType(literalType.get(), valueType)) {
            reportError("Type error: match case pattern type mismatch: '" + typeDisplayName(literalType.get()) +
                        "' pattern for a '" + typeDisplayName(valueType) + "' value");
        }
        return pattern;
    }

    if (variantName.empty()) {
        reportError("Type error: unsupported pattern in match");
        return pattern;
    }
    if (!info) {
        reportError("Type error: pattern '" + variantName + "' needs an enum value, but the matched value is '" +
                    typeDisplayName(valueType) + "'");
        return pattern;
    }
    if (!enumName.empty() && enumName != info->name) {
        reportError("Type error: pattern " + enumName + "." + variantName + " does not match a value of enum '" +
                    info->name + "'");
        return pattern;
    }
    const EnumVariantInfo* variant = info->variant(variantName);
    if (!variant) {
        reportError("Type error: enum '" + info->name + "' has no variant '" + variantName + "'");
        return pattern;
    }
    pattern->kind = PatternNode::Kind::Variant;
    pattern->enumInstance = valueType ? valueType->getName() : info->name;
    pattern->variantName = variant->name;
    pattern->tag = variant->tag;
    pattern->fieldNames = variant->storageNames;
    for (const auto& t : variant->fieldTypes) {
        pattern->fieldTypes.push_back(substituteEnumTypeParameters(t.get(), *info, enumArgs));
    }
    if (hasArguments && subpatterns.size() != variant->fieldTypes.size()) {
        reportError("Type error: pattern " + info->name + "." + variant->name + " needs " +
                    std::to_string(variant->fieldTypes.size()) + " sub-pattern(s), got " +
                    std::to_string(subpatterns.size()));
        return pattern;
    }
    for (size_t k = 0; k < subpatterns.size(); ++k) {
        pattern->subpatterns.push_back(resolvePattern(subpatterns[k], pattern->fieldTypes[k].get()));
    }
    // `Circle` without sub-patterns matches every Circle.
    while (pattern->subpatterns.size() < pattern->fieldTypes.size()) {
        auto wildcard = std::make_unique<PatternNode>();
        wildcard->valueType = pattern->fieldTypes[pattern->subpatterns.size()]->clone();
        pattern->subpatterns.push_back(std::move(wildcard));
    }
    return pattern;
}

// Pattern-matrix exhaustiveness (Maranget, "Warnings for pattern matching"). Each row is the
// sub-patterns of one unguarded case; returns a value no row matches, written as a pattern, or "".
namespace {
using PatternRow = std::vector<const PatternNode*>;

std::string missingValue(const std::vector<PatternRow>& rows, const std::vector<const Type*>& types) {
    if (types.empty()) return rows.empty() ? "_" : "";
    std::vector<std::unique_ptr<Type>> enumArgs;
    const EnumInfo* info = enumOfType(types[0], &enumArgs);
    std::vector<const Type*> rest(types.begin() + 1, types.end());
    auto restWitness = [&](const std::string& head, const std::string& tail) {
        return tail.empty() ? std::string() : head + (types.size() > 1 ? ", " + tail : std::string());
    };
    std::set<int> present;  // variants named in the first column
    if (info) {
        for (const auto& row : rows) if (row[0]->kind == PatternNode::Kind::Variant) present.insert(row[0]->tag);
    }
    if (!info || present.empty()) {
        // No variant is named here (or the values are not an enum; literals never cover an infinite
        // domain): only rows that accept anything in this column count.
        std::vector<PatternRow> remaining;
        for (const auto& row : rows) {
            if (row[0]->isIrrefutable()) remaining.push_back(PatternRow(row.begin() + 1, row.end()));
        }
        std::string tail = missingValue(remaining, rest);
        if (tail.empty()) return "";
        return types.size() > 1 ? "_, " + tail : "_";
    }
    for (const auto& variant : info->variants) {
        const size_t arity = variant.fieldTypes.size();
        if (!present.count(variant.tag)) {
            // A variant no row names is covered only by the rows that accept anything here.
            std::vector<PatternRow> remaining;
            for (const auto& row : rows) {
                if (row[0]->isIrrefutable()) remaining.push_back(PatternRow(row.begin() + 1, row.end()));
            }
            std::string tail = missingValue(remaining, rest);
            if (tail.empty()) continue;
            std::string head = variant.name;
            if (arity > 0) {
                head += "(";
                for (size_t k = 0; k < arity; ++k) head += (k ? ", _" : "_");
                head += ")";
            }
            return types.size() > 1 ? head + ", " + tail : head;
        }
        std::vector<std::unique_ptr<Type>> fieldTypes;
        for (const auto& t : variant.fieldTypes) fieldTypes.push_back(substituteEnumTypeParameters(t.get(), *info, enumArgs));
        std::vector<const Type*> specializedTypes;
        for (const auto& t : fieldTypes) specializedTypes.push_back(t.get());
        specializedTypes.insert(specializedTypes.end(), rest.begin(), rest.end());
        static const PatternNode wildcard;
        std::vector<PatternRow> specialized;
        for (const auto& row : rows) {
            const PatternNode* head = row[0];
            PatternRow next;
            if (head->isIrrefutable()) {
                for (size_t k = 0; k < arity; ++k) next.push_back(&wildcard);
            }
            else if (head->kind == PatternNode::Kind::Variant && head->tag == variant.tag) {
                for (size_t k = 0; k < arity; ++k) {
                    next.push_back(k < head->subpatterns.size() && head->subpatterns[k] ? head->subpatterns[k].get() : &wildcard);
                }
            }
            else {
                continue;
            }
            next.insert(next.end(), row.begin() + 1, row.end());
            specialized.push_back(std::move(next));
        }
        std::string witness = missingValue(specialized, specializedTypes);
        if (witness.empty()) continue;
        // Split the witness back into this variant's fields and the remaining columns.
        std::vector<std::string> parts;
        int depth = 0;
        std::string current;
        for (char c : witness) {
            if (c == '(') ++depth;
            if (c == ')') --depth;
            if (c == ',' && depth == 0) { parts.push_back(current); current.clear(); continue; }
            if (!(c == ' ' && current.empty())) current += c;
        }
        parts.push_back(current);
        std::string head = variant.name;
        if (arity > 0) {
            head += "(";
            for (size_t k = 0; k < arity && k < parts.size(); ++k) head += (k ? ", " : "") + parts[k];
            head += ")";
        }
        std::string tail;
        for (size_t k = arity; k < parts.size(); ++k) tail += (k > arity ? ", " : "") + parts[k];
        return types.size() > 1 ? head + ", " + (tail.empty() ? "_" : tail) : head;
    }
    return "";
}
}  // namespace

void TypeChecker::visit(MatchExpressionNode& node) {
    const Type* expected = expectedType;
    expectedType = nullptr;
    node.normalizeCaseData();
    if (node.expression) {
        node.expression->accept(*this);
    }
    std::unique_ptr<Type> matchType = node.expression ? stripBorrow(node.expression->getType()) : std::make_unique<UnknownType>();
    const EnumInfo* info = enumOfType(matchType.get());
    std::unique_ptr<Type> resultType;
    std::vector<PatternRow> rows;  // the unguarded patterns, for the exhaustiveness check
    auto& cg = CodeGenerator::getInstance();

    for (size_t i = 0; i < node.cases.size(); ++i) {
        auto& casePair = node.cases[i];
        // Resolve the patterns against the matched type (once; a cloned, already resolved match keeps them).
        casePair.first = resolvePattern(casePair.first.get(), matchType.get());
        for (auto& alternative : node.alternatives[i]) alternative = resolvePattern(alternative.get(), matchType.get());
        auto* first = static_cast<PatternNode*>(casePair.first.get());

        std::vector<std::pair<std::string, const Type*>> bindings;
        first->collectBindings(bindings);
        for (auto& alternative : node.alternatives[i]) {
            std::vector<std::pair<std::string, const Type*>> other;
            static_cast<PatternNode*>(alternative.get())->collectBindings(other);
            if (!other.empty() || !bindings.empty()) {
                reportError("Type error: patterns joined with '|' cannot bind variables");
                break;
            }
        }
        std::set<std::string> seen;
        for (const auto& b : bindings) {
            if (!seen.insert(b.first).second) reportError("Type error: pattern binds '" + b.first + "' twice");
        }

        cg.symbolTable.enterScope();
        enterOwnershipScope();
        for (const auto& b : bindings) {
            std::unique_ptr<Type> boundType = b.second ? const_cast<Type*>(b.second)->clone() : std::make_unique<UnknownType>();
            bindVariable(b.first, boundType, false);
            declareOwnership(b.first, boundType);
        }
        if (node.guards[i]) {
            node.guards[i]->accept(*this);
            auto guardType = node.guards[i]->getType();
            if (guardType && guardType->getName() != "Boolean" && guardType->getName() != "Bool" &&
                isResolvedType(guardType)) {
                reportError("Type error: a match guard must be a Boolean, got '" + typeDisplayName(guardType.get()) + "'");
            }
        }
        if (casePair.second) {
            visitExpecting(casePair.second.get(), expected);
            auto caseResultType = casePair.second->getType();
            if (!resultType) {
                resultType = caseResultType ? caseResultType->clone() : std::make_unique<UnknownType>();
            }
            else if (caseResultType &&
                     !caseResultType->equals(resultType) &&
                     !caseResultType->isSubtypeOf(resultType) &&
                     !resultType->isSubtypeOf(caseResultType)) {
                std::cerr << "Type error: match case result type mismatch" << std::endl;
                errorCount++;
            }
        }
        exitOwnershipScope();
        cg.symbolTable.exitScope();

        if (!node.guards[i]) {
            rows.push_back({first});
            for (auto& alternative : node.alternatives[i]) rows.push_back({static_cast<PatternNode*>(alternative.get())});
        }
    }
    node.patternsResolved = true;

    if (info) {
        const std::string missing = missingValue(rows, {matchType.get()});
        if (!missing.empty()) {
            reportError("Type error: match on '" + typeDisplayName(matchType.get()) + "' is not exhaustive: " + missing +
                        " is not handled (add a case for it or `case _ =>`)");
        }
    }

    node.type = resultType ? resultType->clone() : std::make_unique<UnknownType>();
}

void TypeChecker::visit(RangeExpressionNode& node) {
    if (node.startExpr) {
        node.startExpr->accept(*this);
    }
    if (node.endExpr) {
        node.endExpr->accept(*this);
    }
}

void TypeChecker::visit(ImportNode& node) {
    (void)node;
}

void TypeChecker::visit(UnitNode& node) {
    (void)node;
}

void TypeChecker::visit(CallExpressionNode& node) {
    if (node.callee) {
        node.callee->accept(*this);
    }
    for (auto& arg : node.arguments) {
        if (arg) {
            arg->accept(*this);
        }
    }
    auto calleeType = node.callee ? node.callee->getType() : std::make_unique<UnknownType>();
    if (auto* functionType = dynamic_cast<FunctionType*>(calleeType.get())) {
        std::vector<std::unique_ptr<Type>> parameterTypes;
        for (const auto& parameterType : functionType->parameterTypes) {
            parameterTypes.push_back(parameterType ? parameterType->clone() : std::make_unique<UnknownType>());
        }
        checkFunctionArguments(node.arguments, parameterTypes, "call expression");
    }
    for (auto& arg : node.arguments) {
        bool mutableBorrowArg = false;
        if (isBorrowExpression(arg.get(), &mutableBorrowArg)) {
            releaseBorrow(movedOrBorrowedIdentifier(arg.get()), mutableBorrowArg);
        }
    }
    checkTypeResolved(node.getType(), "call expression");
}

void TypeChecker::visit(MethodCallNode& node) {
    if (node.object) {
        node.object->accept(*this);
    }
    {
        auto& table = CodeGenerator::getInstance().symbolTable;
        CallTarget kind = CallTarget::None;
        std::string owner;
        auto* objectId = dynamic_cast<IdentifierNode*>(node.object.get());
        Symbol* named = objectId ? table.lookup(objectId->value) : nullptr;
        if (named && named->symbolType == SymbolType::NAMESPACE) {
            kind = CallTarget::Namespaced;
            owner = objectId->value;
        }
        else if (objectId && (objectId->value == "this" || objectId->value == "super")) {
            ClassSymbol* cls = table.getEnclosingClassSymbol();
            if (cls && objectId->value == "super") cls = table.lookupClass(cls->superClassName);
            if (cls) {
                kind = CallTarget::Method;
                owner = declaredClassName(cls->name);
            }
        }
        else if (node.object) {
            auto receiverType = stripBorrow(node.object->getType());
            // A class, or an interface (calls through it take the interface's declaration).
            if (receiverType && receiverType->getKind() == Type::Kind::CLASS) {
                kind = CallTarget::Method;
                owner = isInterfaceType(receiverType.get()) ? receiverType->getName() : declaredClassName(receiverType->getName());
            }
        }
        if (!expandArguments(node.arguments, kind, owner, node.methodName, node.line, node.object.get())) return;
    }
    {
        // Parameter types, when every overload of the method with this arity agrees on them.
        std::vector<std::vector<const Type*>> candidates;
        auto receiverType = node.object ? stripBorrow(node.object->getType()) : nullptr;
        if (const InterfaceInfo* iface = isInterfaceType(receiverType.get()) ? findInterface(receiverType->getName()) : nullptr) {
            const int index = iface->methodIndex(node.methodName, node.arguments.size());
            if (index < 0) {
                reportError("Type error: interface '" + iface->name + "' has no method '" + node.methodName + "' taking " +
                            std::to_string(node.arguments.size()) + " argument(s)");
                return;
            }
            const auto& method = iface->methods[static_cast<size_t>(index)];
            for (size_t i = 0; i < node.arguments.size(); ++i) {
                visitExpecting(node.arguments[i].get(), method.parameterTypes[i].get());
                coerceToInterface(node.arguments[i], method.parameterTypes[i].get());
                narrowToFloatTarget(method.parameterTypes[i].get(), node.arguments[i].get());
                auto argType = node.arguments[i]->getType();
                if (!isAssignableType(method.parameterTypes[i].get(), argType.get())) {
                    reportError("Type error: argument " + std::to_string(i + 1) + " of " + iface->name + "." + node.methodName +
                                " is '" + typeDisplayName(method.parameterTypes[i].get()) + "', got '" +
                                typeDisplayName(argType.get()) + "'");
                }
            }
            return;
        }
        if (receiverType && receiverType->getKind() == Type::Kind::CLASS) {
            candidates = methodParameterLists(receiverType->getName(), node.methodName, node.arguments.size());
        }
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            if (!node.arguments[i]) continue;
            const Type* expectedArg = candidates.size() == 1 ? candidates[0][i] : nullptr;
            visitExpecting(node.arguments[i].get(), expectedArg);
            coerceToInterface(node.arguments[i], expectedArg);
        }
    }
    if (node.object) {
        auto receiverType = node.object->getType();
        if (receiverType && receiverType->getKind() == Type::Kind::CLASS) {
            narrowArgumentsToFloat(node.arguments,
                                   methodParameterLists(receiverType->getName(), node.methodName, node.arguments.size()));
        }
        if (isSetType(receiverType.get())) {
            if (!setMethodType(receiverType.get(), node.methodName, node.arguments.size())) {
                reportError("Type error: " + typeDisplayName(receiverType.get()) + " has no method '" + node.methodName +
                            "' taking " + std::to_string(node.arguments.size()) + " argument(s); available: add(x), "
                            "remove(x), discard(x), contains(x), size(), isEmpty(), clear(), copy(), elementAt(i), "
                            "union(s), intersection(s), difference(s), symmetricDifference(s), isSubset(s), "
                            "isSuperset(s), isDisjoint(s), toString()");
                return;
            }
            const std::string& m = node.methodName;
            if ((m == "add" || m == "remove" || m == "discard" || m == "contains" || m == "has") && !node.arguments.empty()) {
                checkDictValue(setElementType(receiverType.get()), node.arguments[0].get(), "set element");
            }
            if (m == "union" || m == "intersection" || m == "difference" || m == "symmetricDifference" ||
                m == "isSubset" || m == "isSuperset" || m == "isDisjoint") {
                auto otherType = node.arguments[0] ? node.arguments[0]->getType() : nullptr;
                if (!isSetType(otherType.get()) || !unifyDictElementTypes(receiverType.get(), otherType.get())) {
                    reportError("Type error: " + m + "() needs a set with the same element type, got '" +
                                typeDisplayName(otherType.get()) + "'");
                }
            }
        }
        else if (isDictType(receiverType.get())) {
            if (node.methodName == "copy" && node.arguments.empty() && !dictCanCopy(receiverType.get())) {
                reportError("Type error: Dict.copy() requires values that can be copied independently");
                return;
            }
            if (!dictMethodType(receiverType.get(), node.methodName, node.arguments.size())) {
                reportError("Type error: " + typeDisplayName(receiverType.get()) + " has no method '" + node.methodName +
                            "' taking " + std::to_string(node.arguments.size()) + " argument(s); available: size(), "
                            "isEmpty(), contains(key), get(key), get(key, default), remove(key), keyAt(i), "
                            "valueAt(i), copy(), clear(), toString()");
                return;
            }
            const std::string& m = node.methodName;
            if ((m == "contains" || m == "has" || m == "containsKey" || m == "remove" || m == "get") &&
                !node.arguments.empty()) {
                checkDictKey(receiverType.get(), node.arguments[0].get());
            }
            if (m == "get" && node.arguments.size() == 2) {
                checkDictValue(dictValueType(receiverType.get()), node.arguments[1].get(), "default value");
            }
        }
    }
    checkTypeResolved(node.getType(), "method call");
}

static bool isIntegerLiteralText(const std::string& text) {
    size_t start = !text.empty() && (text[0] == '+' || text[0] == '-') ? 1 : 0;
    if (start >= text.size()) return false;
    for (size_t i = start; i < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
    }
    return true;
}

void TypeChecker::visit(IdentifierNode& node) {
    checkIdentifierUse(node.value);
    auto* symbol = CodeGenerator::getInstance().symbolTable.lookup(node.value);
    if (!symbol) {
        // A non-type template parameter substituted into an instance's body is its value's text
        // (e.g. `7`); its type comes from IdentifierNode::getType, so do not pin it to Real.
        if (isIntegerLiteralText(node.value) || node.value == "true" || node.value == "false") {
            return;
        }
        node.type = std::make_unique<BasicType>("Real");
        return;
    }
    if (symbol->type && isResolvedType(symbol->type)) {
        return;
    }
    if (symbol->symbolType == SymbolType::NAMESPACE ||
        symbol->symbolType == SymbolType::CLASS ||
        symbol->symbolType == SymbolType::FUNCTION ||
        symbol->symbolType == SymbolType::TEMPLATE) {
        return;
    }
    checkTypeResolved(node.getType(), "identifier '" + node.value + "'");
}

void TypeChecker::visit(ValueNode& node) {
    checkTypeResolved(node.getType(), "value literal");
}

void TypeChecker::visit(FunctionCallNode& node) {
    static int callDepth = 0;
    static bool callNestingExceeded = false;
    if (callNestingExceeded) return;
    if (callDepth >= 256) {
        callNestingExceeded = true;
        reportError("Type error: call to '" + node.functionName +
                    "' nests too deeply (a default argument that calls its own function?)");
        return;
    }
    struct DepthGuard {
        int& depth;
        explicit DepthGuard(int& d) : depth(d) { ++depth; }
        ~DepthGuard() { --depth; }
    } depthGuard(callDepth);
    std::vector<std::unique_ptr<Type>> parameterTypes;
    auto& cg = CodeGenerator::getInstance();
    {
        // A declared function (not a closure variable of that name), else a method of the
        // enclosing class called without `this.`.
        CallTarget kind = CallTarget::None;
        std::string owner;
        Symbol* target = cg.symbolTable.lookup(node.functionName);
        if (target && (target->symbolType == SymbolType::FUNCTION || target->symbolType == SymbolType::TEMPLATE)) {
            kind = CallTarget::Function;
        }
        else if (!target || (target->symbolType != SymbolType::VARIABLE && target->symbolType != SymbolType::FIELD)) {
            if (ClassSymbol* cls = cg.symbolTable.getEnclosingClassSymbol()) {
                const std::string className = declaredClassName(cls->name);
                if (hasMethodSignatures(className, node.functionName)) {
                    kind = CallTarget::Method;
                    owner = className;
                }
            }
        }
        if (!expandArguments(node.arguments, kind, owner, node.functionName, node.line)) return;
    }
    if (auto* symbol = cg.symbolTable.lookup(node.functionName)) {
        if (auto* funcType = dynamic_cast<FunctionType*>(symbol->type.get())) {
            for (const auto& parameterType : funcType->parameterTypes) {
                parameterTypes.push_back(parameterType ? parameterType->clone() : std::make_unique<UnknownType>());
            }
        }
    }
    for (size_t i = 0; i < node.arguments.size(); ++i) {
        if (node.arguments[i]) {
            const Type* expectedArg = parameterTypes.size() == node.arguments.size() ? parameterTypes[i].get() : nullptr;
            visitExpecting(node.arguments[i].get(), expectedArg);
            coerceToInterface(node.arguments[i], expectedArg);
        }
    }
    if (!parameterTypes.empty()) {
        checkFunctionArguments(node.arguments, parameterTypes, node.functionName);
    }
    if (node.functionName == "print" || node.functionName == "println") {
        for (auto& arg : node.arguments) wrapInToString(arg, *this);
    }
    if (node.functionName == "Shared" && node.arguments.size() == 1 && node.arguments[0] &&
        !dynamic_cast<FunctionType*>(cg.symbolTable.lookup("Shared") ? cg.symbolTable.lookup("Shared")->type.get() : nullptr)) {
        auto payload = node.arguments[0]->getType();
        if (payload) payload = stripBorrow(payload);
        if (payload && !isCopyType(payload.get())) {
            reportError("Type error: Shared<" + payload->getName() + "> cannot hold an owned value; a Shared payload "
                        "must be immutable (numbers, String, struct, enum, interface) so shared values cannot form cycles");
        }
    }
    if (auto* tmpl = dynamic_cast<TemplateSymbol*>(cg.symbolTable.lookup(node.functionName))) {
        auto* decl = dynamic_cast<FunctionDeclarationNode*>(tmpl->declaration.get());
        std::vector<std::unique_ptr<Type>> owned(tmpl->templateParams.size());
        for (size_t i = 0; i < node.explicitTypeArgs.size() && i < owned.size(); ++i) {
            if (node.explicitTypeArgs[i]) owned[i] = node.explicitTypeArgs[i]->clone();
        }
        // Infer the rest from parameters declared with a bare type parameter.
        for (size_t p = 0; decl && p < decl->parameters.size() && p < node.arguments.size(); ++p) {
            auto* param = dynamic_cast<ParameterNode*>(decl->parameters[p].get());
            if (!param || !param->type) continue;
            for (size_t t = 0; t < tmpl->templateParams.size(); ++t) {
                if (!owned[t] && param->type->getName() == tmpl->templateParams[t].name && node.arguments[p]) {
                    owned[t] = node.arguments[p]->getType();
                }
            }
        }
        std::vector<const Type*> arguments;
        for (const auto& o : owned) arguments.push_back(o.get());
        checkTemplateBounds(node.functionName, arguments);
    }
    // spawn(f): processes share nothing. The body must be a zero-argument lambda literal that
    // captures by value (its environment is copied into the child) or a named zero-argument function.
    if ((node.functionName == "spawn" || node.functionName == "threadStart") &&
        isNativeMicrothreadFunction(node.functionName)) {
        const std::string fn = node.functionName;
        if (node.arguments.size() != 1) {
            reportError("Type error: " + fn + "() takes exactly one argument (a zero-argument lambda or function)");
        }
        else if (auto* lambda = dynamic_cast<LambdaExpressionNode*>(node.arguments[0].get())) {
            if (lambda->capturesByReference) {
                reportError("Type error: " + fn + "() lambda may not capture by reference ([&]); processes share nothing, capture by value with [=]");
            }
            if (!lambda->arguments.empty()) {
                reportError("Type error: " + fn + "() lambda must take no parameters; capture its inputs instead");
            }
        }
        else if (auto* ident = dynamic_cast<IdentifierNode*>(node.arguments[0].get())) {
            auto* symbol = cg.symbolTable.lookup(ident->value);
            auto* fnType = symbol ? dynamic_cast<FunctionType*>(symbol->type.get()) : nullptr;
            if (!symbol || symbol->symbolType != SymbolType::FUNCTION || !fnType) {
                reportError("Type error: " + fn + "() expects a lambda literal or the name of a function, not '" + ident->value + "'");
            }
            else if (!fnType->parameterTypes.empty()) {
                reportError("Type error: " + fn + "() function '" + ident->value + "' must take no parameters");
            }
        }
        else {
            reportError("Type error: " + fn + "() expects a lambda literal or the name of a function");
        }
    }
    if (node.functionName == "free" && node.arguments.size() == 1) {
        const std::string name = identifierName(node.arguments[0].get());
        if (name.empty()) {
            reportError("Type error: free() requires a named owned value");
        }
        else {
            auto* state = findOwnership(name);
            if (!state || !state->owned) {
                reportError("Type error: free() requires an owned value");
            }
            else {
                markMoved(name);
            }
        }
    }
    for (auto& arg : node.arguments) {
        bool mutableBorrowArg = false;
        if (isBorrowExpression(arg.get(), &mutableBorrowArg)) {
            releaseBorrow(movedOrBorrowedIdentifier(arg.get()), mutableBorrowArg);
        }
    }
    checkTypeResolved(node.getType(), "function call '" + node.functionName + "'");
}

void TypeChecker::visit(ArrayAccessNode& node) {
    if (node.array) {
        node.array->accept(*this);
    }
    if (node.index) {
        node.index->accept(*this);
    }
    for (auto& spec : node.indices) {
        if (!spec) continue;
        if (spec->index) spec->index->accept(*this);
        if (spec->start) spec->start->accept(*this);
        if (spec->end) spec->end->accept(*this);
        if (spec->step) spec->step->accept(*this);
    }
    if (node.array) {
        auto containerType = node.array->getType();
        if (isDictType(containerType.get())) {
            ASTNode* keyExpr = node.index ? node.index.get()
                : (!node.indices.empty() && node.indices[0] ? node.indices[0]->index.get() : nullptr);
            if (node.indices.size() > 1 || !keyExpr) {
                reportError("Type error: a dictionary is indexed by exactly one key: d[key]");
                return;
            }
            checkDictKey(containerType.get(), keyExpr);
            return;
        }
    }
    checkTypeResolved(node.getType(), "array access");
}

void TypeChecker::checkDictKey(const Type* dictType, ASTNode* keyExpr) {
    const Type* keyType = dictKeyType(dictType);
    auto actual = keyExpr ? keyExpr->getType() : nullptr;
    if (!keyType || !actual || !isResolvedType(actual)) {
        return;
    }
    auto unified = unifyDictElementTypes(keyType, actual.get());
    if (!unified || !unified->equals(*keyType)) {
        reportError("Type error: dictionary key of type '" + typeDisplayName(actual.get()) +
                    "' does not match the key type of " + typeDisplayName(dictType));
    }
}

void TypeChecker::checkDictValue(const Type* valueType, ASTNode* valueExpr, const std::string& context) {
    auto actual = valueExpr ? valueExpr->getType() : nullptr;
    if (!valueType || !actual || !isResolvedType(actual)) {
        return;
    }
    auto unified = unifyDictElementTypes(valueType, actual.get());
    if (!unified || !unified->equals(*valueType)) {
        reportError("Type error: " + context + " of type '" + typeDisplayName(actual.get()) +
                    "' does not fit the " + (context.rfind("set", 0) == 0 ? "set element" : "dictionary value") +
                    " type '" + typeDisplayName(valueType) + "'");
    }
}

std::string TypeChecker::typeDisplayName(const Type* type) {
    if (!type) return "?";
    if (auto* generic = dynamic_cast<const GenericType*>(type)) {
        std::string text = generic->getName();
        const bool squareBrackets = text == "Vector" || text == "Array";
        text += squareBrackets ? "[" : "<";
        for (size_t i = 0; i < generic->typeArguments.size(); ++i) {
            if (i > 0) text += ", ";
            text += typeDisplayName(generic->typeArguments[i].get());
        }
        text += squareBrackets ? "]" : ">";
        return text;
    }
    if (auto* array = dynamic_cast<const ArrayType*>(type)) {
        return "Array[" + typeDisplayName(array->elementType.get()) + "]";
    }
    // A template instance class (`Option$Int`) reads as the type it was written as (`Option<Int>`).
    std::string templateName;
    std::vector<std::unique_ptr<Type>> arguments;
    if (type->getKind() == Type::Kind::CLASS && templateInstanceArguments(type->getName(), &templateName, &arguments)) {
        std::string text = templateName + "<";
        for (size_t i = 0; i < arguments.size(); ++i) text += (i ? ", " : "") + typeDisplayName(arguments[i].get());
        return text + ">";
    }
    return type->getName();
}

void TypeChecker::visit(DictLiteralNode& node) {
    if (isSetType(node.type.get()) || node.isSet) {
        for (auto& element : node.keys) {
            if (element) element->accept(*this);
        }
        if (isSetType(node.type.get())) {
            if (!node.isSet && !node.keys.empty()) {
                reportError("Type error: a dictionary literal cannot initialize " + typeDisplayName(node.type.get()));
                return;
            }
            const Type* elementType = setElementType(node.type.get());
            for (auto& element : node.keys) {
                checkDictValue(elementType, element.get(), "set element");
            }
            return;
        }
        if (!isSetType(node.getType().get())) {
            reportError("Type error: " + node.inferenceError());
        }
        return;
    }
    if (node.isSet == false && node.type && !isDictType(node.type.get())) {
        node.type.reset();
    }
    const bool expected = isDictType(node.type.get());
    const Type* valueType = expected ? dictValueType(node.type.get()) : nullptr;
    for (size_t i = 0; i < node.keys.size(); ++i) {
        if (expected && isDictType(valueType)) {
            if (auto* nested = dynamic_cast<DictLiteralNode*>(node.values[i].get())) {
                nested->type = cloneType(valueType);
            }
        }
        if (node.keys[i]) node.keys[i]->accept(*this);
        if (node.values[i]) node.values[i]->accept(*this);
    }
    if (expected) {
        for (size_t i = 0; i < node.keys.size(); ++i) {
            checkDictKey(node.type.get(), node.keys[i].get());
            checkDictValue(valueType, node.values[i].get(), "dictionary value");
        }
        return;
    }
    if (!isDictType(node.getType().get())) {
        reportError("Type error: " + node.inferenceError());
    }
}

void TypeChecker::visit(ArrayCreationNode& node) {
    for (auto& element : node.elements) {
        if (element) {
            element->accept(*this);
        }
    }
    checkTypeResolved(node.getType(), "array creation");
}

void TypeChecker::visit(AccessFieldNode& node) {
    if (node.base) {
        node.base->accept(*this);
    }
    if (node.field) {
        node.field->accept(*this);
    }
    checkTypeResolved(node.getType(), "field access");
}

void TypeChecker::visit(ClassInstanceCreationNode& node) {
    if (!expandArguments(node.arguments, CallTarget::Constructor, node.className, "", node.line)) return;
    if (!node.templateArgs.empty()) {
        std::vector<const Type*> arguments;
        for (const auto& a : node.templateArgs) arguments.push_back(a.get());
        checkTemplateBounds(node.className, arguments);
    }
    {
        auto created = node.getType();
        std::vector<std::vector<const Type*>> candidates;
        if (created && created->getKind() == Type::Kind::CLASS) {
            candidates = constructorParameterLists(created->getName(), node.arguments.size());
        }
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            if (!node.arguments[i]) continue;
            visitExpecting(node.arguments[i].get(), candidates.size() == 1 ? candidates[0][i] : nullptr);
            coerceToInterface(node.arguments[i], candidates.size() == 1 ? candidates[0][i] : nullptr);
        }
    }
    {
        auto created = node.getType();
        if (created && created->getKind() == Type::Kind::CLASS) {
            narrowArgumentsToFloat(node.arguments, constructorParameterLists(created->getName(), node.arguments.size()));
        }
    }
    checkTypeResolved(node.getType(), "class instance creation");
}

void TypeChecker::visit(ArrayCreationExpressionNode& node) {
    for (auto& sizeExpr : node.sizes) {
        if (sizeExpr) {
            sizeExpr->accept(*this);
        }
    }
    checkTypeResolved(node.getType(), "array creation expression");
}

void TypeChecker::visit(AssignmentExpressionNode& node) {
    if (node.left) {
        node.left->accept(*this);
    }
    // `d = {...}` / `d = {}` for a Dict variable, and `d[k] = {...}` for a nested Dict value, take the
    // target's type.
    if (auto* dictLiteral = dynamic_cast<DictLiteralNode*>(node.right.get())) {
        auto leftType = node.left ? node.left->getType() : nullptr;
        if ((isDictType(leftType.get()) || isSetType(leftType.get())) && !dictLiteral->type) {
            dictLiteral->type = leftType->clone();
        }
    }
    if (node.right) {
        auto leftType = node.left ? node.left->getType() : nullptr;
        visitExpecting(node.right.get(), leftType.get());
        coerceToInterface(node.right, leftType.get());
        narrowToFloatTarget(leftType.get(), node.right.get());
    }
    auto rightType = node.right ? node.right->getType() : std::make_unique<UnknownType>();
    if (auto* access = dynamic_cast<ArrayAccessNode*>(node.left.get())) {
        auto containerType = access->array ? access->array->getType() : nullptr;
        if (isDictType(containerType.get()) && (node.op == "=" || node.op == "<-")) {
            checkDictValue(dictValueType(containerType.get()), node.right.get(), "assigned value");
        }
    }
    const std::string leftName = identifierName(node.left.get());
    const std::string rightName = movedOrBorrowedIdentifier(node.right.get());
    const bool rightIsMoveChecked = rightType && isMoveCheckedType(rightType.get());
    if (!leftName.empty() && dynamic_cast<IdentifierNode*>(node.left.get())) {
        auto leftType = node.left->getType();
        if (isRefcountHandleType(leftType.get())) {
            reportError("Type error: a " + leftType->getName() + " handle '" + leftName +
                        "' cannot be reassigned; declare a new binding instead");
        }
    }
    if (node.op == "<-") {
        if (!rightName.empty()) {
            markMoved(rightName);
        }
        if (!leftName.empty()) {
            auto* leftState = findOwnership(leftName);
            if (leftState) {
                leftState->moved = false;
            }
        }
    }
    else if (rightIsMoveChecked && !rightName.empty() && !copiesIntoTensorElements(node.left.get()) &&
             !isMoveExpression(node.right.get())) {
        reportError("Type error: ownership transfer requires '<-'");
    }
    checkTypeResolved(node.getType(), "assignment expression");
}

void TypeChecker::visit(UnaryExpressionNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
    if (node.op == "*") {
        auto exprType = node.expression ? node.expression->getType() : std::make_unique<UnknownType>();
        if (!exprType || exprType->getKind() != Type::Kind::UNSAFE_POINTER) {
            reportError("Type error: raw pointer dereference requires unsafe pointer");
        }
        else if (unsafeContextDepth == 0) {
            reportError("Type error: raw pointer dereference requires unsafe context");
        }
    }
    checkTypeResolved(node.getType(), "unary expression");
}

void TypeChecker::visit(CastingExpressionNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
    if (node.typeNode) {
        node.typeNode->accept(*this);
    }
    checkTypeResolved(node.getType(), "casting expression");
}

void TypeChecker::visit(PostfixExpressionNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
    node.type = node.expression ? node.expression->getType() : std::make_unique<UnknownType>();
    checkTypeResolved(node.type, "postfix expression");
}

void TypeChecker::visit(PrefixExpressionNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
    const std::string sourceName = movedOrBorrowedIdentifier(&node);
    if (node.op == "<-") {
        if (auto* id = dynamic_cast<IdentifierNode*>(node.expression.get())) {
            Symbol* source = CodeGenerator::getInstance().symbolTable.lookup(id->value);
            if (source && source->type && (source->type->getKind() == Type::Kind::BORROW ||
                                           source->type->getKind() == Type::Kind::MUTABLE_BORROW)) {
                reportError("Type error: cannot move out of the borrow '" + id->value +
                            "'; pass it on with '&' or copy it with clone(...)");
            }
        }
        if (isStrictOwnership() && isFieldPath(node.expression.get())) {
            auto movedType = node.expression->getType();
            if (movedType && isMoveCheckedType(movedType.get())) {
                reportError("Type error: cannot move out of the field '" + fieldPathText(node.expression.get()) +
                            "'; borrow it with '&' or copy it with clone(...)");
            }
        }
        if (!sourceName.empty()) {
            markMoved(sourceName);
        }
    }
    else if (node.op == "&" || node.op == "&mut") {
        if (!sourceName.empty()) {
            const size_t dot = sourceName.find('.');
            markBorrowed(dot == std::string::npos ? sourceName : sourceName.substr(0, dot), node.op == "&mut");
        }
    }
    else if (node.op == "async" || node.op == "go") {
        // A concurrent call shares nothing with its caller: the operand must be a direct call of a
        // named function, and no argument may be a borrow (the task may outlive the borrow).
        auto* call = dynamic_cast<FunctionCallNode*>(node.expression.get());
        auto& cg = CodeGenerator::getInstance();
        auto* symbol = call ? cg.symbolTable.lookup(call->functionName) : nullptr;
        if (!call) {
            reportError("Type error: '" + node.op + "' must be followed by a function call, e.g. '" + node.op + " work(1)'");
        }
        else if (!symbol || (symbol->symbolType != SymbolType::FUNCTION && symbol->symbolType != SymbolType::TEMPLATE)) {
            reportError("Type error: '" + node.op + "' requires a call of a named function, not '" + call->functionName + "'");
        }
        else {
            for (auto& arg : call->arguments) {
                if (isBorrowExpression(arg.get(), nullptr)) {
                    reportError("Type error: '" + node.op + " " + call->functionName +
                                "(...)' cannot take a borrow; tasks share nothing, pass values");
                }
            }
        }
        node.type = node.getType();
        checkTypeResolved(node.type, "'" + node.op + "' expression");
        return;
    }
    else if (node.op == "await") {
        const bool awaitsFuture = node.expression &&
            dynamic_cast<GenericType*>(node.expression->getType().get()) &&
            node.expression->getType()->getName() == "Future";
        if (!inAsyncFunction && !awaitsFuture) {
            reportError("Type error: 'await' is only valid inside an 'async def' function");
        }
        // M8: a borrow may not be held live across an await point. After suspension another task may
        // run, so a reference into a value that could be mutated or dropped meanwhile is unsound.
        // Any &/&mut still active in scope when we await is rejected.
        for (const auto& scope : ownershipScopes) {
            for (const auto& entry : scope) {
                if (entry.second.immutableBorrows > 0 || entry.second.mutableBorrowed) {
                    reportError("Type error: borrow of '" + entry.first +
                                "' is held across an 'await'; end the borrow before awaiting "
                                "(a reference cannot survive suspension)");
                }
            }
        }
    }
    node.type = node.expression ? node.expression->getType() : std::make_unique<UnknownType>();
    if (node.op == "await") {
        node.type = node.getType();
    }
    if (node.op == "&") {
        node.type = std::make_unique<BorrowType>(node.type, false);
    }
    else if (node.op == "&mut") {
        node.type = std::make_unique<BorrowType>(node.type, true);
    }
    checkTypeResolved(node.type, "prefix expression");
}

void TypeChecker::visit(AttributeNode& node) {
    if (auto* function = dynamic_cast<FunctionDeclarationNode*>(node.target.get())) {
        if (function->isIntrinsic) {
            return;
        }
    }
    if (node.target) {
        node.target->accept(*this);
    }
}

void TypeChecker::visit(LoopControlNode& node) {
    if (loopDepth == 0) {
        reportError(std::string("Type error: '") + (node.isBreak ? "break" : "continue") +
                    "' outside a while or for loop");
    }
}

void TypeChecker::visit(LambdaExpressionNode& node) {
    auto& cg = CodeGenerator::getInstance();
    const int savedLoopDepth = loopDepth;
    loopDepth = 0;
    struct RestoreLoopDepth { int& depth; int saved; ~RestoreLoopDepth() { depth = saved; } } restoreLoopDepth{loopDepth, savedLoopDepth};
    if (!node.capturesByReference && node.body) {
        std::set<std::string> captured;
        ast_walk::walk(node.body.get(), nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
            if (auto* id = dynamic_cast<IdentifierNode*>(n)) {
                const std::string root = id->value.substr(0, id->value.find('.'));
                OwnershipState* state = findOwnership(root);
                if (state && state->owned && !state->moved) captured.insert(root);
            }
            return true;
        });
        for (const auto& name : captured) {
            if (node.escapes) markMoved(name);
            else markBorrowed(name, false);
        }
    }
    cg.symbolTable.enterScope();
    enterOwnershipScope();
    for (auto& capture : node.captureVariables) {
        if (capture.empty() || capture == "&" || capture == "=") {
            continue;
        }
        bindVariable(capture, std::make_unique<BasicType>("Int"), false);
    }
    for (auto& arg : node.arguments) {
        if (arg) {
            auto* paramNode = dynamic_cast<ParameterNode*>(arg.get());
            if (paramNode && !isResolvedType(paramNode->type)) {
                paramNode->type = std::make_unique<BasicType>("Int");
            }
            arg->accept(*this);
            if (paramNode) {
                bindVariable(paramNode->name, paramNode->type, false);
                declareOwnership(paramNode->name, paramNode->type);
            }
        }
    }
    if (node.body) {
        node.body->accept(*this);
    }
    exitOwnershipScope();
    cg.symbolTable.exitScope();
    node.type = node.getType();
}

void TypeChecker::visit(MapStatementNode& node) {
    auto& cg = CodeGenerator::getInstance();
    std::unique_ptr<Type> elementType = std::make_unique<UnknownType>();
    if (node.iterableExpr) {
        node.iterableExpr->accept(*this);
        elementType = collectionElementType(node.iterableExpr->getType());
    }
    cg.symbolTable.enterScope();
    enterOwnershipScope();
    bindVariable(node.variable, elementType, false);
    declareOwnership(node.variable, elementType);
    if (node.body) {
        node.body->accept(*this);
    }
    auto resolved = node.getType();
    if (!isResolvedType(resolved)) {
        std::vector<std::unique_ptr<Type>> typeArgs;
        typeArgs.push_back(std::make_unique<BasicType>("Real"));
        node.type = std::make_unique<GenericType>(std::make_unique<BasicType>("Array"), typeArgs);
    }
    exitOwnershipScope();
    cg.symbolTable.exitScope();
}

void TypeChecker::visit(PMapStatementNode& node) {
    auto& cg = CodeGenerator::getInstance();
    std::unique_ptr<Type> elementType = std::make_unique<UnknownType>();
    if (node.iterableExpr) {
        node.iterableExpr->accept(*this);
        elementType = collectionElementType(node.iterableExpr->getType());
    }
    if (node.backend != "cpu" && node.backend != "openmp" && node.backend != "gpu" && node.backend != "simd") {
        reportError("Type error: unsupported pmap backend '" + node.backend + "'");
    }
    // `gpu` type-checks like any other backend; it currently falls back to CPU execution at codegen
    // (no device code generator yet), which is a performance choice, not a semantic one.
    cg.symbolTable.enterScope();
    enterOwnershipScope();
    bindVariable(node.variable, elementType, false);
    declareOwnership(node.variable, elementType);
    if (node.body) {
        node.body->accept(*this);
    }
    auto resolved = node.getType();
    if (!isResolvedType(resolved)) {
        std::vector<std::unique_ptr<Type>> typeArgs;
        typeArgs.push_back(std::make_unique<BasicType>("Real"));
        node.type = std::make_unique<GenericType>(std::make_unique<BasicType>("Array"), typeArgs);
    }
    exitOwnershipScope();
    cg.symbolTable.exitScope();
}

void TypeChecker::visit(ReduceStatementNode& node) {
    auto& cg = CodeGenerator::getInstance();
    std::unique_ptr<Type> elementType = std::make_unique<UnknownType>();
    if (node.iterableExpr) {
        node.iterableExpr->accept(*this);
        elementType = collectionElementType(node.iterableExpr->getType());
    }
    if (node.isParallel) {
        if (node.backend != "cpu" && node.backend != "simd" && node.backend != "openmp" && node.backend != "gpu") {
            reportError("Type error: unsupported preduce backend '" + node.backend + "'");
        }
        if (node.backend == "openmp" || node.backend == "gpu") {
            reportError("Type error: preduce(" + node.backend + ") is reserved but parallel reduction lowering is not implemented yet");
        }
    }
    cg.symbolTable.enterScope();
    enterOwnershipScope();
    bindVariable(node.variable, elementType, false);
    declareOwnership(node.variable, elementType);
    std::unique_ptr<Type> accType = node.initialValue ? node.initialValue->getType() : std::make_unique<UnknownType>();
    bindVariable(node.accumulatorVariable, accType, true);
    declareOwnership(node.accumulatorVariable, accType);
    if (node.body) {
        node.body->accept(*this);
    }
    if (node.initialValue) {
        node.initialValue->accept(*this);
    }
    checkTypeResolved(node.getType(), "reduce statement");
    exitOwnershipScope();
    cg.symbolTable.exitScope();
}

void TypeChecker::visit(FilterStatementNode& node) {
    auto& cg = CodeGenerator::getInstance();
    std::unique_ptr<Type> elementType = std::make_unique<UnknownType>();
    if (node.iterableExpr) {
        node.iterableExpr->accept(*this);
        elementType = collectionElementType(node.iterableExpr->getType());
    }
    cg.symbolTable.enterScope();
    enterOwnershipScope();
    bindVariable(node.variable, elementType, false);
    declareOwnership(node.variable, elementType);
    if (node.body) {
        node.body->accept(*this);
    }
    auto resolved = node.getType();
    if (!isResolvedType(resolved)) {
        std::vector<std::unique_ptr<Type>> typeArgs;
        typeArgs.push_back(std::make_unique<BasicType>("Real"));
        node.type = std::make_unique<GenericType>(std::make_unique<BasicType>("Array"), typeArgs);
    }
    exitOwnershipScope();
    cg.symbolTable.exitScope();
}

void TypeChecker::checkContractClause(ContractClause& clause, const std::string& kind, bool boolean) {
    if (!clause.expression) return;
    clause.expression->accept(*this);
    auto clauseType = clause.expression->getType();
    if (!isResolvedType(clauseType)) return;
    const bool isBool = clauseType->getName() == "Boolean" || clauseType->getName() == "Bool";
    if (boolean && !isBool) {
        reportError("Type error: '" + kind + " " + clause.text + "' must be a Bool condition, not '" +
                    clauseType->getName() + "'");
    }
    if (!boolean && !clauseType->isIntegerTy()) {
        reportError("Type error: '" + kind + " " + clause.text + "' must be an integer measure, not '" +
                    clauseType->getName() + "'");
    }
}

void TypeChecker::visitLoopContracts(ASTNode& loop) {
    if (!loop.contracts) return;
    for (auto& clause : loop.contracts->invariants) checkContractClause(clause, "invariant", true);
    for (auto& clause : loop.contracts->decreases) checkContractClause(clause, "decreases", false);
}

void TypeChecker::visit(WhileStatementNode& node) {
    if (node.condition) {
        node.condition->accept(*this);
        checkTypeResolved(node.condition->getType(), "while condition");
    }
    visitLoopContracts(node);
    if (node.body) {
        // Path-sensitivity for loops (M3). A while body that always returns runs at most once and
        // diverges on that run, so any moves it makes never reach the code after the loop — the loop
        // can only fall through when the condition is false, i.e. the body never executed. Discard
        // those moves, mirroring the diverging-if-arm handling. A non-diverging body keeps its moves
        // (a value moved in a body that can loop is conservatively treated as moved afterward).
        const auto beforeBody = ownershipScopes;
        visitLoopBody(node.body.get(), ownershipScopes.size());
        // A `break` leaves the loop with the body's moves in effect, so such a body is not diverging.
        if (statementDiverges(node.body.get()) && !containsLoopExit(node.body.get())) {
            ownershipScopes = beforeBody;
        }
    }
}

void TypeChecker::visit(ArrayLiteralNode& node) {
    const Type* expected = expectedType;
    expectedType = nullptr;
    const Type* expectedElement = nullptr;
    if (auto* array = dynamic_cast<const ArrayType*>(expected)) expectedElement = array->elementType.get();
    else if (auto* generic = dynamic_cast<const GenericType*>(expected)) {
        const std::string base = generic->baseType ? generic->baseType->getName() : "";
        if ((base == "Array" || base == "Vector") && !generic->typeArguments.empty()) expectedElement = generic->typeArguments[0].get();
    }
    for (auto& element : node.elements) {
        if (element) {
            visitExpecting(element.get(), expectedElement);
            coerceToInterface(element, expectedElement);
        }
    }
    if (isInterfaceType(expectedElement) && !node.elements.empty()) {
        std::unique_ptr<Type> elementType = const_cast<Type*>(expectedElement)->clone();
        node.type = std::make_unique<ArrayType>(elementType, static_cast<int>(node.elements.size()));
    }
    checkTypeResolved(node.getType(), "array literal");
}

void TypeChecker::visit(TupleExpressionNode& node) {
    // Elements are evaluated left-to-right. Returning an owned value moves it, so a value used
    // twice in one tuple (`return buffer, buffer`) is reported as a use-after-move on the second
    // occurrence; the fix is `clone(...)` or `Shared<T>`.
    for (auto& element : node.elements) {
        if (!element) continue;
        element->accept(*this);
        // A tuple is only ever a multiple-return value, so a borrow element (`return value, &value`
        // / `return owner, owner.ref`) escapes the function — reject it (memory model §6, §4.3).
        bool isMutableBorrow = false;
        if (isBorrowExpression(element.get(), &isMutableBorrow)) {
            const std::string borrowed = movedOrBorrowedIdentifier(element.get());
            reportError("Type error: reference to local '" + borrowed +
                        "' escapes as a returned value; return the value itself or clone(" +
                        borrowed + ")");
            continue;
        }
        const std::string name = movedOrBorrowedIdentifier(element.get());
        if (!name.empty()) {
            auto* state = findOwnership(name);
            if (state && state->owned) {
                markMoved(name);
            }
        }
    }
    checkTypeResolved(node.getType(), "tuple expression");
}

void TypeChecker::visit(DestructuringAssignmentNode& node) {
    if (node.value) {
        node.value->accept(*this);
    }
    // Step 3 of the destructuring protocol: reject aliased destinations (`a, a = f()`).
    for (size_t i = 0; i < node.targets.size(); ++i) {
        if (node.targets[i].name == "_") continue;
        for (size_t j = i + 1; j < node.targets.size(); ++j) {
            if (node.targets[j].name == node.targets[i].name) {
                reportError("Type error: destructuring destination '" + node.targets[i].name +
                            "' is assigned more than once");
            }
        }
    }
    auto rhsType = node.value ? node.value->getType() : std::make_unique<UnknownType>();
    auto* tupleType = dynamic_cast<TupleType*>(rhsType.get());
    if (!tupleType) {
        reportError("Type error: the right-hand side of a destructuring assignment must return "
                    "multiple values");
        return;
    }
    if (tupleType->elementTypes.size() != node.targets.size()) {
        reportError("Type error: destructuring binds " + std::to_string(node.targets.size()) +
                    " names but the right-hand side returns " +
                    std::to_string(tupleType->elementTypes.size()) + " values");
    }
    for (size_t i = 0; i < node.targets.size() && i < tupleType->elementTypes.size(); ++i) {
        if (node.targets[i].name == "_") continue;
        bindVariable(node.targets[i].name, tupleType->elementTypes[i], node.targets[i].isMutable);
        declareOwnership(node.targets[i].name, tupleType->elementTypes[i]);
    }
}

void TypeChecker::visit(TemplateDeclarationNode& node) {
    if (node.templateParams.empty() && node.typeParameters.empty()) {
        reportError("Type error: Template declaration has no type parameters");
    }
    ensureTemplateSymbol(node);
}

bool typeIsCopy(const Type* type) { return isCopyType(type); }
