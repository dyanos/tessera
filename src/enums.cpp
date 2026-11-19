// enums.cpp -- lowering of `enum` declarations and variant constructions (see enum_nodes.h).
#include "enums.h"
#include "type_checker.h"

#include "ast_walk.h"
#include "codegen.h"
#include "enum_nodes.h"
#include "lexer.h"
#include "parser.h"
#include "source_map.h"

#include <iostream>
#include <map>
#include <set>

static bool trivialPayloadType(const Type* type);

namespace {

std::map<std::string, EnumInfo>& registry() {
    static std::map<std::string, EnumInfo> enums;
    return enums;
}

std::string where(int line) {
    if (line <= 0) return "";
    const SourceLocation location = sourceLocationOf(line);
    if (location.file.empty()) return " (line " + std::to_string(line) + ")";
    return " (" + location.file + ":" + std::to_string(location.line) + ")";
}

// Option and Result, added to a program that uses them without declaring its own.
const char* kPrelude = R"TESSERA(
enum Option<T> {
    Some(value: T),
    None

    def isSome(): Boolean = __tag == 0
    def isNone(): Boolean = __tag != 0
    def unwrap(): T {
        assert(__tag == 0, "Option.unwrap() called on None");
        return __Some_value;
    }
    def unwrapOr(fallback: T): T {
        if (__tag == 0) { return __Some_value; }
        return fallback;
    }
}

enum Result<T, E> {
    Ok(value: T),
    Err(error: E)

    def isOk(): Boolean = __tag == 0
    def isErr(): Boolean = __tag != 0
    def unwrap(): T {
        assert(__tag == 0, "Result.unwrap() called on Err");
        return __Ok_value;
    }
    def unwrapOr(fallback: T): T {
        if (__tag == 0) { return __Ok_value; }
        return fallback;
    }
    def unwrapErr(): E {
        assert(__tag != 0, "Result.unwrapErr() called on Ok");
        return __Err_error;
    }
}
)TESSERA";

bool mentionsType(const Type* type, const std::string& name) {
    if (!type) return false;
    if (type->getName() == name &&
        (type->getKind() == Type::Kind::CLASS || type->getKind() == Type::Kind::BASIC)) return true;
    if (auto* generic = dynamic_cast<const GenericType*>(type)) {
        if (generic->baseType && generic->baseType->getName() == name) return true;
        for (const auto& argument : generic->typeArguments) if (mentionsType(argument.get(), name)) return true;
    }
    if (auto* array = dynamic_cast<const ArrayType*>(type)) return mentionsType(array->elementType.get(), name);
    if (auto* tuple = dynamic_cast<const TupleType*>(type)) {
        for (const auto& e : tuple->elementTypes) if (mentionsType(e.get(), name)) return true;
    }
    return false;
}

// Builds the registry entry and the class declaration an enum lowers to.
std::unique_ptr<ASTNode> lowerEnumDeclaration(EnumDeclarationNode& decl, int& errors) {
    EnumInfo info;
    info.name = decl.name;
    info.typeParameters = decl.typeParameters;
    std::set<std::string> variantNames;
    for (size_t i = 0; i < decl.variants.size(); ++i) {
        auto& variant = decl.variants[i];
        if (!variantNames.insert(variant.name).second) {
            std::cerr << "Error" << where(variant.line) << ": enum '" << decl.name << "' declares variant '"
                      << variant.name << "' twice" << std::endl;
            ++errors;
        }
        EnumVariantInfo v;
        v.name = variant.name;
        v.tag = static_cast<int>(i);
        std::set<std::string> fieldNames;
        for (auto& field : variant.fields) {
            if (!fieldNames.insert(field->name).second) {
                std::cerr << "Error" << where(variant.line) << ": variant '" << variant.name << "' declares field '"
                          << field->name << "' twice" << std::endl;
                ++errors;
            }
            v.fieldNames.push_back(field->name);
            v.storageNames.push_back("__" + variant.name + "_" + field->name);
            v.fieldTypes.push_back(field->type ? field->type->clone() : std::make_unique<UnknownType>());
            if (mentionsType(field->type.get(), decl.name)) info.isReference = true;
            if (isStrictOwnership() && !trivialPayloadType(field->type.get())) info.isReference = true;
            info.hasPayload = true;
        }
        info.variants.push_back(std::move(v));
    }
    if (info.variants.empty()) {
        std::cerr << "Error" << where(decl.line) << ": enum '" << decl.name << "' has no variants" << std::endl;
        ++errors;
    }

    std::vector<std::unique_ptr<ParameterNode>> params;
    params.push_back(std::make_unique<ParameterNode>(enumTagField(), std::make_unique<BasicType>("Int")));
    for (const auto& v : info.variants) {
        for (size_t k = 0; k < v.storageNames.size(); ++k) {
            params.push_back(std::make_unique<ParameterNode>(v.storageNames[k], v.fieldTypes[k]->clone()));
        }
    }
    auto body = std::make_unique<ClassBodyNode>();
    bool hasToString = false;
    for (auto& method : decl.methods) {
        if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(method.get())) {
            if (fn->name == "toString" && fn->parameters.empty()) hasToString = true;
        }
        body->methods.push_back(std::move(method));
    }
    decl.methods.clear();
    if (!hasToString) {
        // `toString()`: the variant name and its payload, e.g. `Rect(2.000000, 3.000000)`. Used by
        // println and string concatenation.
        std::string text = "def toString(): String {\n";
        for (const auto& v : info.variants) {
            // if (__tag == 1) { return "Rect(" + __Rect_w + ", " + __Rect_h + ")"; }
            std::string value = "\"" + v.name;
            if (v.storageNames.empty()) {
                value += "\"";
            }
            else {
                value += "(\"";
                for (size_t k = 0; k < v.storageNames.size(); ++k) {
                    value += (k ? " + \", \" + " : " + ") + v.storageNames[k];
                }
                value += " + \")\"";
            }
            text += "    if (__tag == " + std::to_string(v.tag) + ") { return " + value + "; }\n";
        }
        text += "    return \"?\";\n}\n";
        Lexer lexer(text);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto parsed = parser.parse();
        if (auto* program = dynamic_cast<ProgramNode*>(parsed.get())) {
            for (auto& statement : program->statements) {
                ast_walk::walk(statement.get(), nullptr, [](ASTNode* n, std::unique_ptr<ASTNode>*) { n->line = 0; return true; });
                body->methods.push_back(std::move(statement));
            }
        }
    }
    auto cls = std::make_unique<ClassDeclarationNode>(decl.name, params, "", std::move(body));
    cls->isStruct = !info.isReference;
    cls->line = decl.line;
    cls->interfaces = decl.interfaces;

    const std::string name = decl.name;
    registry()[name] = std::move(info);
    if (decl.typeParameters.empty()) return cls;

    auto tmpl = std::make_unique<TemplateDeclarationNode>();
    tmpl->typeParameters = decl.typeParameters;
    for (const auto& p : decl.typeParameters) tmpl->templateParams.push_back(TemplateParam(p));
    tmpl->declaration = std::move(cls);
    tmpl->line = decl.line;
    return tmpl;
}

// Rewrites variant constructions in function bodies. Names declared in an enclosing scope (a local
// `val None = ...`) shadow a bare variant name.
class ConstructionRewriter {
public:
    int errors = 0;
    std::set<std::string> globalNames;  // top-level functions, classes, objects

    void run(ProgramNode& program) {
        for (auto& statement : program.statements) visitSlot(statement);
    }

private:
    std::vector<std::set<std::string>> scopes;

    bool shadowed(const std::string& name) const {
        for (const auto& scope : scopes) if (scope.count(name)) return true;
        return globalNames.count(name) > 0;
    }
    void declare(const std::string& name) { if (!scopes.empty()) scopes.back().insert(name); }

    // The enum declaring `variant` when exactly one does; reports an ambiguity otherwise.
    const EnumInfo* uniqueEnumOf(const std::string& variant, int line, bool* ambiguous) {
        const EnumInfo* found = nullptr;
        *ambiguous = false;
        for (const auto& entry : registry()) {
            if (!entry.second.variant(variant)) continue;
            if (found) {
                *ambiguous = true;
                std::cerr << "Error" << where(line) << ": '" << variant << "' is a variant of both '" << found->name
                          << "' and '" << entry.second.name << "'; write " << found->name << "." << variant
                          << " or " << entry.second.name << "." << variant << std::endl;
                ++errors;
                return nullptr;
            }
            found = &entry.second;
        }
        return found;
    }

    std::unique_ptr<ASTNode> makeConstruct(const EnumInfo& info, const std::string& variant,
                                           std::vector<std::unique_ptr<ASTNode>> args, bool qualified, int line,
                                           std::vector<std::unique_ptr<Type>> typeArgs = {}) {
        auto node = std::make_unique<EnumConstructNode>();
        node->enumName = info.name;
        node->variantName = variant;
        node->arguments = std::move(args);
        node->qualified = qualified;
        node->typeArguments = std::move(typeArgs);
        node->line = line;
        return node;
    }

    // The replacement for a variant construction, or null.
    std::unique_ptr<ASTNode> rewrite(ASTNode* node) {
        if (auto* access = dynamic_cast<AccessFieldNode*>(node)) {
            auto* base = dynamic_cast<IdentifierNode*>(access->base.get());
            auto* field = dynamic_cast<IdentifierNode*>(access->field.get());
            if (base && field && !shadowed(base->value)) {
                if (const EnumInfo* info = findEnum(base->value)) {
                    if (!info->variant(field->value)) {
                        std::cerr << "Error" << where(node->line) << ": enum '" << info->name << "' has no variant '"
                                  << field->value << "'" << std::endl;
                        ++errors;
                        return nullptr;
                    }
                    return makeConstruct(*info, field->value, {}, true, node->line);
                }
            }
        }
        else if (auto* method = dynamic_cast<MethodCallNode*>(node)) {
            auto* object = dynamic_cast<IdentifierNode*>(method->object.get());
            if (object && !shadowed(object->value)) {
                if (const EnumInfo* info = findEnum(object->value)) {
                    if (!info->variant(method->methodName)) {
                        std::cerr << "Error" << where(node->line) << ": enum '" << info->name << "' has no variant '"
                                  << method->methodName << "'" << std::endl;
                        ++errors;
                        return nullptr;
                    }
                    return makeConstruct(*info, method->methodName, std::move(method->arguments), true, node->line);
                }
            }
        }
        else if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
            if (!shadowed(call->functionName)) {
                bool ambiguous = false;
                if (const EnumInfo* info = uniqueEnumOf(call->functionName, node->line, &ambiguous)) {
                    return makeConstruct(*info, call->functionName, std::move(call->arguments), false, node->line,
                                         std::move(call->explicitTypeArgs));
                }
            }
        }
        else if (auto* id = dynamic_cast<IdentifierNode*>(node)) {
            if (id->value.find('.') == std::string::npos && !shadowed(id->value)) {
                bool ambiguous = false;
                if (const EnumInfo* info = uniqueEnumOf(id->value, node->line, &ambiguous)) {
                    return makeConstruct(*info, id->value, {}, false, node->line);
                }
            }
        }
        else if (auto* construct = dynamic_cast<EnumConstructNode*>(node)) {
            // Written as `Option<Int>.None` (parsed directly).
            const EnumInfo* info = findEnum(construct->enumName);
            if (!info) {
                std::cerr << "Error" << where(node->line) << ": '" << construct->enumName << "' is not an enum" << std::endl;
                ++errors;
            }
            else if (!info->variant(construct->variantName)) {
                std::cerr << "Error" << where(node->line) << ": enum '" << info->name << "' has no variant '"
                          << construct->variantName << "'" << std::endl;
                ++errors;
            }
        }
        return nullptr;
    }

    // Names a raw `case` pattern binds (identifiers that are not variant names), approximately.
    void declarePatternNames(ASTNode* pattern) {
        if (!pattern) return;
        if (auto* id = dynamic_cast<IdentifierNode*>(pattern)) {
            bool isVariant = false;
            for (const auto& entry : registry()) if (entry.second.variant(id->value)) isVariant = true;
            if (!isVariant && id->value.find('.') == std::string::npos) declare(id->value);
            return;
        }
        if (auto* call = dynamic_cast<FunctionCallNode*>(pattern)) {
            for (auto& a : call->arguments) declarePatternNames(a.get());
        }
        else if (auto* method = dynamic_cast<MethodCallNode*>(pattern)) {
            for (auto& a : method->arguments) declarePatternNames(a.get());
        }
    }

    void visitChildren(ASTNode* node) {
        ast_walk::forEachChild(node, [&](ASTNode* child, std::unique_ptr<ASTNode>* slot) {
            if (slot) visitSlot(*slot);
            else visitNode(child);
        });
    }

    void visitSlot(std::unique_ptr<ASTNode>& slot) {
        if (!slot) return;
        if (auto replacement = rewrite(slot.get())) {
            replacement->line = replacement->line ? replacement->line : slot->line;
            slot = std::move(replacement);
        }
        visitNode(slot.get());
    }

    void visitNode(ASTNode* node) {
        if (!node) return;
        if (auto* match = dynamic_cast<MatchExpressionNode*>(node)) {
            if (match->expression) visitSlot(match->expression);
            for (size_t i = 0; i < match->cases.size(); ++i) {
                scopes.emplace_back();
                declarePatternNames(match->cases[i].first.get());
                if (i < match->guards.size() && match->guards[i]) visitSlot(match->guards[i]);
                visitSlot(match->cases[i].second);
                scopes.pop_back();
            }
            return;
        }
        if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(node)) {
            scopes.emplace_back();
            for (auto& p : fn->parameters) {
                if (auto* param = dynamic_cast<ParameterNode*>(p.get())) {
                    declare(param->name);
                    if (param->defaultValue) visitSlot(param->defaultValue);
                }
            }
            visitNode(fn->body.get());
            scopes.pop_back();
            return;
        }
        if (auto* cls = dynamic_cast<ClassDeclarationNode*>(node)) {
            scopes.emplace_back();
            for (auto& p : cls->constructorParams) {
                if (auto* param = dynamic_cast<ParameterNode*>(p.get())) declare(param->name);
            }
            if (auto* body = dynamic_cast<ClassBodyNode*>(cls->body.get())) {
                for (auto& f : body->fields) {
                    if (auto* field = dynamic_cast<VariableDeclarationNode*>(f.get())) declare(field->name);
                }
            }
            visitChildren(node);
            scopes.pop_back();
            return;
        }
        if (auto* lambda = dynamic_cast<LambdaExpressionNode*>(node)) {
            scopes.emplace_back();
            for (auto& p : lambda->arguments) {
                if (auto* param = dynamic_cast<ParameterNode*>(p.get())) declare(param->name);
            }
            visitSlot(lambda->body);
            scopes.pop_back();
            return;
        }
        if (dynamic_cast<BlockNode*>(node)) {
            scopes.emplace_back();
            visitChildren(node);
            scopes.pop_back();
            return;
        }
        if (auto* loop = dynamic_cast<ForStatementNode*>(node)) {
            visitSlot(loop->iterableExpr);
            scopes.emplace_back();
            declare(loop->variable);
            if (!loop->tokenValue.empty()) declare(loop->tokenValue);
            visitNode(loop->body.get());
            scopes.pop_back();
            return;
        }
        if (auto* decl = dynamic_cast<VariableDeclarationNode*>(node)) {
            visitSlot(decl->initializer);
            declare(decl->name);
            return;
        }
        if (auto* destructuring = dynamic_cast<DestructuringAssignmentNode*>(node)) {
            visitSlot(destructuring->value);
            for (const auto& target : destructuring->targets) declare(target.name);
            return;
        }
        if (auto* tmpl = dynamic_cast<TemplateDeclarationNode*>(node)) {
            visitNode(tmpl->declaration.get());
            return;
        }
        if (auto* access = dynamic_cast<AccessFieldNode*>(node)) {
            visitSlot(access->base);  // the field name is not an expression
            return;
        }
        if (auto* assign = dynamic_cast<AssignmentExpressionNode*>(node)) {
            if (assign->isNamedArgument) {  // `name = value`: the name is a parameter, not an expression
                visitSlot(assign->right);
                return;
            }
        }
        visitChildren(node);
    }
};

bool programMentions(ASTNode* program, const std::set<std::string>& names) {
    bool found = false;
    ast_walk::walk(program, nullptr, [&](ASTNode* node, std::unique_ptr<ASTNode>*) {
        if (found) return false;
        if (auto* id = dynamic_cast<IdentifierNode*>(node)) {
            if (names.count(id->value)) found = true;
            const auto dot = id->value.find('.');
            if (dot != std::string::npos && names.count(id->value.substr(0, dot))) found = true;
        }
        else if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
            if (names.count(call->functionName)) found = true;
        }
        else if (auto* construct = dynamic_cast<EnumConstructNode*>(node)) {
            if (names.count(construct->enumName)) found = true;
        }
        auto typeMentions = [&](const Type* t) {
            for (const auto& n : names) if (mentionsType(t, n)) return true;
            return false;
        };
        if (auto* param = dynamic_cast<ParameterNode*>(node)) { if (typeMentions(param->type.get())) found = true; }
        else if (auto* decl = dynamic_cast<VariableDeclarationNode*>(node)) { if (typeMentions(decl->type.get())) found = true; }
        else if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(node)) { if (typeMentions(fn->returnType.get())) found = true; }
        return !found;
    });
    return found;
}

}  // namespace

const EnumVariantInfo* EnumInfo::variant(const std::string& variantName) const {
    for (const auto& v : variants) if (v.name == variantName) return &v;
    return nullptr;
}


static bool trivialPayloadType(const Type* type) {
    if (!type || type->getKind() != Type::Kind::BASIC) return false;
    static const std::set<std::string> scalars = {
        "Int", "Long", "Short", "Byte", "Double", "Float", "Real", "Bool", "Boolean", "Char", "Natural", "Integer"};
    return scalars.count(type->getName()) != 0;
}

const EnumInfo* findEnum(const std::string& name) {
    auto it = registry().find(name);
    return it == registry().end() ? nullptr : &it->second;
}

const EnumInfo* enumDeclaringVariant(const std::string& variant) {
    for (const auto& entry : registry()) if (entry.second.variant(variant)) return &entry.second;
    return nullptr;
}

const EnumInfo* enumOfType(const Type* type, std::vector<std::unique_ptr<Type>>* typeArguments) {
    if (!type) return nullptr;
    if (auto* borrow = dynamic_cast<const BorrowType*>(type)) return enumOfType(borrow->baseType.get(), typeArguments);
    if (type->getKind() != Type::Kind::CLASS) return nullptr;
    if (const EnumInfo* info = findEnum(type->getName())) {
        if (info->typeParameters.empty()) {
            if (typeArguments) typeArguments->clear();
            return info;
        }
        return nullptr;  // a generic enum's own name is not a value type
    }
    std::string templateName;
    std::vector<std::unique_ptr<Type>> arguments;
    if (!templateInstanceArguments(type->getName(), &templateName, &arguments)) return nullptr;
    const EnumInfo* info = findEnum(templateName);
    if (!info) return nullptr;
    if (typeArguments) *typeArguments = std::move(arguments);
    return info;
}

namespace {
void substituteInPlace(std::unique_ptr<Type>& type, const EnumInfo& info, const std::vector<std::unique_ptr<Type>>& args) {
    if (!type) return;
    if (type->getKind() == Type::Kind::VARIABLE || type->getKind() == Type::Kind::CLASS ||
        type->getKind() == Type::Kind::BASIC) {
        for (size_t i = 0; i < info.typeParameters.size() && i < args.size(); ++i) {
            if (type->getName() == info.typeParameters[i] && args[i]) {
                type = args[i]->clone();
                return;
            }
        }
        // The enum itself (a recursive, non-generic enum): already the lowered class name.
        return;
    }
    if (auto* generic = dynamic_cast<GenericType*>(type.get())) {
        for (auto& a : generic->typeArguments) substituteInPlace(a, info, args);
        const std::string base = generic->baseType ? generic->baseType->getName() : "";
        auto* symbol = CodeGenerator::getInstance().symbolTable.lookup(base);
        if (symbol && symbol->symbolType == SymbolType::TEMPLATE) {
            instantiateTemplateClass(base, generic->typeArguments);
            type = std::make_unique<ClassType>(templateInstanceName(base, generic->typeArguments));
        }
        return;
    }
    if (auto* array = dynamic_cast<ArrayType*>(type.get())) { substituteInPlace(array->elementType, info, args); return; }
    if (auto* tuple = dynamic_cast<TupleType*>(type.get())) {
        for (auto& e : tuple->elementTypes) substituteInPlace(e, info, args);
        return;
    }
}
}  // namespace

std::unique_ptr<Type> substituteEnumTypeParameters(const Type* type, const EnumInfo& info,
                                                   const std::vector<std::unique_ptr<Type>>& arguments) {
    if (!type) return std::make_unique<UnknownType>();
    auto result = const_cast<Type*>(type)->clone();
    substituteInPlace(result, info, arguments);
    return result;
}

std::string enumInstanceClass(const EnumInfo& info, const std::vector<std::unique_ptr<Type>>& arguments) {
    if (info.typeParameters.empty()) return info.name;
    instantiateTemplateClass(info.name, arguments);
    return templateInstanceName(info.name, arguments);
}

bool lowerEnums(ASTNode* root) {
    auto* program = dynamic_cast<ProgramNode*>(root);
    if (!program) return true;
    int errors = 0;

    // The built-in Option/Result, unless the program declares enums of those names itself.
    bool declaresOption = false, declaresResult = false;
    for (auto& statement : program->statements) {
        if (auto* e = dynamic_cast<EnumDeclarationNode*>(statement.get())) {
            if (e->name == "Option") declaresOption = true;
            if (e->name == "Result") declaresResult = true;
        }
    }
    const bool wantOption = !declaresOption && programMentions(program, {"Option", "Some", "None"});
    const bool wantResult = !declaresResult && programMentions(program, {"Result", "Ok", "Err"});
    if (wantOption || wantResult) {
        Lexer lexer(kPrelude);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto prelude = parser.parse();
        auto* preludeProgram = dynamic_cast<ProgramNode*>(prelude.get());
        std::vector<std::unique_ptr<ASTNode>> added;
        for (auto& statement : preludeProgram->statements) {
            auto* e = dynamic_cast<EnumDeclarationNode*>(statement.get());
            if (!e || (e->name == "Option" && !wantOption) || (e->name == "Result" && !wantResult)) continue;
            // Prelude lines are not lines of the user's program.
            ast_walk::walk(statement.get(), nullptr, [](ASTNode* n, std::unique_ptr<ASTNode>*) { n->line = 0; return true; });
            for (auto& v : e->variants) v.line = 0;
            added.push_back(std::move(statement));
        }
        for (auto& statement : program->statements) added.push_back(std::move(statement));
        program->statements = std::move(added);
    }

    ConstructionRewriter rewriter;
    for (auto& statement : program->statements) {
        if (!statement) continue;
        if (auto* e = dynamic_cast<EnumDeclarationNode*>(statement.get())) {
            if (findEnum(e->name)) {
                std::cerr << "Error" << where(e->line) << ": enum '" << e->name << "' is declared twice" << std::endl;
                ++errors;
            }
            statement = lowerEnumDeclaration(*e, errors);
        }
    }
    for (auto& statement : program->statements) {
        ASTNode* s = statement.get();
        if (auto* tmpl = dynamic_cast<TemplateDeclarationNode*>(s)) s = tmpl->declaration.get();
        if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(s)) rewriter.globalNames.insert(fn->name);
        else if (auto* cls = dynamic_cast<ClassDeclarationNode*>(s)) { if (!findEnum(cls->name)) rewriter.globalNames.insert(cls->name); }
        else if (auto* object = dynamic_cast<ObjectDeclarationNode*>(s)) rewriter.globalNames.insert(object->name);
    }
    if (!registry().empty()) rewriter.run(*program);
    return errors == 0 && rewriter.errors == 0;
}
