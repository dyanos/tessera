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
#include "interfaces.h"
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
    std::string ownerClass;                  // the class (or interface) declaring a method
};

struct SignatureIndex {
    std::map<std::string, std::vector<Signature>> functions;     // top-level (and template) `def f`
    std::map<std::string, std::vector<Signature>> namespaced;    // `Obj.f` (object members)
    std::map<std::string, std::vector<Signature>> constructors;  // `new C(...)`
    std::map<std::string, std::vector<Signature>> classMethods;  // `C#m` (also `Interface#m`)
    std::map<std::string, std::string> superClass;               // class -> parent
    std::map<std::string, std::set<std::string>> fields;         // class -> its constructor parameters and fields
    std::map<std::string, bool> keepsThis;                       // methodMayKeepThis results, by `C#m`
    std::map<std::string, std::vector<std::string>> methodClasses;  // method name -> classes declaring it
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
    for (auto* p : parametersOf(cls.constructorParams)) table.fields[cls.name].insert(p->name);
    auto* body = dynamic_cast<ClassBodyNode*>(cls.body.get());
    if (!body) return;
    for (auto& fieldNode : body->fields) {
        if (auto* field = dynamic_cast<VariableDeclarationNode*>(fieldNode.get())) table.fields[cls.name].insert(field->name);
    }
    for (auto& m : body->methods) {
        auto* method = dynamic_cast<FunctionDeclarationNode*>(m.get());
        if (!method) continue;
        table.classMethods[cls.name + "#" + method->name].push_back(
            {cls.name + "." + method->name, parametersOf(method->parameters), method, cls.name});
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
    // Calls through an interface value use the interface's declaration of the method.
    forEachInterfaceMethod([&](const std::string& interfaceName, FunctionDeclarationNode* method) {
        table.classMethods[interfaceName + "#" + method->name].push_back(
            {interfaceName + "." + method->name, parametersOf(method->parameters), method, interfaceName});
    });
    for (const auto& entry : table.classMethods) {
        const auto hash = entry.first.find('#');
        table.methodClasses[entry.first.substr(hash + 1)].push_back(entry.first.substr(0, hash));
    }
}

namespace {
// Calls visit(cls) for `className` and then each parent, until visit returns true (a cycle in the
// declared parents ends the walk). Returns whether some visit returned true.
template <class Visit>
bool forEachAncestor(const std::string& className, Visit visit) {
    auto& table = index();
    std::set<std::string> seen;
    for (std::string cls = className; !cls.empty() && seen.insert(cls).second; ) {
        if (visit(cls)) return true;
        auto parent = table.superClass.find(cls);
        cls = parent == table.superClass.end() ? std::string() : parent->second;
    }
    return false;
}

// Whether `name` is a constructor parameter or field of `className` or a parent.
bool isFieldOf(const std::string& className, const std::string& name) {
    return forEachAncestor(className, [&](const std::string& cls) {
        auto it = index().fields.find(cls);
        return it != index().fields.end() && it->second.count(name) > 0;
    });
}

// Whether `cls` is `ancestor` or derives from it.
bool derivesFrom(const std::string& cls, const std::string& ancestor) {
    return forEachAncestor(cls, [&](const std::string& c) { return c == ancestor; });
}
}  // namespace

bool hasNamedArguments(const std::vector<std::unique_ptr<ASTNode>>& args) {
    for (const auto& a : args) {
        auto* named = dynamic_cast<AssignmentExpressionNode*>(a.get());
        if (named && named->isNamedArgument) return true;
    }
    return false;
}

bool hasMethodSignatures(const std::string& className, const std::string& method) {
    return forEachAncestor(className, [&](const std::string& cls) {
        return index().classMethods.count(cls + "#" + method) > 0;
    });
}

namespace {
// Whether a method of `className` named `method` may hand out or store `this` (directly or through
// the methods it calls on `this`). Unknown methods may.
bool mayKeepThis(const std::string& className, const std::string& method, std::set<std::string>& visiting) {
    auto& table = index();
    std::vector<const Signature*> declarations;  // each with the class that declares it
    forEachAncestor(className, [&](const std::string& cls) {
        auto it = table.classMethods.find(cls + "#" + method);
        if (it == table.classMethods.end()) return false;
        for (const auto& sig : it->second) if (sig.declaration) declarations.push_back(&sig);
        return true;
    });
    if (declarations.empty()) return true;
    // An override in a subclass runs when the object is of that subclass.
    auto classes = table.methodClasses.find(method);
    if (classes != table.methodClasses.end()) {
        for (const auto& cls : classes->second) {
            if (cls == className || !derivesFrom(cls, className)) continue;
            for (const auto& sig : table.classMethods[cls + "#" + method]) if (sig.declaration) declarations.push_back(&sig);
        }
    }
    // A declaration without a body (an interface method) runs some implementation not seen here.
    for (const Signature* declared : declarations) {
        if (!declared->declaration->body) return true;
    }
    if (!visiting.insert(className + "#" + method).second) return false;  // recursion: judged by the other uses
    for (const Signature* declared : declarations) {
        FunctionDeclarationNode* fn = declared->declaration;
        const std::string& declaringClass = declared->ownerClass.empty() ? className : declared->ownerClass;
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
                if (hasMethodSignatures(declaringClass, call->functionName) &&
                    mayKeepThis(className, call->functionName, visiting)) {
                    keeps = true;
                    return;
                }
            }
            if (dynamic_cast<LambdaExpressionNode*>(node)) {
                // A lambda that mentions `this` or a field (by its bare name too) could outlive the call.
                ast_walk::walk(node, nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
                    if (auto* id = dynamic_cast<IdentifierNode*>(n)) {
                        const std::string base = id->value.substr(0, id->value.find('.'));
                        if (base == "this" || base == "super" || isFieldOf(declaringClass, base)) keeps = true;
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
    auto& cache = index().keepsThis;
    const std::string key = className + "#" + method;
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    std::set<std::string> visiting;
    const bool keeps = mayKeepThis(className, method, visiting);
    cache[key] = keeps;
    return keeps;
}

namespace {
// Calls visit(slot, id) for each identifier of `slot` in an expression position. Member names
// (`x.field`) and argument names (`g(field = 1)`) are not expressions and are skipped.
template <class Visit>
void forEachExpressionName(std::unique_ptr<ASTNode>& slot, Visit& visit) {
    if (!slot) return;
    if (auto* id = dynamic_cast<IdentifierNode*>(slot.get())) {
        visit(slot, *id);
        return;
    }
    if (auto* access = dynamic_cast<AccessFieldNode*>(slot.get())) {
        forEachExpressionName(access->base, visit);  // not the member name
        return;
    }
    if (auto* named = dynamic_cast<AssignmentExpressionNode*>(slot.get())) {
        if (named->isNamedArgument) {
            forEachExpressionName(named->right, visit);  // not the parameter name
            return;
        }
    }
    if (auto* method = dynamic_cast<MethodCallNode*>(slot.get())) {
        forEachExpressionName(method->object, visit);
        for (auto& a : method->arguments) forEachExpressionName(a, visit);
        return;
    }
    ast_walk::forEachChild(slot.get(), [&](ASTNode*, std::unique_ptr<ASTNode>* child) {
        if (child) forEachExpressionName(*child, visit);
    });
}

// `field` and `field.member` in expression positions of `slot` become reads through `receiver`.
void rewriteFieldReads(std::unique_ptr<ASTNode>& slot, const std::string& fieldOwner, const std::string& receiver) {
    auto rewrite = [&](std::unique_ptr<ASTNode>& at, IdentifierNode& id) {
        const auto dot = id.value.find('.');
        const std::string head = id.value.substr(0, dot);
        if (head == "this" || head == "super" || !isFieldOf(fieldOwner, head)) return;
        std::unique_ptr<ASTNode> chain = std::make_unique<AccessFieldNode>(std::make_unique<IdentifierNode>(receiver),
                                                                           std::make_unique<IdentifierNode>(head));
        size_t start = dot;
        while (start != std::string::npos) {
            const size_t next = id.value.find('.', start + 1);
            const std::string member = id.value.substr(start + 1, next == std::string::npos ? std::string::npos : next - start - 1);
            chain = std::make_unique<AccessFieldNode>(std::move(chain), std::make_unique<IdentifierNode>(member));
            start = next;
        }
        at = std::move(chain);  // destroys `id`; it is not used after this
    };
    forEachExpressionName(slot, rewrite);
}
}  // namespace

bool expandCallArguments(std::vector<std::unique_ptr<ASTNode>>& args, CallTarget kind, const std::string& owner,
                         const std::string& name, int line, const CallReport& report, ASTNode* receiver) {
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
            forEachAncestor(owner, [&](const std::string& cls) {
                collect(candidates, table.classMethods, cls + "#" + name);
                return !candidates.empty();
            });
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
    // The class whose fields a method default may name (the class declaring the method): they are
    // read from the receiver.
    const std::string fieldOwner = kind == CallTarget::Method ? chosen->ownerClass : std::string();
    const std::string receiverText = receiverName(receiver);
    std::vector<size_t> defaulted;
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
        defaulted.push_back(k);
        bool shadowed = false;
        bool needsReceiver = false;
        std::string shadowedName;
        // The same names rewriteFieldReads rewrites: not member or argument names.
        auto check = [&](std::unique_ptr<ASTNode>&, IdentifierNode& id) {
            const std::string base = id.value.substr(0, id.value.find('.'));
            if (!fieldOwner.empty() && isFieldOf(fieldOwner, base)) {
                // A field of the receiver: `this` inside the class, `r.field` for a named receiver.
                if (receiver && receiverText.empty()) { needsReceiver = true; shadowedName = base; }
                return;
            }
            if (!shadowed && report.isShadowed(base)) { shadowed = true; shadowedName = base; }
        };
        forEachExpressionName(param->defaultValue, check);
        if (needsReceiver) {
            fail("the default value of parameter '" + param->name + "' of '" + chosen->display + "' reads the field '" +
                 shadowedName + "'; call the method on a named value or pass the argument");
            return false;
        }
        if (shadowed) {
            fail("the default value of parameter '" + param->name + "' of '" + chosen->display + "' refers to '" +
                         shadowedName + "', which is a different variable at this call; pass the argument");
            return false;
        }
    }
    std::vector<std::unique_ptr<ASTNode>> rewritten;
    bindArguments(args, *chosen, &rewritten, nullptr);
    if (!fieldOwner.empty()) {
        // A field named in a copied default is read from the receiver: `r.field` for a named one,
        // `this.field` for `this`/`super` or a call without a receiver (so no local of the calling
        // method can stand in for it).
        const bool named = !receiverText.empty() && receiverText != "this" && receiverText != "super";
        const std::string base = named ? receiverText : std::string("this");
        for (size_t k : defaulted) rewriteFieldReads(rewritten[k], fieldOwner, base);
    }
    args = std::move(rewritten);
    return true;
}

namespace {
// The names a template body declares, and for those whose every declaration names one concrete
// class (`val c: Cache = ...`, `val c = new Cache(...)`, a parameter `c: Cache`) that class: a
// method call on them has a certain target although the body is not type checked.
struct BodyNames {
    std::set<std::string> all;
    std::map<std::string, std::string> classOf;
};

// Template bodies are not type checked, so their calls are matched syntactically: functions and
// constructors by name (unless a parameter or local of the body has that name), methods of
// `this`/`super` or of a local of a known class, and object functions.
void expandInBody(ASTNode* body, int startLine, const std::string& enclosingClass, const BodyNames& names,
                  const CallReport& report, int& errors) {
    const std::set<std::string>& locals = names.all;
    CallReport local = report;
    local.isShadowed = [&locals](const std::string& name) { return locals.count(name) > 0; };
    local.error = [&](const std::string& message) { report.error(message); ++errors; };
    auto& table = index();
    int line = startLine;
    ast_walk::walk(body, nullptr, [&](ASTNode* node, std::unique_ptr<ASTNode>*) {
        if (node->line > 0) line = node->line;
        auto unresolved = [&](const std::vector<std::unique_ptr<ASTNode>>& args, const std::string& callee) {
            if (hasNamedArguments(args)) {
                local.error("Error" + where(line) + ": named arguments in a template body need a function, a "
                            "constructor, an object function, a method of this/super or of a local whose class "
                            "is declared; '" + callee + "' is none");
            }
        };
        if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
            if (locals.count(call->functionName)) {
                unresolved(call->arguments, call->functionName);
            }
            // A bare call names a global function before a method of the class (as code generation
            // resolves it); a method is the target only when no function has the name.
            else if (table.functions.count(call->functionName)) {
                expandCallArguments(call->arguments, CallTarget::Function, "", call->functionName, line, local);
            }
            else if (!enclosingClass.empty() && hasMethodSignatures(enclosingClass, call->functionName)) {
                expandCallArguments(call->arguments, CallTarget::Method, enclosingClass, call->functionName, line, local);
            }
            else {
                unresolved(call->arguments, call->functionName);
            }
        }
        else if (auto* method = dynamic_cast<MethodCallNode*>(node)) {
            const std::string object = receiverName(method->object.get());
            auto known = names.classOf.find(object);
            if ((object == "this" || object == "super") && !enclosingClass.empty()) {
                std::string cls = enclosingClass;
                if (object == "super") {
                    auto parent = table.superClass.find(cls);
                    cls = parent == table.superClass.end() ? std::string() : parent->second;
                }
                if (!cls.empty()) {
                    expandCallArguments(method->arguments, CallTarget::Method, cls, method->methodName, line, local,
                                        method->object.get());
                }
            }
            else if (known != names.classOf.end() && hasMethodSignatures(known->second, method->methodName)) {
                expandCallArguments(method->arguments, CallTarget::Method, known->second, method->methodName, line,
                                    local, method->object.get());
            }
            else if (!object.empty() && !locals.count(object) && table.namespaced.count(object + "." + method->methodName)) {
                expandCallArguments(method->arguments, CallTarget::Namespaced, object, method->methodName, line, local);
            }
            else {
                // The receiver's type is not known in a template body.
                unresolved(method->arguments, method->methodName);
            }
        }
        else if (auto* creation = dynamic_cast<ClassInstanceCreationNode*>(node)) {
            expandCallArguments(creation->arguments, CallTarget::Constructor, creation->className, "", line, local);
        }
        return true;
    });
}

// `notConcrete`: the template's type parameters and the program's template classes, whose
// members depend on the instance.
BodyNames namesDeclaredIn(FunctionDeclarationNode& fn, const std::set<std::string>& notConcrete) {
    auto& table = index();
    BodyNames names;
    std::set<std::string> unknown;
    auto declare = [&](const std::string& name, const std::string& cls) {
        names.all.insert(name);
        const bool concrete = !cls.empty() && !notConcrete.count(cls) &&
                              (table.constructors.count(cls) > 0 || findInterface(cls) != nullptr);
        if (!concrete) { unknown.insert(name); return; }
        auto entry = names.classOf.emplace(name, cls);
        if (!entry.second && entry.first->second != cls) unknown.insert(name);
    };
    auto typeName = [](const std::unique_ptr<Type>& type) { return type ? type->getName() : std::string(); };
    for (auto* p : parametersOf(fn.parameters)) declare(p->name, typeName(p->type));
    ast_walk::walk(fn.body.get(), nullptr, [&](ASTNode* n, std::unique_ptr<ASTNode>*) {
        if (auto* decl = dynamic_cast<VariableDeclarationNode*>(n)) {
            std::string cls;
            if (decl->hasExplicitType) cls = typeName(decl->type);
            else if (auto* creation = dynamic_cast<ClassInstanceCreationNode*>(decl->initializer.get())) {
                if (creation->templateArgs.empty()) cls = creation->className;
            }
            declare(decl->name, cls);
        }
        else if (auto* lambda = dynamic_cast<LambdaExpressionNode*>(n)) {
            for (auto* p : parametersOf(lambda->arguments)) declare(p->name, "");
        }
        else if (auto* loop = dynamic_cast<ForStatementNode*>(n)) {
            declare(loop->variable, "");
            if (!loop->tokenValue.empty()) declare(loop->tokenValue, "");
        }
        return true;
    });
    for (const auto& name : unknown) names.classOf.erase(name);
    return names;
}
}  // namespace

bool expandTemplateBodies(ASTNode* root, const CallReport& report) {
    auto* program = dynamic_cast<ProgramNode*>(root);
    if (!program) return true;
    std::set<std::string> templateClasses;
    for (auto& statement : program->statements) {
        if (auto* tmpl = dynamic_cast<TemplateDeclarationNode*>(statement.get())) {
            if (auto* cls = dynamic_cast<ClassDeclarationNode*>(tmpl->declaration.get())) templateClasses.insert(cls->name);
        }
    }
    int errors = 0;
    for (auto& statement : program->statements) {
        auto* tmpl = dynamic_cast<TemplateDeclarationNode*>(statement.get());
        if (!tmpl) continue;
        std::set<std::string> notConcrete = templateClasses;
        notConcrete.insert(tmpl->typeParameters.begin(), tmpl->typeParameters.end());
        for (const auto& p : tmpl->templateParams) notConcrete.insert(p.name);
        if (auto* fn = dynamic_cast<FunctionDeclarationNode*>(tmpl->declaration.get())) {
            expandInBody(fn->body.get(), fn->line, "", namesDeclaredIn(*fn, notConcrete), report, errors);
        }
        else if (auto* cls = dynamic_cast<ClassDeclarationNode*>(tmpl->declaration.get())) {
            auto* body = dynamic_cast<ClassBodyNode*>(cls->body.get());
            if (!body) continue;
            for (auto& m : body->methods) {
                if (auto* method = dynamic_cast<FunctionDeclarationNode*>(m.get())) {
                    BodyNames names = namesDeclaredIn(*method, notConcrete);
                    // The class's fields are in scope too: they shadow a default's global names, and a
                    // local of the same name does not give them a known class.
                    auto addField = [&](const std::string& name) {
                        names.all.insert(name);
                        names.classOf.erase(name);
                    };
                    for (auto* p : parametersOf(cls->constructorParams)) addField(p->name);
                    for (auto& fieldNode : body->fields) {
                        if (auto* field = dynamic_cast<VariableDeclarationNode*>(fieldNode.get())) addField(field->name);
                    }
                    expandInBody(method->body.get(), method->line, cls->name, names, report, errors);
                }
            }
        }
    }
    return errors == 0;
}
