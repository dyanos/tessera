// default_arguments.cpp -- default parameter values and named call arguments.
//
//   def f(x: Int, scale: Double = 1.0e0, offset: Int = 0): Double
//   f(3);                  // f(3, 1.0e0, 0)
//   f(3, offset = 2);      // f(3, 1.0e0, 2)
//
// Runs on the parsed program before type checking. Every call that omits trailing arguments or
// names an argument is rewritten into an ordinary positional call: named arguments move to their
// parameter's position and each missing argument becomes a copy of the parameter's default
// expression, evaluated at the call site. Calls are matched by name -- top-level functions,
// object (namespace) functions, constructors (`new C(...)`), and methods -- so the overload that
// the rewritten call finally binds to is still chosen by the type checker. A call whose arguments
// already fill one of the candidate signatures is left untouched.
#include "default_arguments.h"

#include "ast_walk.h"
#include "source_map.h"

#include <iostream>
#include <map>
#include <set>
#include <sstream>

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
};

struct SignatureIndex {
    std::map<std::string, std::vector<Signature>> functions;     // top-level `def f`
    std::map<std::string, std::vector<Signature>> namespaced;    // `Obj.f` (object members)
    std::map<std::string, std::vector<Signature>> constructors;  // `new C(...)`
    std::map<std::string, std::vector<Signature>> classMethods;  // `C#m`
    std::map<std::string, std::vector<Signature>> anyMethod;     // `m` on any class (receiver type unknown here)
    std::map<std::string, std::string> superClass;               // class -> parent
    std::set<std::string> objects;
};

std::vector<ParameterNode*> parametersOf(std::vector<std::unique_ptr<ASTNode>>& nodes) {
    std::vector<ParameterNode*> result;
    for (auto& node : nodes) {
        if (auto* p = dynamic_cast<ParameterNode*>(node.get())) result.push_back(p);
    }
    return result;
}

void indexClass(SignatureIndex& index, ClassDeclarationNode& cls) {
    index.constructors[cls.name].push_back({"new " + cls.name, parametersOf(cls.constructorParams)});
    if (!cls.superClassName.empty()) index.superClass[cls.name] = cls.superClassName;
    auto* body = dynamic_cast<ClassBodyNode*>(cls.body.get());
    if (!body) return;
    for (auto& m : body->methods) {
        auto* method = dynamic_cast<FunctionDeclarationNode*>(m.get());
        if (!method) continue;
        Signature sig{cls.name + "." + method->name, parametersOf(method->parameters)};
        index.classMethods[cls.name + "#" + method->name].push_back(sig);
        index.anyMethod[method->name].push_back(sig);
    }
}

void indexObject(SignatureIndex& index, ObjectDeclarationNode& object) {
    index.objects.insert(object.name);
    auto* body = dynamic_cast<BlockNode*>(object.body.get());
    if (!body) return;
    for (auto& statement : body->statements) {
        if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(statement.get())) {
            index.namespaced[object.name + "." + fn->name].push_back(
                {object.name + "." + fn->name, parametersOf(fn->parameters)});
        }
    }
}

void indexTopLevel(SignatureIndex& index, ASTNode* statement) {
    if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(statement)) {
        index.functions[fn->name].push_back({fn->name, parametersOf(fn->parameters)});
    }
    else if (auto* cls = dynamic_cast<ClassDeclarationNode*>(statement)) {
        indexClass(index, *cls);
    }
    else if (auto* object = dynamic_cast<ObjectDeclarationNode*>(statement)) {
        indexObject(index, *object);
    }
    else if (auto* tmpl = dynamic_cast<TemplateDeclarationNode*>(statement)) {
        indexTopLevel(index, tmpl->declaration.get());
    }
    else if (auto* attribute = dynamic_cast<AttributeNode*>(statement)) {
        indexTopLevel(index, attribute->target.get());
    }
}

class Expander {
public:
    explicit Expander(SignatureIndex& index) : index(index) {}
    int errors = 0;

    void run(ProgramNode& program) {
        checkDefaults(program);
        for (auto& statement : program.statements) visitTop(statement.get());
    }

private:
    SignatureIndex& index;
    std::vector<std::string> classStack;  // enclosing class names (for unqualified sibling calls)
    int currentLine = 0;                  // line of the innermost enclosing node that has one

    void error(int line, const std::string& message) {
        std::cerr << "Error" << where(line) << ": " << message << std::endl;
        ++errors;
    }

    // A default may not refer to the function's other parameters: it is evaluated at the call site,
    // where those names mean something else (or nothing).
    void checkDefaults(ProgramNode& program) {
        ast_walk::walk(&program, nullptr, [&](ASTNode* node, std::unique_ptr<ASTNode>*) {
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
                        error(p->line, "parameter '" + p->name +
                              "' needs a default value: it follows a parameter that has one");
                    }
                    continue;
                }
                seenDefault = true;
                ast_walk::walk(p->defaultValue.get(), nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
                    if (auto* id = dynamic_cast<IdentifierNode*>(n)) {
                        if (names.count(id->value)) {
                            error(p->line, "the default value of parameter '" + p->name +
                                  "' may not refer to parameter '" + id->value + "'");
                        }
                    }
                    return true;
                });
            }
            return true;
        });
    }

    void visitTop(ASTNode* node) {
        if (auto* cls = dynamic_cast<ClassDeclarationNode*>(node)) {
            classStack.push_back(cls->name);
            visit(node, nullptr);
            classStack.pop_back();
            return;
        }
        if (auto* tmpl = dynamic_cast<TemplateDeclarationNode*>(node)) {
            visitTop(tmpl->declaration.get());
            return;
        }
        visit(node, nullptr);
    }

    void visit(ASTNode* node, std::unique_ptr<ASTNode>* slot) {
        if (!node) return;
        if (auto* cls = dynamic_cast<ClassDeclarationNode*>(node)) {
            classStack.push_back(cls->name);
            ast_walk::forEachChild(node, [&](ASTNode* c, std::unique_ptr<ASTNode>* s) { visit(c, s); });
            classStack.pop_back();
            return;
        }
        const int savedLine = currentLine;
        if (node->line > 0) currentLine = node->line;
        struct Restore { int& line; int saved; ~Restore() { line = saved; } } restore{currentLine, savedLine};
        // Children first: arguments may themselves be calls to expand.
        ast_walk::forEachChild(node, [&](ASTNode* c, std::unique_ptr<ASTNode>* s) { visit(c, s); });

        if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
            std::vector<const Signature*> candidates;
            add(candidates, index.functions, call->functionName);
            for (auto it = classStack.rbegin(); it != classStack.rend(); ++it) {
                addClassChain(candidates, *it, call->functionName);
            }
            expand(call->arguments, candidates, call->functionName, call->line);
        }
        else if (auto* method = dynamic_cast<MethodCallNode*>(node)) {
            std::vector<const Signature*> candidates;
            const std::string receiver = receiverName(method->object.get());
            if (!receiver.empty() && index.objects.count(receiver)) {
                add(candidates, index.namespaced, receiver + "." + method->methodName);
            }
            else if ((receiver == "this" || receiver == "super") && !classStack.empty()) {
                std::string cls = classStack.back();
                if (receiver == "super") {
                    auto parent = index.superClass.find(cls);
                    cls = parent == index.superClass.end() ? std::string() : parent->second;
                }
                addClassChain(candidates, cls, method->methodName);
            }
            else {
                add(candidates, index.anyMethod, method->methodName);
            }
            expand(method->arguments, candidates, method->methodName, method->line);
        }
        else if (auto* creation = dynamic_cast<ClassInstanceCreationNode*>(node)) {
            std::vector<const Signature*> candidates;
            add(candidates, index.constructors, creation->className);
            expand(creation->arguments, candidates, "new " + creation->className, creation->line);
        }
        (void)slot;
    }

    static std::string receiverName(ASTNode* object) {
        if (auto* id = dynamic_cast<IdentifierNode*>(object)) return id->value;
        return "";
    }

    static void add(std::vector<const Signature*>& out, const std::map<std::string, std::vector<Signature>>& map,
                    const std::string& key) {
        auto it = map.find(key);
        if (it == map.end()) return;
        for (const auto& sig : it->second) out.push_back(&sig);
    }

    void addClassChain(std::vector<const Signature*>& out, std::string cls, const std::string& method) {
        std::set<std::string> seen;
        while (!cls.empty() && seen.insert(cls).second) {
            add(out, index.classMethods, cls + "#" + method);
            auto parent = index.superClass.find(cls);
            cls = parent == index.superClass.end() ? std::string() : parent->second;
        }
    }

    // Maps the call's arguments onto `sig`. Returns false (with `why`) when they do not fit.
    static bool bind(std::vector<std::unique_ptr<ASTNode>>& args, const Signature& sig,
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
                auto copy = sig.parameters[k]->defaultValue->clone();
                result.push_back(std::move(copy));
            }
        }
        *out = std::move(result);
        return true;
    }

    void expand(std::vector<std::unique_ptr<ASTNode>>& args, const std::vector<const Signature*>& candidates,
                const std::string& callee, int line) {
        if (line <= 0) line = currentLine;
        bool hasNamed = false;
        for (auto& a : args) {
            auto* named = dynamic_cast<AssignmentExpressionNode*>(a.get());
            if (named && named->isNamedArgument) hasNamed = true;
        }
        if (candidates.empty()) {
            if (hasNamed) error(line, "named arguments need a function whose parameters are known; '" + callee +
                                      "' is not a function, method or constructor declared in this program");
            return;
        }
        // Already a complete positional call for some overload: nothing to do.
        if (!hasNamed) {
            for (const auto* sig : candidates) {
                if (sig->parameters.size() == args.size()) return;
            }
        }
        std::vector<const Signature*> viable;
        std::string firstWhy;
        for (const auto* sig : candidates) {
            std::string why;
            if (bind(args, *sig, nullptr, &why)) viable.push_back(sig);
            else if (firstWhy.empty()) firstWhy = sig->display + " " + why;
        }
        if (viable.empty()) {
            // A plain positional call that matches nothing is left to the type checker's diagnostics.
            if (hasNamed) error(line, "call to '" + callee + "': " + firstWhy);
            return;
        }
        // Several overloads accept the call: they must agree on parameter names and defaults
        // (e.g. the same method declared in several classes), otherwise the call is ambiguous.
        const Signature* chosen = viable.front();
        for (const auto* other : viable) {
            if (other->parameters.size() != chosen->parameters.size()) {
                error(line, "call to '" + callee + "' is ambiguous between overloads with default or named "
                            "arguments; pass every argument positionally");
                return;
            }
            for (size_t k = 0; k < chosen->parameters.size(); ++k) {
                if (other->parameters[k]->name != chosen->parameters[k]->name ||
                    (other->parameters[k]->defaultValue == nullptr) != (chosen->parameters[k]->defaultValue == nullptr)) {
                    error(line, "call to '" + callee + "' is ambiguous: it matches '" + chosen->display +
                                "' and '" + other->display + "'");
                    return;
                }
            }
        }
        std::vector<std::unique_ptr<ASTNode>> rewritten;
        bind(args, *chosen, &rewritten, nullptr);
        args = std::move(rewritten);
    }
};

}  // namespace

bool expandDefaultAndNamedArguments(ASTNode* root) {
    auto* program = dynamic_cast<ProgramNode*>(root);
    if (!program) return true;
    SignatureIndex index;
    for (auto& statement : program->statements) indexTopLevel(index, statement.get());
    Expander expander(index);
    expander.run(*program);
    if (expander.errors) return false;
    // Any named argument left over is in a position the pass could not resolve.
    ast_walk::walk(program, nullptr, [&](ASTNode* node, std::unique_ptr<ASTNode>*) {
        auto* named = dynamic_cast<AssignmentExpressionNode*>(node);
        if (named && named->isNamedArgument) {
            std::cerr << "Error" << where(named->line) << ": named argument used where no parameter names are known" << std::endl;
            ++expander.errors;
            return false;
        }
        return true;
    });
    return expander.errors == 0;
}
