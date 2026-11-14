#include "codegen.h"

#include "ClassInstanceCreationNode.h"
#include "ClassDeclarationNode.h"
#include "BinaryExpressionNode.h"
#include "BlockNode.h"
#include "ClassBodyNode.h"
#include "FunctionCallNode.h"
#include "FunctionDeclarationNode.h"
#include "IdentifierNode.h"
#include "IfStatementNode.h"
#include "ParameterNode.h"
#include "ReturnStatementNode.h"
#include "ASTVisitor.h"
#include "ValueNode.h"
#include "VariableDeclarationNode.h"
#include "TensorRuntime.h"
#include "type_utils.h"
#include "builtin_structs.h"
#include "all_ast.h"
#include "type_checker.h"

#include <iostream>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>

#include "utils.h"

namespace {
std::string mangleTemplateClassName(const std::string& className,
                                    const std::vector<std::unique_ptr<Type>>& templateArgs) {
    std::string mangledName = className;
    for (const auto& arg : templateArgs) {
        mangledName += "$" + (arg ? arg->getName() : "Unknown");
    }
    return mangledName;
}

llvm::Value* coerceForStore(llvm::Value* value, llvm::Type* targetType) {
    return coerceValueToLLVMType(value, targetType);
}

void substituteTypeVariables(std::unique_ptr<Type>& type,
                             const std::vector<std::string>& typeParams,
                             const std::vector<std::unique_ptr<Type>>& concreteArgs) {
    if (!type) {
        return;
    }

    if (type->getKind() == Type::Kind::VARIABLE) {
        for (size_t i = 0; i < typeParams.size(); ++i) {
            if (type->getName() == typeParams[i] && i < concreteArgs.size() && concreteArgs[i]) {
                type = concreteArgs[i]->clone();
                return;
            }
        }
    }

    if (type->getKind() == Type::Kind::GENERIC) {
        auto* generic = dynamic_cast<GenericType*>(type.get());
        if (generic) {
            substituteTypeVariables(generic->baseType, typeParams, concreteArgs);
            for (auto& arg : generic->typeArguments) {
                substituteTypeVariables(arg, typeParams, concreteArgs);
            }
        }
    }
}

ClassSymbol* ensureTemplateClassInstantiation(const std::string& className,
                                              const std::vector<std::unique_ptr<Type>>& templateArgs);

std::unordered_set<std::string>& templateInstantiationsInProgress();

// Instances registered while type checking, generated once code generation starts (by mangled name).
std::unordered_map<std::string, std::unique_ptr<ASTNode>>& pendingTemplateClasses() {
    static std::unordered_map<std::string, std::unique_ptr<ASTNode>> pending;
    return pending;
}

void generateTemplateClass(CodeGenerator& cg, ASTNode* declaration, const std::string& mangledName) {
    templateInstantiationsInProgress().insert(mangledName);
    auto* savedInsertBlock = cg.builder.GetInsertBlock();
    auto savedInsertPoint = cg.builder.GetInsertPoint();
    auto* savedCurrentSymbol = cg.symbolTable.getCurrentSymbol();
    cg.symbolTable.saveCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(nullptr);
    declaration->codegen();
    cg.symbolTable.popCurrentSymbol();
    cg.symbolTable.setCurrentSymbol(savedCurrentSymbol);
    if (savedInsertBlock) {
        cg.builder.SetInsertPoint(savedInsertBlock, savedInsertPoint);
    }
    templateInstantiationsInProgress().erase(mangledName);
}

// Template instantiations being generated right now (by mangled name). A class that mentions its
// own instance type (`def operator +(o: GF<P>): GF<P>`) refers to the instance by name instead of
// instantiating it again.
std::unordered_set<std::string>& templateInstantiationsInProgress() {
    static std::unordered_set<std::string> names;
    return names;
}

struct TemplateBinding {
    std::vector<std::string> names;              // all template parameters, type and non-type
    std::vector<std::unique_ptr<Type>> arguments;
    std::unordered_map<std::string, std::string> constants;  // non-type parameter -> its value text
};

// Substitutes the template parameters in `type`, and turns an applied template type such as
// GF<7> (also GF<P> after substitution) into the instance class type GF$7.
void resolveTemplateType(std::unique_ptr<Type>& type, const TemplateBinding& binding) {
    if (!type) return;
    switch (type->getKind()) {
        case Type::Kind::VARIABLE:
        case Type::Kind::CLASS:
            // A non-type parameter can also be parsed as a class name in some positions.
            for (size_t i = 0; i < binding.names.size(); ++i) {
                if (type->getName() == binding.names[i] && i < binding.arguments.size() && binding.arguments[i]) {
                    type = binding.arguments[i]->clone();
                    return;
                }
            }
            return;
        case Type::Kind::GENERIC: {
            auto* generic = dynamic_cast<GenericType*>(type.get());
            if (!generic) return;
            resolveTemplateType(generic->baseType, binding);
            for (auto& argument : generic->typeArguments) resolveTemplateType(argument, binding);
            const std::string baseName = generic->baseType ? generic->baseType->getName() : "";
            auto* symbol = CodeGenerator::getInstance().symbolTable.lookup(baseName);
            if (!symbol || symbol->symbolType != SymbolType::TEMPLATE) return;
            const std::string mangled = mangleTemplateClassName(baseName, generic->typeArguments);
            if (!templateInstantiationsInProgress().count(mangled)) {
                ensureTemplateClassInstantiation(baseName, generic->typeArguments);
            }
            type = std::make_unique<ClassType>(mangled);
            return;
        }
        case Type::Kind::ARRAY: {
            auto* array = dynamic_cast<ArrayType*>(type.get());
            if (array) resolveTemplateType(array->elementType, binding);
            return;
        }
        case Type::Kind::BORROW:
        case Type::Kind::MUTABLE_BORROW: {
            auto* borrow = dynamic_cast<BorrowType*>(type.get());
            if (borrow) resolveTemplateType(borrow->baseType, binding);
            return;
        }
        case Type::Kind::BOX: {
            auto* box = dynamic_cast<BoxType*>(type.get());
            if (box) resolveTemplateType(box->baseType, binding);
            return;
        }
        case Type::Kind::FUNCTION: {
            auto* function = dynamic_cast<FunctionType*>(type.get());
            if (!function) return;
            for (auto& parameter : function->parameterTypes) resolveTemplateType(parameter, binding);
            resolveTemplateType(function->returnType, binding);
            return;
        }
        case Type::Kind::TUPLE: {
            auto* tuple = dynamic_cast<TupleType*>(type.get());
            if (tuple) for (auto& element : tuple->elementTypes) resolveTemplateType(element, binding);
            return;
        }
        default:
            return;
    }
}

// Substitutes the template parameters throughout a cloned method body or initializer: non-type
// parameters used as values, and every type written in it (declarations, `new C<...>`, explicit
// call type arguments, lambda parameters).
void substituteTemplateBinding(ASTNode* node, const TemplateBinding& binding) {
    if (!node) return;
    auto visit = [&](const std::unique_ptr<ASTNode>& child) { substituteTemplateBinding(child.get(), binding); };
    if (node->type) resolveTemplateType(node->type, binding);

    if (auto* identifier = dynamic_cast<IdentifierNode*>(node)) {
        auto it = binding.constants.find(identifier->value);
        if (it != binding.constants.end()) identifier->value = it->second;
        return;
    }
    if (auto* block = dynamic_cast<BlockNode*>(node)) { for (auto& st : block->statements) visit(st); return; }
    if (auto* ret = dynamic_cast<ReturnStatementNode*>(node)) { visit(ret->expression); return; }
    if (auto* binary = dynamic_cast<BinaryExpressionNode*>(node)) { visit(binary->left); visit(binary->right); return; }
    if (auto* unary = dynamic_cast<UnaryExpressionNode*>(node)) { visit(unary->expression); return; }
    if (auto* prefix = dynamic_cast<PrefixExpressionNode*>(node)) { visit(prefix->expression); return; }
    if (auto* postfix = dynamic_cast<PostfixExpressionNode*>(node)) { visit(postfix->expression); return; }
    if (auto* ifNode = dynamic_cast<IfStatementNode*>(node)) {
        visit(ifNode->condition);
        substituteTemplateBinding(ifNode->thenBlock.get(), binding);
        substituteTemplateBinding(ifNode->elseBlock.get(), binding);
        return;
    }
    if (auto* loop = dynamic_cast<WhileStatementNode*>(node)) {
        visit(loop->condition);
        substituteTemplateBinding(loop->body.get(), binding);
        return;
    }
    if (auto* loop = dynamic_cast<ForStatementNode*>(node)) {
        visit(loop->iterableExpr);
        substituteTemplateBinding(loop->body.get(), binding);
        return;
    }
    if (auto* range = dynamic_cast<RangeExpressionNode*>(node)) { visit(range->startExpr); visit(range->endExpr); return; }
    if (auto* decl = dynamic_cast<VariableDeclarationNode*>(node)) {
        resolveTemplateType(decl->type, binding);  // VariableDeclarationNode has its own `type`
        visit(decl->initializer);
        return;
    }
    if (auto* assignment = dynamic_cast<AssignmentExpressionNode*>(node)) { visit(assignment->left); visit(assignment->right); return; }
    if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
        for (auto& t : call->explicitTypeArgs) resolveTemplateType(t, binding);
        for (auto& argument : call->arguments) visit(argument);
        return;
    }
    if (auto* method = dynamic_cast<MethodCallNode*>(node)) {
        visit(method->object);
        for (auto& argument : method->arguments) visit(argument);
        return;
    }
    if (auto* access = dynamic_cast<AccessFieldNode*>(node)) { visit(access->base); return; }
    if (auto* access = dynamic_cast<ArrayAccessNode*>(node)) {
        visit(access->array);
        visit(access->index);
        for (auto& spec : access->indices) {
            if (!spec) continue;
            visit(spec->index); visit(spec->start); visit(spec->end); visit(spec->step);
        }
        return;
    }
    if (auto* creation = dynamic_cast<ClassInstanceCreationNode*>(node)) {
        for (auto& t : creation->templateArgs) resolveTemplateType(t, binding);
        for (auto& argument : creation->arguments) visit(argument);
        return;
    }
    if (auto* literal = dynamic_cast<ArrayLiteralNode*>(node)) { for (auto& e : literal->elements) visit(e); return; }
    if (auto* tuple = dynamic_cast<TupleExpressionNode*>(node)) { for (auto& e : tuple->elements) visit(e); return; }
    if (auto* dict = dynamic_cast<DictLiteralNode*>(node)) {
        for (auto& k : dict->keys) visit(k);
        for (auto& v : dict->values) visit(v);
        return;
    }
    if (auto* match = dynamic_cast<MatchExpressionNode*>(node)) {
        visit(match->expression);
        for (auto& c : match->cases) { visit(c.first); visit(c.second); }
        return;
    }
    if (auto* cast = dynamic_cast<CastingExpressionNode*>(node)) { visit(cast->expression); return; }
    if (auto* lambda = dynamic_cast<LambdaExpressionNode*>(node)) {
        for (auto& argument : lambda->arguments) {
            if (auto* parameter = dynamic_cast<ParameterNode*>(argument.get())) resolveTemplateType(parameter->type, binding);
        }
        visit(lambda->body);
        return;
    }
    if (auto* destructuring = dynamic_cast<DestructuringAssignmentNode*>(node)) { visit(destructuring->value); return; }
}

TemplateBinding makeTemplateBinding(const TemplateSymbol& tmplSymbol, const std::vector<std::unique_ptr<Type>>& templateArgs) {
    TemplateBinding binding;
    if (!tmplSymbol.templateParams.empty()) {
        for (const auto& param : tmplSymbol.templateParams) binding.names.push_back(param.name);
    }
    else {
        binding.names = tmplSymbol.typeParameters;
    }
    for (const auto& argument : templateArgs) binding.arguments.push_back(argument ? argument->clone() : std::make_unique<UnknownType>());
    for (size_t i = 0; i < tmplSymbol.templateParams.size() && i < templateArgs.size(); ++i) {
        if (!tmplSymbol.templateParams[i].isType && templateArgs[i]) {
            binding.constants[tmplSymbol.templateParams[i].name] = templateArgs[i]->getName();
        }
    }
    return binding;
}

void substituteTemplateConstants(ASTNode* node,
                                 const std::unordered_map<std::string, std::string>& constantValues) {
    if (!node) {
        return;
    }

    if (auto* identifier = dynamic_cast<IdentifierNode*>(node)) {
        auto it = constantValues.find(identifier->value);
        if (it != constantValues.end()) {
            identifier->value = it->second;
        }
        return;
    }

    if (auto* block = dynamic_cast<BlockNode*>(node)) {
        for (auto& stmt : block->statements) {
            substituteTemplateConstants(stmt.get(), constantValues);
        }
        return;
    }

    if (auto* returnStmt = dynamic_cast<ReturnStatementNode*>(node)) {
        substituteTemplateConstants(returnStmt->expression.get(), constantValues);
        return;
    }

    if (auto* binary = dynamic_cast<BinaryExpressionNode*>(node)) {
        substituteTemplateConstants(binary->left.get(), constantValues);
        substituteTemplateConstants(binary->right.get(), constantValues);
        return;
    }

    if (auto* ifStmt = dynamic_cast<IfStatementNode*>(node)) {
        substituteTemplateConstants(ifStmt->condition.get(), constantValues);
        substituteTemplateConstants(ifStmt->thenBlock.get(), constantValues);
        substituteTemplateConstants(ifStmt->elseBlock.get(), constantValues);
        return;
    }

    if (auto* varDecl = dynamic_cast<VariableDeclarationNode*>(node)) {
        substituteTemplateConstants(varDecl->initializer.get(), constantValues);
        return;
    }

    if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
        for (auto& arg : call->arguments) {
            substituteTemplateConstants(arg.get(), constantValues);
        }
        return;
    }
}

void substituteClassTemplateArguments(ClassDeclarationNode* classDecl, const TemplateBinding& binding) {
    if (!classDecl) {
        return;
    }

    for (auto& param : classDecl->constructorParams) {
        auto* paramNode = dynamic_cast<ParameterNode*>(param.get());
        if (paramNode) {
            resolveTemplateType(paramNode->type, binding);
        }
    }

    auto* classBody = dynamic_cast<ClassBodyNode*>(classDecl->body.get());
    if (!classBody) {
        return;
    }

    for (auto& field : classBody->fields) {
        auto* fieldNode = dynamic_cast<VariableDeclarationNode*>(field.get());
        if (!fieldNode) {
            continue;
        }
        resolveTemplateType(fieldNode->type, binding);
        substituteTemplateBinding(fieldNode->initializer.get(), binding);
    }

    for (auto& method : classBody->methods) {
        auto* methodNode = dynamic_cast<FunctionDeclarationNode*>(method.get());
        if (!methodNode) {
            continue;
        }
        for (auto& param : methodNode->parameters) {
            auto* paramNode = dynamic_cast<ParameterNode*>(param.get());
            if (paramNode) {
                resolveTemplateType(paramNode->type, binding);
            }
        }
        resolveTemplateType(methodNode->returnType, binding);
        substituteTemplateBinding(methodNode->body.get(), binding);
    }
}

ClassSymbol* ensureTemplateClassInstantiation(const std::string& className,
                                              const std::vector<std::unique_ptr<Type>>& templateArgs) {
    auto& cg = CodeGenerator::getInstance();
    auto* symbol = cg.symbolTable.lookup(className);
    if (!symbol || symbol->symbolType != SymbolType::TEMPLATE) {
        return nullptr;
    }

    auto* tmplSymbol = dynamic_cast<TemplateSymbol*>(symbol);
    auto* classDecl = tmplSymbol ? dynamic_cast<ClassDeclarationNode*>(tmplSymbol->declaration.get()) : nullptr;
    if (!tmplSymbol || !classDecl) {
        return nullptr;
    }

    std::string mangledName = mangleTemplateClassName(className, templateArgs);
    auto& pending = pendingTemplateClasses();
    if (auto* existingClass = cg.symbolTable.lookupClass(mangledName)) {
        // Registered while type checking: generate it now that code generation has started.
        auto it = pending.find(mangledName);
        if (it != pending.end() && cg.generatingCode && !templateInstantiationsInProgress().count(mangledName)) {
            std::unique_ptr<ASTNode> declaration = std::move(it->second);
            pending.erase(it);
            generateTemplateClass(cg, declaration.get(), mangledName);
            existingClass = cg.symbolTable.lookupClass(mangledName);
        }
        tmplSymbol->classInstantiations[mangledName] = existingClass;
        return existingClass;
    }

    auto clonedDecl = classDecl->clone();
    auto* clonedClass = dynamic_cast<ClassDeclarationNode*>(clonedDecl.get());
    if (!clonedClass) {
        return nullptr;
    }
    clonedClass->name = mangledName;

    // Mark the instance as in progress before substituting: its own methods may name GF<P>.
    templateInstantiationsInProgress().insert(mangledName);
    struct InProgress {
        std::string name;
        ~InProgress() { templateInstantiationsInProgress().erase(name); }
    } inProgress{mangledName};
    const TemplateBinding binding = makeTemplateBinding(*tmplSymbol, templateArgs);
    substituteClassTemplateArguments(clonedClass, binding);

    if (!cg.generatingCode) {
        // Type checking: register the instance's class symbol and methods (so operators and method
        // calls on it type-check) and generate its code later -- the program's external functions and
        // other declarations its methods call are not generated yet.
        auto* savedCurrentSymbol = cg.symbolTable.getCurrentSymbol();
        cg.symbolTable.saveCurrentSymbol();
        cg.symbolTable.setCurrentSymbol(nullptr);
        TypeChecker checker;
        clonedClass->accept(checker);
        cg.symbolTable.popCurrentSymbol();
        cg.symbolTable.setCurrentSymbol(savedCurrentSymbol);
        pendingTemplateClasses()[mangledName] = std::move(clonedDecl);
    }
    else {
        generateTemplateClass(cg, clonedClass, mangledName);
    }

    auto* instantiatedSymbol = cg.symbolTable.lookupClass(mangledName);
    if (instantiatedSymbol) {
        tmplSymbol->classInstantiations[mangledName] = instantiatedSymbol;
    }
    return instantiatedSymbol;
}
}

void ClassInstanceCreationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* ClassInstanceCreationNode::codegen() {
    auto& cg = CodeGenerator::getInstance();

    // Primitive allocation: return typed pointer from malloc.
    if (isPrimitiveType(className)) {
        BasicType basicType(className);
        llvm::Type* type = cg.getLLVMType(&basicType);
        if (!type) {
            std::cerr << "Error: Unsupported primitive type '" << className << "'" << std::endl;
            return nullptr;
        }

        const llvm::DataLayout& dl = cg.module->getDataLayout();
        uint64_t typeSize = dl.getTypeAllocSize(type);
        llvm::Value* allocSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(cg.context), typeSize);
        llvm::Value* rawPtr = cg.builder.CreateCall(cg.mallocFunction, allocSize, "malloc");
        return cg.builder.CreateBitCast(rawPtr, llvm::PointerType::getUnqual(type->getContext()), "newptr");
    }

    if (className == "Tensor") {
        std::vector<int64_t> dims = TensorRuntime::dimensionsFromTemplateArgs(templateArgs);
        if (dims.empty()) {
            std::cerr << "Error: Tensor requires at least one positive dimension" << std::endl;
            return nullptr;
        }

        llvm::Value* fillValue = nullptr;
        if (!arguments.empty()) {
            fillValue = arguments.front()->codegen();
        }
        return TensorRuntime::createTensor(cg, dims, fillValue);
    }

    // Built-in value aggregates: `new Complex(re, im)` etc. produce a struct value.
    if (const auto* builtinFields = builtinValueStructFields(className)) {
        auto* structSymbol = dynamic_cast<StructSymbol*>(cg.symbolTable.lookup(className));
        auto* structType = structSymbol ? llvm::dyn_cast_or_null<llvm::StructType>(structSymbol->structType) : nullptr;
        if (!structType) {
            std::cerr << "Error: built-in type '" << className << "' is unavailable" << std::endl;
            return nullptr;
        }
        if (arguments.size() != builtinFields->size()) {
            std::cerr << "Error: new " << className << "(...) takes " << builtinFields->size() << " arguments" << std::endl;
            return nullptr;
        }
        llvm::Value* aggregate = llvm::UndefValue::get(structType);
        for (size_t i = 0; i < arguments.size(); ++i) {
            llvm::Value* value = arguments[i] ? arguments[i]->codegen() : nullptr;
            if (!value) return nullptr;
            llvm::Type* fieldType = structType->getElementType(static_cast<unsigned>(i));
            if (fieldType->isDoubleTy()) {
                if (value->getType()->isIntegerTy()) value = cg.builder.CreateSIToFP(value, fieldType);
                else if (value->getType()->isFloatTy()) value = cg.builder.CreateFPExt(value, fieldType);
            } else if (fieldType->isIntegerTy() && value->getType()->isIntegerTy() && value->getType() != fieldType) {
                value = cg.builder.CreateSExtOrTrunc(value, fieldType);
            } else if (fieldType->isIntegerTy() && value->getType()->isFloatingPointTy()) {
                value = cg.builder.CreateFPToSI(value, fieldType);
            }
            aggregate = cg.builder.CreateInsertValue(aggregate, value, { static_cast<unsigned>(i) });
        }
        return aggregate;
    }

    std::string resolvedClassName = className;

    // Template Class instantiation: new ClassName<Type>(args)
    if (!templateArgs.empty()) {
        if (auto* instantiatedClass = ensureTemplateClassInstantiation(className, templateArgs)) {
            resolvedClassName = instantiatedClass->name;
        }
    }

    auto classSymbolOpt = cg.symbolTable.lookupClass(resolvedClassName);
    if (!classSymbolOpt) {
        std::cerr << "Error: Class '" << resolvedClassName << "' not found" << std::endl;
        return nullptr;
    }

    auto* classSymbol = classSymbolOpt;
    llvm::StructType* classType = llvm::dyn_cast<llvm::StructType>(classSymbol->classType);
    if (!classType) {
        std::cerr << "Error: Unknown type about class '" << resolvedClassName << "'" << std::endl;
        return nullptr;
    }

    llvm::Value* instance = nullptr;
    if (classSymbol->isStruct) {
        instance = cg.builder.CreateAlloca(classType, nullptr, "newstruct");
    }
    else {
        const llvm::DataLayout& dl = cg.module->getDataLayout();
        uint64_t typeSize = dl.getTypeAllocSize(classType);
        llvm::Value* allocSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(cg.context), typeSize);
        llvm::Value* rawPtr = cg.builder.CreateCall(cg.mallocFunction, allocSize, "newobj.malloc");
        instance = cg.builder.CreateBitCast(rawPtr, llvm::PointerType::getUnqual(classType->getContext()), "newobj");
    }

    unsigned fieldIndex = 0;
    auto initializeField = [&](const std::string& fieldName, const std::unique_ptr<Type>& fieldType, llvm::Value* value) -> bool {
        llvm::Type* targetType = getABIStorageType(fieldType.get());
        if (!targetType) {
            std::cerr << "Error: Unsupported field type in class '" << resolvedClassName << "'" << std::endl;
            return false;
        }
        if (!value) {
            value = llvm::Constant::getNullValue(targetType);
        }
        value = coerceForStore(value, targetType);
        if (!value || value->getType() != targetType) {
            std::cerr << "Error: Constructor value for field '" << fieldName << "' has incompatible type" << std::endl;
            return false;
        }
        if (fieldIndex >= classType->getNumElements()) {
            std::cerr << "Error: Field layout for '" << resolvedClassName << "' does not match initializer '" << fieldName << "'" << std::endl;
            return false;
        }
        llvm::Value* fieldPtr = cg.builder.CreateStructGEP(classType, instance, fieldIndex, fieldName + ".init");
        cg.builder.CreateStore(value, fieldPtr);
        fieldIndex += 1;
        return true;
    };

    // Prefer the declaration order preserved in the instantiated ClassSymbol's
    // struct body by re-reading the original class declaration when possible.
    if (!templateArgs.empty()) {
        auto* symbol = cg.symbolTable.lookup(className);
        auto* tmplSymbol = symbol && symbol->symbolType == SymbolType::TEMPLATE ? dynamic_cast<TemplateSymbol*>(symbol) : nullptr;
        auto* originalClass = tmplSymbol ? dynamic_cast<ClassDeclarationNode*>(tmplSymbol->declaration.get()) : nullptr;
        if (originalClass) {
            if (arguments.size() > originalClass->constructorParams.size()) {
                std::cerr << "Error: Too many constructor arguments for class '" << resolvedClassName << "'" << std::endl;
                return nullptr;
            }

            std::unordered_map<std::string, std::string> constantValues;
            for (size_t i = 0; i < tmplSymbol->templateParams.size() && i < templateArgs.size(); ++i) {
                if (!tmplSymbol->templateParams[i].isType && templateArgs[i]) {
                    constantValues[tmplSymbol->templateParams[i].name] = templateArgs[i]->getName();
                }
            }

            std::vector<std::unique_ptr<Type>> concreteTypes;
            concreteTypes.reserve(templateArgs.size());
            for (const auto& arg : templateArgs) {
                concreteTypes.push_back(arg ? arg->clone() : std::make_unique<UnknownType>());
            }
            const TemplateBinding constructionBinding = makeTemplateBinding(*tmplSymbol, templateArgs);

            cg.symbolTable.enterScope();
            auto failTemplateInitialization = [&]() -> llvm::Value* {
                cg.symbolTable.exitScope();
                return nullptr;
            };

            std::vector<std::unique_ptr<ASTNode>> clonedCtorParams;
            clonedCtorParams.reserve(originalClass->constructorParams.size());
            for (const auto& param : originalClass->constructorParams) {
                clonedCtorParams.push_back(param ? param->clone() : nullptr);
            }

            for (size_t i = 0; i < clonedCtorParams.size(); ++i) {
                auto* param = dynamic_cast<ParameterNode*>(clonedCtorParams[i].get());
                if (!param) {
                    continue;
                }
                resolveTemplateType(param->type, constructionBinding);
                llvm::Value* argValue = i < arguments.size() ? arguments[i]->codegen() : nullptr;
                if (!initializeField(param->name, param->type, argValue)) {
                    return failTemplateInitialization();
                }
                llvm::Type* targetType = getABIStorageType(param->type.get());
                if (!argValue && targetType) {
                    argValue = llvm::Constant::getNullValue(targetType);
                }
                argValue = coerceForStore(argValue, targetType);
                cg.symbolTable.addSymbol(
                    param->name,
                    std::make_unique<Symbol>(param->name, param->type ? param->type->clone() : std::make_unique<UnknownType>(), argValue, false, SymbolType::VARIABLE));
            }

            if (auto* originalBody = dynamic_cast<ClassBodyNode*>(originalClass->body.get())) {
                for (const auto& fieldAst : originalBody->fields) {
                    auto clonedField = fieldAst ? fieldAst->clone() : nullptr;
                    auto* fieldNode = dynamic_cast<VariableDeclarationNode*>(clonedField.get());
                    if (!fieldNode) {
                        continue;
                    }
                    resolveTemplateType(fieldNode->type, constructionBinding);
                    substituteTemplateBinding(fieldNode->initializer.get(), constructionBinding);
                    llvm::Value* initValue = fieldNode->initializer ? fieldNode->initializer->codegen() : nullptr;
                    if (!initializeField(fieldNode->name, fieldNode->type, initValue)) {
                        return failTemplateInitialization();
                    }
                }
            }

            cg.symbolTable.exitScope();
            return instance;
        }
    }

    if (arguments.size() != classSymbol->constructorParamOrder.size()) {
        std::cerr << "Error: Constructor for class '" << resolvedClassName << "' expects "
                  << classSymbol->constructorParamOrder.size() << " argument(s), got "
                  << arguments.size() << std::endl;
        return nullptr;
    }

    cg.symbolTable.enterScope();
    auto failInitialization = [&]() -> llvm::Value* {
        cg.symbolTable.exitScope();
        return nullptr;
    };

    for (size_t i = 0; i < classSymbol->constructorParamOrder.size(); ++i) {
        const std::string& paramName = classSymbol->constructorParamOrder[i];
        auto paramIt = classSymbol->constructorParams.find(paramName);
        if (paramIt == classSymbol->constructorParams.end() || !paramIt->second || !paramIt->second->type) {
            std::cerr << "Error: Missing constructor parameter metadata for '" << paramName << "'" << std::endl;
            return failInitialization();
        }

        llvm::Value* argValue = arguments[i]->codegen();
        if (!initializeField(paramName, paramIt->second->type, argValue)) {
            return failInitialization();
        }

        llvm::Type* targetType = getABIStorageType(paramIt->second->type.get());
        argValue = coerceForStore(argValue, targetType);
        cg.symbolTable.addSymbol(
            paramName,
            std::make_unique<Symbol>(paramName, paramIt->second->type->clone(), argValue, false, SymbolType::VARIABLE));
    }

    // Constructor field initializers are emitted inline, so a class that eagerly constructs its own
    // type in a field default (`var next: Node = new Node(0)`) recurses forever at codegen. Detect
    // re-entry into the same class's field initialization and fail with a diagnostic rather than
    // overflowing the stack. The guard sits after the constructor-argument loop above so a
    // legitimate same-class argument (`new A(new A(5))`) is not caught.
    if (cg.classesUnderConstruction.count(resolvedClassName)) {
        std::cerr << "Error: class '" << resolvedClassName
                  << "' eagerly constructs its own type in a field initializer (infinite "
                     "construction); reference or nullable fields are not yet supported" << std::endl;
        return failInitialization();
    }
    cg.classesUnderConstruction.insert(resolvedClassName);
    struct ConstructionGuard {
        std::unordered_set<std::string>& set;
        std::string name;
        ~ConstructionGuard() { set.erase(name); }
    } constructionGuard{cg.classesUnderConstruction, resolvedClassName};

    for (const auto& fieldName : classSymbol->fieldOrder) {
        auto fieldIt = classSymbol->fields.find(fieldName);
        if (fieldIt == classSymbol->fields.end() || !fieldIt->second || !fieldIt->second->type) {
            std::cerr << "Error: Missing field metadata for '" << fieldName << "'" << std::endl;
            return failInitialization();
        }

        auto initIt = classSymbol->fieldInitializers.find(fieldName);
        ASTNode* initializer = initIt != classSymbol->fieldInitializers.end() ? initIt->second.get() : nullptr;
        if (!initializer) {
            std::cerr << "Error: Field '" << fieldName << "' in class '" << resolvedClassName
                      << "' is not initialized" << std::endl;
            return failInitialization();
        }

        llvm::Value* initValue = initializer->codegen();
        if (!initializeField(fieldName, fieldIt->second->type, initValue)) {
            return failInitialization();
        }
        llvm::Type* targetType = getABIStorageType(fieldIt->second->type.get());
        initValue = coerceForStore(initValue, targetType);
        cg.symbolTable.addSymbol(
            fieldName,
            std::make_unique<Symbol>(fieldName, fieldIt->second->type->clone(), initValue, fieldIt->second->isMutable, SymbolType::VARIABLE));
    }

    cg.symbolTable.exitScope();

    return instance;
}

// The LLVM type of a template instance first needed by a signature: generate a pending instance.
bool materializeTemplateClass(const std::string& mangledName) {
    auto& cg = CodeGenerator::getInstance();
    auto& pending = pendingTemplateClasses();
    auto it = pending.find(mangledName);
    if (it == pending.end() || !cg.generatingCode || templateInstantiationsInProgress().count(mangledName)) return false;
    std::unique_ptr<ASTNode> declaration = std::move(it->second);
    pending.erase(it);
    generateTemplateClass(cg, declaration.get(), mangledName);
    return true;
}

std::unique_ptr<Type> ClassInstanceCreationNode::getType() {
    if (!templateArgs.empty()) {
        if (className == "Tensor") {
            std::vector<std::unique_ptr<Type>> clonedArgs;
            clonedArgs.reserve(templateArgs.size());
            for (const auto& arg : templateArgs) {
                clonedArgs.push_back(arg ? arg->clone() : std::make_unique<UnknownType>());
            }
            return std::make_unique<GenericType>(std::make_unique<BasicType>("Tensor"), clonedArgs);
        }
        if (className != "Tensor") {
            ensureTemplateClassInstantiation(className, templateArgs);
        }
        std::string mangledName = mangleTemplateClassName(className, templateArgs);
        return std::make_unique<ClassType>(mangledName);
    }

    return std::make_unique<ClassType>(className);
}
