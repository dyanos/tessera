// enum_nodes.cpp -- code generation for enum values and the `?` operator (see enum_nodes.h).
#include "codegen.h"
#include "type_checker.h"

#include "enum_nodes.h"
#include "enums.h"
#include "ASTVisitor.h"
#include "AccessFieldNode.h"
#include "IdentifierNode.h"
#include "ReturnStatementNode.h"
#include "VariableDeclarationNode.h"
#include "type_utils.h"

#include <iostream>
#include <sstream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>

namespace {
std::vector<std::unique_ptr<Type>> cloneTypes(const std::vector<std::unique_ptr<Type>>& types) {
    std::vector<std::unique_ptr<Type>> result;
    for (const auto& t : types) result.push_back(t ? t->clone() : std::make_unique<UnknownType>());
    return result;
}
}  // namespace

// ---- EnumDeclarationNode --------------------------------------------------------------------

EnumDeclarationNode::EnumDeclarationNode(const EnumDeclarationNode& other)
    : name(other.name), typeParameters(other.typeParameters), interfaces(other.interfaces) {
    nodeType = ASTNodeType::ENUM_DECLARATION;
    line = other.line;
    for (const auto& v : other.variants) {
        Variant copy;
        copy.name = v.name;
        copy.line = v.line;
        for (const auto& f : v.fields) copy.fields.push_back(std::make_unique<ParameterNode>(*f));
        variants.push_back(std::move(copy));
    }
    for (const auto& m : other.methods) methods.push_back(m ? m->clone() : nullptr);
}

void EnumDeclarationNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }

llvm::Value* EnumDeclarationNode::codegen() {
    CodeGenerator::getInstance().error("enum '", name, "' was not lowered");
    return nullptr;
}

// ---- EnumConstructNode ----------------------------------------------------------------------

EnumConstructNode::EnumConstructNode(const EnumConstructNode& other)
    : enumName(other.enumName), variantName(other.variantName), typeArguments(cloneTypes(other.typeArguments)),
      qualified(other.qualified) {
    nodeType = ASTNodeType::ENUM_CONSTRUCT;
    line = other.line;
    type = other.type ? other.type->clone() : nullptr;
    for (const auto& a : other.arguments) arguments.push_back(a ? a->clone() : nullptr);
}

void EnumConstructNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }

std::string EnumConstructNode::instanceName() const {
    const EnumInfo* info = findEnum(enumName);
    if (!info) return enumName;
    return enumInstanceClass(*info, typeArguments);
}

std::unique_ptr<Type> EnumConstructNode::getType() {
    if (type) return type->clone();
    const EnumInfo* info = findEnum(enumName);
    if (!info || (!info->typeParameters.empty() && typeArguments.size() != info->typeParameters.size())) {
        return std::make_unique<UnknownType>();
    }
    return std::make_unique<ClassType>(instanceName());
}

llvm::Value* EnumConstructNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    const EnumInfo* info = findEnum(enumName);
    const EnumVariantInfo* variant = info ? info->variant(variantName) : nullptr;
    if (!info || !variant) {
        CodeGenerator::getInstance().error("unknown enum variant ", enumName, ".", variantName);
        return nullptr;
    }
    const std::string className = instanceName();
    ClassSymbol* symbol = cg.symbolTable.lookupClass(className);
    if (symbol && !symbol->classType) {
        materializeTemplateClass(className);
        symbol = cg.symbolTable.lookupClass(className);
    }
    auto* structType = symbol ? llvm::dyn_cast_or_null<llvm::StructType>(symbol->classType) : nullptr;
    if (structType && structType->isOpaque()) {
        materializeTemplateClass(className);
        symbol = cg.symbolTable.lookupClass(className);
        structType = symbol ? llvm::dyn_cast_or_null<llvm::StructType>(symbol->classType) : nullptr;
    }
    if (!symbol || !structType || structType->isOpaque()) {
        CodeGenerator::getInstance().error("enum class '", className, "' is not generated");
        return nullptr;
    }

    // Evaluate the payload first: an argument may itself build a value of this enum.
    std::vector<llvm::Value*> values;
    for (auto& argument : arguments) {
        llvm::Value* v = argument ? argument->codegen() : nullptr;
        if (!v) return nullptr;
        values.push_back(v);
    }

    llvm::Value* instance = nullptr;
    if (symbol->isStruct) {
        instance = cg.builder.CreateAlloca(structType, nullptr, variantName + ".value");
    }
    else {
        const llvm::DataLayout& dl = cg.module->getDataLayout();
        llvm::Value* size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(cg.context), dl.getTypeAllocSize(structType));
        instance = cg.builder.CreateCall(cg.mallocFunction, size, variantName + ".object");
    }
    // Every other variant's fields are zero (null for references).
    cg.builder.CreateStore(llvm::Constant::getNullValue(structType), instance);

    auto fieldIndex = [&](const std::string& field) -> int {
        const auto& order = symbol->constructorParamOrder;
        for (size_t i = 0; i < order.size(); ++i) if (order[i] == field) return static_cast<int>(i);
        return -1;
    };
    const int tagIndex = fieldIndex(enumTagField());
    if (tagIndex < 0) {
        CodeGenerator::getInstance().error("enum class '", className, "' has no tag field");
        return nullptr;
    }
    const bool countedInstance = !symbol->isStruct && isStrictOwnership();
    cg.builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), variant->tag),
                           cg.builder.CreateStructGEP(structType, instance, static_cast<unsigned>(tagIndex), "tag.ptr"));
    for (size_t k = 0; k < values.size() && k < variant->storageNames.size(); ++k) {
        const std::string& field = variant->storageNames[k];
        const int index = fieldIndex(field);
        auto param = symbol->constructorParams.find(field);
        if (index < 0 || param == symbol->constructorParams.end() || !param->second) {
            CodeGenerator::getInstance().error("enum class '", className, "' has no field '", field, "'");
            return nullptr;
        }
        llvm::Type* storage = getABIStorageType(param->second->type.get());
        llvm::Value* v = coerceValueToLLVMType(values[k], storage);
        if (!v || v->getType() != storage) {
            CodeGenerator::getInstance().error("value for ", enumName, ".", variantName, " field '", variant->fieldNames[k], "' has an incompatible type");
            return nullptr;
        }
        v = cg.takeOwned(v, param->second->type.get());
        cg.builder.CreateStore(v, cg.builder.CreateStructGEP(structType, instance, static_cast<unsigned>(index), field + ".ptr"));
    }
    if (countedInstance) {
        ClassType instanceType(className);
        if (cg.isRefcountedType(&instanceType)) {
            llvm::Function* release = cg.enumFieldReleaseFunction(&instanceType);
            auto* ptrTy = llvm::PointerType::getUnqual(cg.context);
            llvm::FunctionCallee adopt = cg.module->getOrInsertFunction(
                "csec_rc_adopt_with", llvm::FunctionType::get(llvm::Type::getVoidTy(cg.context), { ptrTy, ptrTy }, false));
            cg.requireSystemNative();
            cg.builder.CreateCall(adopt, { instance, release ? static_cast<llvm::Value*>(release)
                                                             : llvm::ConstantPointerNull::get(ptrTy) });
            cg.registerOwnedTemporary(instance, cg.rcReleaseFunction());
        }
    }
    return instance;
}

// ---- PatternNode ----------------------------------------------------------------------------

PatternNode::PatternNode(const PatternNode& other)
    : kind(other.kind), name(other.name), literal(other.literal ? other.literal->clone() : nullptr),
      enumInstance(other.enumInstance), variantName(other.variantName), tag(other.tag), fieldNames(other.fieldNames),
      fieldTypes(cloneTypes(other.fieldTypes)), valueType(other.valueType ? other.valueType->clone() : nullptr) {
    nodeType = ASTNodeType::PATTERN;
    line = other.line;
    for (const auto& s : other.subpatterns) subpatterns.push_back(s ? std::make_unique<PatternNode>(*s) : nullptr);
}

void PatternNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void PatternNode::collectBindings(std::vector<std::pair<std::string, const Type*>>& out) const {
    if (kind == Kind::Binding) out.push_back({name, valueType.get()});
    for (const auto& s : subpatterns) if (s) s->collectBindings(out);
}

// ---- TryNode --------------------------------------------------------------------------------

TryNode::TryNode(const TryNode& other)
    : expression(other.expression ? other.expression->clone() : nullptr),
      functionReturnType(other.functionReturnType ? other.functionReturnType->clone() : nullptr) {
    nodeType = ASTNodeType::TRY_EXPRESSION;
    line = other.line;
    type = other.type ? other.type->clone() : nullptr;
}

void TryNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }

std::unique_ptr<Type> TryNode::getType() {
    if (type) return type->clone();
    std::vector<std::unique_ptr<Type>> args;
    const EnumInfo* info = expression ? enumOfType(expression->getType().get(), &args) : nullptr;
    if (!info || args.empty()) return std::make_unique<UnknownType>();
    return args[0]->clone();
}

llvm::Value* TryNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    static int counter = 0;
    auto valueType = expression ? expression->getType() : nullptr;
    std::vector<std::unique_ptr<Type>> args;
    const EnumInfo* info = enumOfType(valueType.get(), &args);
    std::vector<std::unique_ptr<Type>> returnArgs;
    const EnumInfo* returnInfo = enumOfType(functionReturnType.get(), &returnArgs);
    if (!info || !returnInfo) {
        CodeGenerator::getInstance().error("'?' needs an Option or Result value in a function returning one");
        return nullptr;
    }
    const bool isOption = info->name == "Option";
    const EnumVariantInfo* success = info->variant(isOption ? "Some" : "Ok");
    const EnumVariantInfo* failure = info->variant(isOption ? "None" : "Err");
    if (!success || !failure || success->storageNames.empty()) return nullptr;

    // Bind the value once, then test its tag.
    const std::string name = "__try_" + std::to_string(counter++);
    VariableDeclarationNode temporary(name, valueType->clone(), std::move(expression), false);
    temporary.hasExplicitType = true;
    temporary.codegen();
    expression = std::move(temporary.initializer);

    AccessFieldNode tagAccess(std::make_unique<IdentifierNode>(name), std::make_unique<IdentifierNode>(enumTagField()));
    llvm::Value* tag = tagAccess.codegen();
    if (!tag) return nullptr;
    llvm::Function* function = cg.builder.GetInsertBlock()->getParent();
    auto* failBB = llvm::BasicBlock::Create(cg.context, "try.fail", function);
    auto* okBB = llvm::BasicBlock::Create(cg.context, "try.ok", function);
    cg.builder.CreateCondBr(
        cg.builder.CreateICmpEQ(tag, llvm::ConstantInt::get(tag->getType(), failure->tag), "try.failed"), failBB, okBB);

    // None / Err(e): return it, rebuilt as the function's own Option/Result type.
    cg.builder.SetInsertPoint(failBB);
    auto construct = std::make_unique<EnumConstructNode>();
    construct->enumName = returnInfo->name;
    construct->variantName = failure->name;
    construct->typeArguments = std::move(returnArgs);
    construct->qualified = true;
    if (!isOption) {
        construct->arguments.push_back(std::make_unique<AccessFieldNode>(
            std::make_unique<IdentifierNode>(name), std::make_unique<IdentifierNode>(failure->storageNames[0])));
    }
    ReturnStatementNode earlyReturn;
    earlyReturn.expression = std::move(construct);
    earlyReturn.codegen();

    cg.builder.SetInsertPoint(okBB);
    AccessFieldNode payload(std::make_unique<IdentifierNode>(name), std::make_unique<IdentifierNode>(success->storageNames[0]));
    return payload.codegen();
}
