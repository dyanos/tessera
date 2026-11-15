// default_arguments.cpp -- default parameter values and named call arguments.
//
//   def f(x: Int, scale: Double = 1.0e0, offset: Int = 0): Double
//   f(3);                  // f(3, 1.0e0, 0)
//   f(3, offset = 2);      // f(3, 1.0e0, 2)
//
// checkDefaultParameters validates the declarations right after parsing. The calls are expanded by
// the type checker once it knows what a call refers to (expandCallArguments): a top-level or
// template function -- not a local closure of the same name --, a function of an `object`, a
// method of the receiver's class (the most derived class declaring it), or a constructor. Named
// arguments move to their parameter's position and each missing argument becomes a copy of the
// parameter's default, evaluated at the call site; a default whose names a local of the call site
// would shadow is an error. A call that already fills a signature positionally is left alone.
#include "default_arguments.h"

#include "ast_walk.h"
#include "source_map.h"

#include <functional>
#include <iostream>
#include <map>
#include <set>

namespace {

std::string where(int line) {
    if (line <= 0) return "";
    const SourceLocation location = sourceLocationOf(line);
    if (location.file.empty()) return " (line " + std::to_string(line) + ")";
    return " (" + location.file + ":" + std::to_string(location.line) + ")";
}

struct Signature {
    std::string display;                     // for diagnostics
    std::vector<ParameterNode*> parameters;
    FunctionDeclarationNode* declaration = nullptr;
};

struct SignatureIndex {
    std::map<std::string, std::vector<Signature>> functions;     // top-level (and template) `def f`
    std::map<std::string, std::vector<Signature>> namespaced;    // `Obj.f` (object members)
    std::map<std::string, std::vector<Signature>> constructors;  // `new C(...)`
    std::map<std::string, std::vector<Signature>> classMethods;  // `C#m`
    std::map<std::string, std::string> superClass;               // class -> parent
};

SignatureIndex& index() {
    static SignatureIndex table;
    return table;
}

std::vector<ParameterNode*> parametersOf(std::vector<std::unique_ptr<ASTNode>>& nodes) {
    std::vector<ParameterNode*> result;
    for (auto& node : nodes) {
        if (auto* p = dynamic_cast<ParameterNode*>(node.get())) result.push_back(p);
    }
    return result;
}

void indexClass(SignatureIndex& table, ClassDeclarationNode& cls) {
    table.constructors[cls.name].push_back({"new " + cls.name, parametersOf(cls.constructorParams)});
    if (!cls.superClassName.empty()) table.superClass[cls.name] = cls.superClassName;
    auto* body = dynamic_cast<ClassBodyNode*>(cls.body.get());
    if (!body) return;
    for (auto& m : body->methods) {
        auto* method = dynamic_cast<FunctionDeclarationNode*>(m.get());
        if (!method) continue;
        table.classMethods[cls.name + "#" + method->name].push_back(
            {cls.name + "." + method->name, parametersOf(method->parameters), method});
    }
}

void indexTopLevel(SignatureIndex& table, ASTNode* statement) {
    if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(statement)) {
        table.functions[fn->name].push_back({fn->name, parametersOf(fn->parameters)});
    }
    else if (auto* cls = dynamic_cast<ClassDeclarationNode*>(statement)) {
        indexClass(table, *cls);
    }
    else if (auto* object = dynamic_cast<ObjectDeclarationNode*>(statement)) {
        auto* body = dynamic_cast<BlockNode*>(object->body.get());
        if (!body) return;
        for (auto& s : body->statements) {
            if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(s.get())) {
                table.namespaced[object->name + "." + fn->name].push_back(
                    {object->name + "." + fn->name, parametersOf(fn->parameters)});
            }
        }
    }
    else if (auto* tmpl = dynamic_cast<TemplateDeclarationNode*>(statement)) {
        indexTopLevel(table, tmpl->declaration.get());
    }
    else if (auto* attribute = dynamic_cast<AttributeNode*>(statement)) {
        indexTopLevel(table, attribute->target.get());
    }
}

std::string receiverName(ASTNode* node) {
    if (auto* id = dynamic_cast<IdentifierNode*>(node)) return id->value;
    return "";
}

// Maps the call's arguments onto `sig`. Returns false (with `why`) when they do not fit.
bool bindArguments(std::vector<std::unique_ptr<ASTNode>>& args, const Signature& sig,
          std::vector<std::unique_ptr<ASTNode>>* out, std::string* why) {
    const size_t n = sig.parameters.size();
    std::vector<ASTNode*> slots(n, nullptr);
    std::vector<size_t> source(n, SIZE_MAX);
    size_t positional = 0;
    bool sawNamed = false;
    for (size_t i = 0; i < args.size(); ++i) {
        auto* named = dynamic_cast<AssignmentExpressionNode*>(args[i].get());
        if (named && named->isNamedArgument) {
            sawNamed = true;
            const std::string name = receiverName(named->left.get());
            size_t k = 0;
            while (k < n && sig.parameters[k]->name != name) ++k;
            if (k == n) { if (why) *why = "has no parameter named '" + name + "'"; return false; }
            if (slots[k]) { if (why) *why = "got two values for parameter '" + name + "'"; return false; }
            slots[k] = named->right.get();
            source[k] = i;
            continue;
        }
        if (sawNamed) { if (why) *why = "a positional argument follows a named one"; return false; }
        if (positional >= n) {
            if (why) *why = "takes " + std::to_string(n) + " argument(s), got " + std::to_string(args.size());
            return false;
        }
        slots[positional] = args[i].get();
        source[positional] = i;
        ++positional;
    }
    for (size_t k = 0; k < n; ++k) {
        if (!slots[k] && !sig.parameters[k]->defaultValue) {
            if (why) *why = "is missing a value for parameter '" + sig.parameters[k]->name + "'";
            return false;
        }
    }
    if (!out) return true;
    std::vector<std::unique_ptr<ASTNode>> result;
    for (size_t k = 0; k < n; ++k) {
        if (source[k] != SIZE_MAX) {
            auto* named = dynamic_cast<AssignmentExpressionNode*>(args[source[k]].get());
            if (named && named->isNamedArgument) result.push_back(std::move(named->right));
            else result.push_back(std::move(args[source[k]]));
        }
        else {
            result.push_back(sig.parameters[k]->defaultValue->clone());
        }
    }
    *out = std::move(result);
    return true;
}

void collect(std::vector<const Signature*>& out, const std::map<std::string, std::vector<Signature>>& map,
             const std::string& key) {
    auto it = map.find(key);
    if (it == map.end()) return;
    for (const auto& sig : it->second) out.push_back(&sig);
}

}  // namespace

bool checkDefaultParameters(ASTNode* program) {
    int errors = 0;
    auto error = [&](int line, const std::string& message) {
        std::cerr << "Error" << where(line) << ": " << message << std::endl;
        ++errors;
    };
    ast_walk::walk(program, nullptr, [&](ASTNode* node, std::unique_ptr<ASTNode>*) {
        std::vector<ParameterNode*> params;
        if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(node)) params = parametersOf(fn->parameters);
        else if (auto* cls = dynamic_cast<ClassDeclarationNode*>(node)) params = parametersOf(cls->constructorParams);
        else return true;
        std::set<std::string> names;
        for (auto* p : params) names.insert(p->name);
        bool seenDefault = false;
        for (auto* p : params) {
            if (!p->defaultValue) {
                if (seenDefault) {
                    error(p->line, "parameter '" + p->name + "' needs a default value: it follows a parameter that has one");
                }
                continue;
            }
            seenDefault = true;
            ast_walk::walk(p->defaultValue.get(), nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
                if (auto* id = dynamic_cast<IdentifierNode*>(n)) {
                    if (names.count(id->value)) {
                        error(p->line, "the default value of parameter '" + p->name + "' may not refer to parameter '" +
                                       id->value + "'");
                    }
                }
                return true;
            });
        }
        return true;
    });
    return errors == 0;
}

void indexCallSignatures(ASTNode* root) {
    auto& table = index();
    table = SignatureIndex();
    auto* program = dynamic_cast<ProgramNode*>(root);
    if (!program) return;
    for (auto& statement : program->statements) indexTopLevel(table, statement.get());
}

bool hasNamedArguments(const std::vector<std::unique_ptr<ASTNode>>& args) {
    for (const auto& a : args) {
        auto* named = dynamic_cast<AssignmentExpressionNode*>(a.get());
        if (named && named->isNamedArgument) return true;
    }
    return false;
}

bool hasMethodSignatures(const std::string& className, const std::string& method) {
    auto& table = index();
    std::set<std::string> seen;
    for (std::string cls = className; !cls.empty() && seen.insert(cls).second; ) {
        if (table.classMethods.count(cls + "#" + method)) return true;
        auto parent = table.superClass.find(cls);
        cls = parent == table.superClass.end() ? std::string() : parent->second;
    }
    return false;
}

namespace {
// Whether a method of `className` named `method` may hand out or store `this` (directly or through
// the methods it calls on `this`). Unknown methods may.
bool mayKeepThis(const std::string& className, const std::string& method, std::set<std::string>& visiting) {
    auto& table = index();
    std::vector<FunctionDeclarationNode*> declarations;
    std::set<std::string> seen;
    for (std::string cls = className; !cls.empty() && seen.insert(cls).second; ) {
        auto it = table.classMethods.find(cls + "#" + method);
        if (it != table.classMethods.end()) {
            for (const auto& sig : it->second) if (sig.declaration) declarations.push_back(sig.declaration);
            break;
        }
        auto parent = table.superClass.find(cls);
        cls = parent == table.superClass.end() ? std::string() : parent->second;
    }
    if (declarations.empty()) return true;
    if (!visiting.insert(className + "#" + method).second) return false;  // recursion: judged by the other uses
    for (FunctionDeclarationNode* fn : declarations) {
        bool keeps = false;
        std::function<void(ASTNode*, ASTNode*)> scan = [&](ASTNode* node, ASTNode* parent) {
            if (!node || keeps) return;
            if (auto* id = dynamic_cast<IdentifierNode*>(node)) {
                if (id->value != "this" && id->value != "super") return;
                if (auto* access = dynamic_cast<AccessFieldNode*>(parent)) { if (access->base.get() == node) return; }
                if (auto* call = dynamic_cast<MethodCallNode*>(parent)) {
                    if (call->object.get() == node) {
                        keeps = mayKeepThis(className, call->methodName, visiting);
                        return;
                    }
                }
                keeps = true;
                return;
            }
            if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
                // A method called without `this.` receives this object too.
                if (hasMethodSignatures(className, call->functionName) && mayKeepThis(className, call->functionName, visiting)) {
                    keeps = true;
                    return;
                }
            }
            if (dynamic_cast<LambdaExpressionNode*>(node)) {
                // A lambda that mentions `this` or a field could outlive the call.
                ast_walk::walk(node, nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
                    if (auto* id = dynamic_cast<IdentifierNode*>(n)) {
                        if (id->value == "this" || id->value.rfind("this.", 0) == 0) keeps = true;
                    }
                    return !keeps;
                });
                return;
            }
            ast_walk::forEachChild(node, [&](ASTNode* child, std::unique_ptr<ASTNode>*) { scan(child, node); });
        };
        scan(fn->body.get(), nullptr);
        if (keeps) return true;
    }
    return false;
}
}  // namespace

bool methodMayKeepThis(const std::string& className, const std::string& method) {
    std::set<std::string> visiting;
    return mayKeepThis(className, method, visiting);
}

bool expandCallArguments(std::vector<std::unique_ptr<ASTNode>>& args, CallTarget kind, const std::string& owner,
                         const std::string& name, int line, const CallReport& report) {
    auto& table = index();
    std::vector<const Signature*> candidates;
    std::string callee = name;
    switch (kind) {
        case CallTarget::Function:
            collect(candidates, table.functions, name);
            break;
        case CallTarget::Namespaced:
            collect(candidates, table.namespaced, owner + "." + name);
            callee = owner + "." + name;
            break;
        case CallTarget::Constructor:
            collect(candidates, table.constructors, owner);
            callee = "new " + owner;
            break;
        case CallTarget::Method: {
            // The most derived class that declares the method: its overloads override the parents'.
            std::set<std::string> seen;
            for (std::string cls = owner; !cls.empty() && seen.insert(cls).second; ) {
                collect(candidates, table.classMethods, cls + "#" + name);
                if (!candidates.empty()) break;
                auto parent = table.superClass.find(cls);
                cls = parent == table.superClass.end() ? std::string() : parent->second;
            }
            callee = owner + "." + name;
            break;
        }
        case CallTarget::None:
            break;
    }
    const bool named = hasNamedArguments(args);
    const std::string at = where(line);
    auto fail = [&](const std::string& message) { report.error("Error" + at + ": " + message); };
    if (candidates.empty()) {
        if (named) {
            fail("named arguments need a function whose parameters are known; '" + callee +
                 "' is not a function, method or constructor declared in this program");
            return false;
        }
        return true;
    }
    if (!named) {
        for (const auto* sig : candidates) {
            if (sig->parameters.size() == args.size()) return true;  // already a positional call
        }
    }
    std::vector<const Signature*> viable;
    std::string firstWhy;
    for (const auto* sig : candidates) {
        std::string why;
        if (bindArguments(args, *sig, nullptr, &why)) viable.push_back(sig);
        else if (firstWhy.empty()) firstWhy = sig->display + " " + why;
    }
    if (viable.empty()) {
        if (named) {
            fail("call to '" + callee + "': " + firstWhy);
            return false;
        }
        return true;  // a plain positional mismatch is reported by the usual checks
    }
    if (viable.size() > 1) {
        // Overloads of one declaration that all accept the call must agree on everything they fill in.
        const Signature* first = viable.front();
        for (const auto* other : viable) {
            bool same = other->parameters.size() == first->parameters.size();
            for (size_t k = 0; same && k < first->parameters.size(); ++k) {
                same = other->parameters[k]->name == first->parameters[k]->name &&
                       (other->parameters[k]->defaultValue == nullptr) == (first->parameters[k]->defaultValue == nullptr);
            }
            if (!same) {
                fail("call to '" + callee + "' is ambiguous: it matches '" + first->display + "' and '" +
                             other->display + "'; pass every argument positionally");
                return false;
            }
        }
    }
    const Signature* chosen = viable.front();
    // A default is evaluated at the call site: a name in it must mean the same there as at the
    // declaration, i.e. not be shadowed by a local of the caller.
    for (size_t k = 0; k < chosen->parameters.size(); ++k) {
        ParameterNode* param = chosen->parameters[k];
        if (!param->defaultValue) continue;
        bool supplied = false;
        size_t positional = 0;
        for (auto& a : args) {
            auto* n = dynamic_cast<AssignmentExpressionNode*>(a.get());
            if (n && n->isNamedArgument) { if (receiverName(n->left.get()) == param->name) supplied = true; }
            else if (positional++ == k) supplied = true;
        }
        if (supplied) continue;
        bool shadowed = false;
        std::string shadowedName;
        ast_walk::walk(param->defaultValue.get(), nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
            if (auto* id = dynamic_cast<IdentifierNode*>(n)) {
                const std::string base = id->value.substr(0, id->value.find('.'));
                if (!shadowed && report.isShadowed(base)) { shadowed = true; shadowedName = base; }
            }
            return true;
        });
        if (shadowed) {
            fail("the default value of parameter '" + param->name + "' of '" + chosen->display + "' refers to '" +
                         shadowedName + "', which is a different variable at this call; pass the argument");
            return false;
        }
    }
    std::vector<std::unique_ptr<ASTNode>> rewritten;
    bindArguments(args, *chosen, &rewritten, nullptr);
    args = std::move(rewritten);
    return true;
}
