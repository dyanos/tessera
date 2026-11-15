// interfaces.cpp -- interfaces, conformance, interface values and dynamic dispatch (see interfaces.h).
#include "codegen.h"

#include "interfaces.h"
#include "ast_walk.h"
#include "source_map.h"
#include "type_utils.h"

#include <iostream>
#include <sstream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <map>

namespace {

std::map<std::string, InterfaceInfo>& interfaces() {
    static std::map<std::string, InterfaceInfo> table;
    return table;
}

// class name -> declared interfaces / parent class (from the declarations)
std::map<std::string, std::vector<std::string>>& classInterfaces() {
    static std::map<std::string, std::vector<std::string>> table;
    return table;
}
std::map<std::string, std::string>& classParents() {
    static std::map<std::string, std::string> table;
    return table;
}

std::string where(int line) {
    if (line <= 0) return "";
    const SourceLocation location = sourceLocationOf(line);
    if (location.file.empty()) return " (line " + std::to_string(line) + ")";
    return " (" + location.file + ":" + std::to_string(location.line) + ")";
}

std::string typeText(const Type* type) {
    if (!type) return "Unit";
    if (auto* generic = dynamic_cast<const GenericType*>(type)) {
        std::string text = generic->getName() + "<";
        for (size_t i = 0; i < generic->typeArguments.size(); ++i) {
            text += (i ? ", " : "") + typeText(generic->typeArguments[i].get());
        }
        return text + ">";
    }
    if (auto* array = dynamic_cast<const ArrayType*>(type)) return "Array[" + typeText(array->elementType.get()) + "]";
    const std::string name = type->getName();
    return name == "Bool" ? "Boolean" : name;
}

bool sameType(const Type* a, const Type* b) {
    const std::string ta = typeText(a), tb = typeText(b);
    if (ta == tb) return true;
    return (ta == "Unit" || ta == "void") && (tb == "Unit" || tb == "void");
}

std::string signatureText(const std::string& name, const std::vector<const Type*>& params, const Type* ret) {
    std::string text = name + "(";
    for (size_t i = 0; i < params.size(); ++i) text += (i ? ", " : "") + typeText(params[i]);
    return text + "): " + typeText(ret);
}

// Default method bodies, by interface.
std::map<std::string, std::vector<std::unique_ptr<FunctionDeclarationNode>>>& defaultMethods() {
    static std::map<std::string, std::vector<std::unique_ptr<FunctionDeclarationNode>>> table;
    return table;
}

std::vector<const Type*> parameterTypesOf(FunctionDeclarationNode& fn) {
    std::vector<const Type*> types;
    for (auto& p : fn.parameters) {
        if (auto* param = dynamic_cast<ParameterNode*>(p.get())) types.push_back(param->type.get());
    }
    return types;
}

}  // namespace

// ---- nodes ----------------------------------------------------------------------------------

InterfaceDeclarationNode::InterfaceDeclarationNode(const InterfaceDeclarationNode& other) : name(other.name) {
    nodeType = ASTNodeType::INTERFACE_DECLARATION;
    line = other.line;
    for (const auto& m : other.methods) methods.push_back(std::make_unique<FunctionDeclarationNode>(*m));
}

void InterfaceDeclarationNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }

InterfaceWrapNode::InterfaceWrapNode(const InterfaceWrapNode& other)
    : expression(other.expression ? other.expression->clone() : nullptr), className(other.className),
      interfaceName(other.interfaceName) {
    nodeType = ASTNodeType::INTERFACE_WRAP;
    line = other.line;
}

void InterfaceWrapNode::accept(ASTVisitor& visitor) { visitor.visit(*this); }

// ---- registry -------------------------------------------------------------------------------

int InterfaceInfo::methodIndex(const std::string& methodName, size_t arity) const {
    for (size_t i = 0; i < methods.size(); ++i) {
        if (methods[i].name == methodName && methods[i].parameterTypes.size() == arity) return static_cast<int>(i);
    }
    return -1;
}

const InterfaceInfo* findInterface(const std::string& name) {
    auto it = interfaces().find(name);
    return it == interfaces().end() ? nullptr : &it->second;
}

bool isInterfaceType(const Type* type) {
    return type && type->getKind() == Type::Kind::CLASS && findInterface(type->getName()) != nullptr;
}

const std::vector<std::string>* declaredInterfaces(const std::string& className) {
    auto it = classInterfaces().find(className);
    return it == classInterfaces().end() ? nullptr : &it->second;
}

bool implementsInterface(const Type* type, const std::string& interfaceName) {
    if (!type) return false;
    if (type->getName() == interfaceName) return true;
    std::string name = type->getName();
    std::string templateName;
    if (templateInstanceArguments(name, &templateName, nullptr)) name = templateName;
    std::set<std::string> seen;
    while (!name.empty() && seen.insert(name).second) {
        if (auto* list = declaredInterfaces(name)) {
            for (const auto& i : *list) if (i == interfaceName) return true;
        }
        auto parent = classParents().find(name);
        name = parent == classParents().end() ? std::string() : parent->second;
    }
    return false;
}

// ---- lowering -------------------------------------------------------------------------------

bool lowerInterfaces(ASTNode* root) {
    auto* program = dynamic_cast<ProgramNode*>(root);
    if (!program) return true;
    int errors = 0;

    // 1. Register the interfaces and drop their declarations.
    std::vector<std::unique_ptr<ASTNode>> kept;
    for (auto& statement : program->statements) {
        auto* decl = dynamic_cast<InterfaceDeclarationNode*>(statement.get());
        if (!decl) {
            kept.push_back(std::move(statement));
            continue;
        }
        if (findInterface(decl->name)) {
            std::cerr << "Error" << where(decl->line) << ": interface '" << decl->name << "' is declared twice" << std::endl;
            ++errors;
            continue;
        }
        InterfaceInfo info;
        info.name = decl->name;
        for (auto& m : decl->methods) {
            InterfaceMethod method;
            method.name = m->name;
            for (auto& p : m->parameters) {
                if (auto* param = dynamic_cast<ParameterNode*>(p.get())) {
                    method.parameterNames.push_back(param->name);
                    method.parameterTypes.push_back(param->type ? param->type->clone() : std::make_unique<UnknownType>());
                }
            }
            method.returnType = m->returnType ? m->returnType->clone() : std::make_unique<BasicType>("Unit");
            for (const auto& existing : info.methods) {
                if (existing.name == method.name && existing.parameterTypes.size() == method.parameterTypes.size()) {
                    std::cerr << "Error" << where(m->line) << ": interface '" << decl->name << "' declares '" << m->name
                              << "' twice" << std::endl;
                    ++errors;
                }
            }
            info.methods.push_back(std::move(method));
            if (m->body) defaultMethods()[decl->name].push_back(std::move(m));
        }
        interfaces()[decl->name] = std::move(info);
    }
    program->statements = std::move(kept);

    // 2. Record every class's parent and interfaces.
    std::map<std::string, ClassDeclarationNode*> classes;
    for (auto& statement : program->statements) {
        ASTNode* s = statement.get();
        if (auto* tmpl = dynamic_cast<TemplateDeclarationNode*>(s)) s = tmpl->declaration.get();
        if (auto* cls = dynamic_cast<ClassDeclarationNode*>(s)) {
            classes[cls->name] = cls;
            if (!cls->superClassName.empty()) classParents()[cls->name] = cls->superClassName;
            if (!cls->interfaces.empty()) classInterfaces()[cls->name] = cls->interfaces;
        }
    }

    // 3. Conformance: every interface method is implemented (or has a default, copied in).
    for (auto& entry : classes) {
        ClassDeclarationNode* cls = entry.second;
        for (const auto& interfaceName : cls->interfaces) {
            const InterfaceInfo* info = findInterface(interfaceName);
            if (!info) {
                std::cerr << "Error" << where(cls->line) << ": '" << cls->name << "' implements '" << interfaceName
                          << "', which is not an interface" << std::endl;
                ++errors;
                continue;
            }
            for (const auto& method : info->methods) {
                std::vector<const Type*> wanted;
                for (const auto& t : method.parameterTypes) wanted.push_back(t.get());
                // Look in the class and its parents.
                FunctionDeclarationNode* found = nullptr;
                FunctionDeclarationNode* sameName = nullptr;
                for (ClassDeclarationNode* c = cls; c && !found; ) {
                    if (auto* body = dynamic_cast<ClassBodyNode*>(c->body.get())) {
                        for (auto& m : body->methods) {
                            auto* fn = dynamic_cast<FunctionDeclarationNode*>(m.get());
                            if (!fn || fn->name != method.name) continue;
                            sameName = fn;
                            auto params = parameterTypesOf(*fn);
                            bool match = params.size() == wanted.size();
                            for (size_t k = 0; match && k < params.size(); ++k) match = sameType(params[k], wanted[k]);
                            if (match) { found = fn; break; }
                        }
                    }
                    auto parent = classes.find(c->superClassName);
                    c = parent == classes.end() ? nullptr : parent->second;
                }
                const std::string expected = signatureText(method.name, wanted, method.returnType.get());
                if (found) {
                    if (!sameType(found->returnType.get(), method.returnType.get())) {
                        std::cerr << "Error" << where(found->line) << ": " << cls->name << "." << method.name << " returns '"
                                  << typeText(found->returnType.get()) << "', but interface '" << interfaceName
                                  << "' declares " << expected << std::endl;
                        ++errors;
                    }
                    continue;
                }
                FunctionDeclarationNode* fallback = nullptr;
                for (auto& d : defaultMethods()[interfaceName]) {
                    if (d->name == method.name && d->parameters.size() == wanted.size()) fallback = d.get();
                }
                if (fallback) {
                    auto* body = dynamic_cast<ClassBodyNode*>(cls->body.get());
                    if (!body) {
                        cls->body = std::make_unique<ClassBodyNode>();
                        body = static_cast<ClassBodyNode*>(cls->body.get());
                    }
                    auto copy = std::make_unique<FunctionDeclarationNode>(*fallback);
                    // In the default body, `area()` means this object's `area()`.
                    std::set<std::string> methodNames;
                    for (const auto& m : info->methods) methodNames.insert(m.name);
                    ast_walk::walk(copy->body.get(), nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>* slot) {
                        auto* call = dynamic_cast<FunctionCallNode*>(n);
                        if (call && slot && methodNames.count(call->functionName)) {
                            auto method = std::make_unique<MethodCallNode>();
                            method->line = call->line;
                            method->object = std::make_unique<IdentifierNode>("this");
                            method->methodName = call->functionName;
                            method->arguments = std::move(call->arguments);
                            *slot = std::move(method);
                        }
                        return true;
                    });
                    body->methods.push_back(std::move(copy));
                    continue;
                }
                std::cerr << "Error" << where(sameName ? sameName->line : cls->line) << ": '" << cls->name
                          << "' does not implement " << expected << " of interface '" << interfaceName << "'"
                          << (sameName ? " (its '" + method.name + "' has different parameters)" : std::string()) << std::endl;
                ++errors;
            }
        }
    }
    return errors == 0;
}

// ---- code generation ------------------------------------------------------------------------

namespace {
// One thunk per interface method: (object, args...) -> object.method(args...).
llvm::Function* makeThunk(const std::string& className, const InterfaceInfo& info, const InterfaceMethod& method) {
    auto& cg = CodeGenerator::getInstance();
    const std::string name = "__vt." + className + "." + info.name + "." + method.name + "." +
                             std::to_string(method.parameterTypes.size());
    if (auto* existing = cg.module->getFunction(name)) return existing;
    auto* ptrTy = llvm::PointerType::getUnqual(cg.context);
    std::vector<llvm::Type*> params{ptrTy};
    for (const auto& t : method.parameterTypes) params.push_back(getABIStorageType(t.get()));
    const bool isVoid = !method.returnType || method.returnType->getName() == "Unit" || method.returnType->isVoidTy();
    llvm::Type* ret = isVoid ? llvm::Type::getVoidTy(cg.context) : getABIStorageType(method.returnType.get());
    auto* fn = llvm::Function::Create(llvm::FunctionType::get(ret, params, false), llvm::Function::InternalLinkage,
                                      name, cg.module.get());

    llvm::BasicBlock* savedBB = cg.builder.GetInsertBlock();
    auto savedPoint = cg.builder.GetInsertPoint();
    cg.builder.SetInsertPoint(llvm::BasicBlock::Create(cg.context, "entry", fn));
    cg.symbolTable.enterScope();
    cg.enterCleanupScope();
    const std::string self = "__self";
    auto selfType = std::make_unique<ClassType>(className);
    llvm::Value* selfValue = fn->getArg(0);
    if (isStructClassType(selfType.get())) {
        // A struct variable is a slot holding the struct.
        selfValue = fn->getArg(0);
    }
    cg.symbolTable.addSymbol(self, std::make_unique<Symbol>(self, std::move(selfType), selfValue, false, SymbolType::VARIABLE));
    MethodCallNode call;
    call.object = std::make_unique<IdentifierNode>(self);
    call.methodName = method.name;
    for (size_t i = 0; i < method.parameterTypes.size(); ++i) {
        const std::string param = "__arg" + std::to_string(i);
        cg.symbolTable.addSymbol(param, std::make_unique<Symbol>(param, method.parameterTypes[i]->clone(),
                                                                 fn->getArg(static_cast<unsigned>(i + 1)), false,
                                                                 SymbolType::VARIABLE));
        call.arguments.push_back(std::make_unique<IdentifierNode>(param));
    }
    llvm::Value* result = call.codegen();
    if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
        if (isVoid) cg.builder.CreateRetVoid();
        else {
            result = result ? coerceValueToLLVMType(result, ret) : nullptr;
            if (!result || result->getType() != ret) result = llvm::Constant::getNullValue(ret);
            cg.builder.CreateRet(result);
        }
    }
    cg.exitCleanupScope();
    cg.symbolTable.exitScope();
    if (savedBB) cg.builder.SetInsertPoint(savedBB, savedPoint);
    return fn;
}

llvm::Value* vtableFor(const std::string& className, const InterfaceInfo& info) {
    auto& cg = CodeGenerator::getInstance();
    const std::string name = "__vtable." + className + "." + info.name;
    if (auto* existing = cg.module->getGlobalVariable(name, true)) return existing;
    auto* ptrTy = llvm::PointerType::getUnqual(cg.context);
    std::vector<llvm::Constant*> entries;
    for (const auto& method : info.methods) entries.push_back(makeThunk(className, info, method));
    auto* arrayTy = llvm::ArrayType::get(ptrTy, entries.size());
    return new llvm::GlobalVariable(*cg.module, arrayTy, true, llvm::GlobalValue::InternalLinkage,
                                    llvm::ConstantArray::get(arrayTy, entries), name);
}
}  // namespace

llvm::Value* InterfaceWrapNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    const InterfaceInfo* info = findInterface(interfaceName);
    if (!info || !expression) return nullptr;
    llvm::Value* value = expression->codegen();
    if (!value) return nullptr;
    auto* ptrTy = llvm::PointerType::getUnqual(cg.context);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    ClassType classType(className);
    llvm::Value* object = value;
    if (isStructClassType(&classType)) {
        // A struct is a value: the interface value holds a heap copy.
        llvm::Type* structTy = cg.getLLVMType(&classType);
        llvm::Value* aggregate = coerceValueToLLVMType(value, structTy);
        const uint64_t size = cg.module->getDataLayout().getTypeAllocSize(structTy);
        object = cg.builder.CreateCall(cg.mallocFunction, llvm::ConstantInt::get(i64Ty, size), "iface.copy");
        cg.builder.CreateStore(aggregate, object);
    }
    llvm::Value* vtable = vtableFor(className, *info);
    auto* pairTy = llvm::StructType::get(cg.context, {ptrTy, ptrTy});
    llvm::Value* pair = cg.builder.CreateCall(cg.mallocFunction, llvm::ConstantInt::get(i64Ty, 16), "iface.value");
    cg.builder.CreateStore(object, cg.builder.CreateStructGEP(pairTy, pair, 0, "iface.object.ptr"));
    cg.builder.CreateStore(vtable, cg.builder.CreateStructGEP(pairTy, pair, 1, "iface.vtable.ptr"));
    return pair;
}

llvm::Value* codegenInterfaceCall(MethodCallNode& call) {
    auto& cg = CodeGenerator::getInstance();
    auto receiverType = call.object ? call.object->getType() : nullptr;
    const InterfaceInfo* info = receiverType ? findInterface(receiverType->getName()) : nullptr;
    const int index = info ? info->methodIndex(call.methodName, call.arguments.size()) : -1;
    if (index < 0) {
        { std::ostringstream os_; os_ << "interface has no method '" << call.methodName << "'"; CodeGenerator::getInstance().reportError(os_.str()); }
        return nullptr;
    }
    const InterfaceMethod& method = info->methods[static_cast<size_t>(index)];
    llvm::Value* pair = call.object->codegen();
    if (!pair) return nullptr;
    auto* ptrTy = llvm::PointerType::getUnqual(cg.context);
    if (!pair->getType()->isPointerTy()) return nullptr;
    auto* pairTy = llvm::StructType::get(cg.context, {ptrTy, ptrTy});
    llvm::Value* object = cg.builder.CreateLoad(ptrTy, cg.builder.CreateStructGEP(pairTy, pair, 0), "iface.object");
    llvm::Value* vtable = cg.builder.CreateLoad(ptrTy, cg.builder.CreateStructGEP(pairTy, pair, 1), "iface.vtable");
    llvm::Value* slot = cg.builder.CreateGEP(ptrTy, vtable, llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), index));
    llvm::Value* target = cg.builder.CreateLoad(ptrTy, slot, call.methodName + ".impl");

    std::vector<llvm::Type*> params{ptrTy};
    std::vector<llvm::Value*> args{object};
    for (size_t i = 0; i < call.arguments.size(); ++i) {
        llvm::Type* paramTy = getABIStorageType(method.parameterTypes[i].get());
        llvm::Value* arg = call.arguments[i]->codegen();
        if (!arg || !paramTy) return nullptr;
        params.push_back(paramTy);
        args.push_back(coerceValueToLLVMType(arg, paramTy));
    }
    const bool isVoid = !method.returnType || method.returnType->getName() == "Unit" || method.returnType->isVoidTy();
    llvm::Type* ret = isVoid ? llvm::Type::getVoidTy(cg.context) : getABIStorageType(method.returnType.get());
    llvm::Value* result = cg.builder.CreateCall(llvm::FunctionType::get(ret, params, false), target, args);
    return isVoid ? llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), 0) : result;
}
