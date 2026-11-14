// type_checker.cpp

#include "type_checker.h"
#include "codegen.h"
#include "symbol.h"
#include "type_utils.h"
#include "dict_support.h"

#include <iostream>

#include "all_ast.h"

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
    cg.symbolTable.addSymbol(node.name, std::move(symbol));
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
    cg.symbolTable.addSymbol(symbolName, std::move(symbol));
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
    cg.symbolTable.addSymbol(node.name, std::move(symbol));
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
            return isStructClassType(type) || type->getName() == "Nat" || type->getName() == "Complex" ||
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
                     n == "List" || n == "Shared" || n == "Weak");
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
        bool mutableBorrowArg = false;
        const bool argIsMove = isMoveExpression(arg);
        const bool argIsBorrow = isBorrowExpression(arg, &mutableBorrowArg);
        auto argType = arg ? arg->getType() : std::make_unique<UnknownType>();
        // An explicit `<-` is only required for a NAMED owned value (an identifier bound to an owned
        // local). A temporary — a literal, `new X()`, a call result, a computed value — is an rvalue
        // with no other owner, so it moves implicitly and needs no marker (as in Rust).
        const bool argIsNamedValue = arg && !identifierName(arg).empty();

        if (paramType && isMoveCheckedType(paramType)) {
            if (!argIsMove && !argIsBorrow && argIsNamedValue) {
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
        node.initializer->accept(*this);
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

    if (node.body) {
        node.body->accept(*this);
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

    exitOwnershipScope();
    cg.symbolTable.exitScope();
    cg.symbolTable.popCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(savedCurrentSymbol);
}

void TypeChecker::visit(ExpressionNode& node) {
    checkTypeResolved(node.getType(), "expression");
}

void TypeChecker::visit(BinaryExpressionNode& node) {
    if (node.left) {
        node.left->accept(*this);
    }
    if (node.right) {
        node.right->accept(*this);
    }
    if (node.left && node.right) {
        auto leftType = node.left->getType();
        auto rightType = node.right->getType();
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
    for (auto& stmt : node.statements) {
        if (!stmt) {
            continue;
        }

        if (auto* functionNode = dynamic_cast<FunctionDeclarationNode*>(stmt.get())) {
            ensureFunctionSymbol(*functionNode);
        }
        else if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(stmt.get())) {
            ensureClassSymbol(*classNode);
        }
        else if (auto* objectNode = dynamic_cast<ObjectDeclarationNode*>(stmt.get())) {
            auto& cg = CodeGenerator::getInstance();
            if (!cg.symbolTable.lookup(objectNode->name)) {
                cg.symbolTable.addSymbol(objectNode->name, std::make_unique<NamespaceSymbol>(objectNode->name));
            }
        }
        else if (auto* templateNode = dynamic_cast<TemplateDeclarationNode*>(stmt.get())) {
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
    const auto afterThen = ownershipScopes;

    // The else arm runs on the false path, from the pre-branch state — not the then arm's state.
    ownershipScopes = before;
    if (node.elseBlock) {
        node.elseBlock->accept(*this);
    }
    const bool elseDiverges = node.elseBlock && statementDiverges(node.elseBlock.get());
    const auto afterElse = ownershipScopes;

    // Join the surviving paths. With no else arm, the false path falls through with `before` (no
    // extra moves), which is exactly what starting from `before` and skipping the else merge gives.
    ownershipScopes = before;
    if (!thenDiverges) {
        mergeMovedFrom(afterThen);
    }
    if (!elseDiverges) {
        mergeMovedFrom(afterElse);
    }

    node.type = node.getType();
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
        bindVariable(node.variable, std::make_unique<BasicType>("Int"), false);
        declareOwnership(node.variable, std::make_unique<BasicType>("Int"));
    }
    visitLoopContracts(node);
    if (node.body) {
        node.body->accept(*this);
    }
    exitOwnershipScope();
    cg.symbolTable.exitScope();
}

void TypeChecker::visit(ReturnStatementNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
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
    for (auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
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

void TypeChecker::visit(AssignmentNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
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

void TypeChecker::visit(MatchExpressionNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }

    std::unique_ptr<Type> matchType = node.expression ? node.expression->getType() : std::make_unique<UnknownType>();
    std::unique_ptr<Type> resultType;

    for (auto& casePair : node.cases) {
        if (casePair.first) {
            casePair.first->accept(*this);
            auto patternType = casePair.first->getType();
            const bool isWildcard = dynamic_cast<UnitNode*>(casePair.first.get()) != nullptr;
            const bool isRegexPattern = [&]() {
                auto* value = dynamic_cast<ValueNode*>(casePair.first.get());
                return value && value->valueType == TokenType::REGEX_LITERAL;
            }();
            if (!isWildcard && matchType && patternType &&
                !(isRegexPattern && matchType->isStringTy()) &&
                !patternType->equals(matchType) &&
                !patternType->isSubtypeOf(matchType) &&
                !matchType->isSubtypeOf(patternType)) {
                std::cerr << "Type error: match case pattern type mismatch" << std::endl;
                errorCount++;
            }
        }
        if (casePair.second) {
            casePair.second->accept(*this);
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
    for (auto& arg : node.arguments) {
        if (arg) {
            arg->accept(*this);
        }
    }
    if (node.object) {
        auto receiverType = node.object->getType();
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

void TypeChecker::visit(IdentifierNode& node) {
    checkIdentifierUse(node.value);
    auto* symbol = CodeGenerator::getInstance().symbolTable.lookup(node.value);
    if (!symbol) {
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
    for (auto& arg : node.arguments) {
        if (arg) {
            arg->accept(*this);
        }
    }
    std::vector<std::unique_ptr<Type>> parameterTypes;
    auto& cg = CodeGenerator::getInstance();
    if (auto* symbol = cg.symbolTable.lookup(node.functionName)) {
        if (auto* funcType = dynamic_cast<FunctionType*>(symbol->type.get())) {
            for (const auto& parameterType : funcType->parameterTypes) {
                parameterTypes.push_back(parameterType ? parameterType->clone() : std::make_unique<UnknownType>());
            }
        }
    }
    if (!parameterTypes.empty()) {
        checkFunctionArguments(node.arguments, parameterTypes, node.functionName);
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
    for (auto& arg : node.arguments) {
        if (arg) {
            arg->accept(*this);
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
        node.right->accept(*this);
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
    else if (rightIsMoveChecked && !rightName.empty()) {
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
        if (!sourceName.empty()) {
            markMoved(sourceName);
        }
    }
    else if (node.op == "&" || node.op == "&mut") {
        if (!sourceName.empty()) {
            markBorrowed(sourceName, node.op == "&mut");
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

void TypeChecker::visit(LambdaExpressionNode& node) {
    auto& cg = CodeGenerator::getInstance();
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
        node.body->accept(*this);
        if (statementDiverges(node.body.get())) {
            ownershipScopes = beforeBody;
        }
    }
}

void TypeChecker::visit(ArrayLiteralNode& node) {
    for (auto& element : node.elements) {
        if (element) {
            element->accept(*this);
        }
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
