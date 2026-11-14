// std.logic: formulas, decision with Z3, and an LCF-style proof kernel.
//
// Formulas and integer terms are hash-consed nodes behind Long handles, so two handles are equal
// exactly when the formulas are syntactically equal. Validity and satisfiability are decided by the
// Z3 SMT solver, loaded at run time (libz3 next to the program, on the library path, or named by the
// CSEC_Z3_LIBRARY environment variable), so programs that do not ask the solver do not need it.
//
// Theorems are created only by the kernel rules below (natural deduction for the connectives and
// quantifiers, equality, and `decide`, which asks Z3 and marks the theorem as depending on that
// oracle). A theorem handle can only name a theorem the kernel produced; there is no way to build a
// theorem with an arbitrary conclusion. Misuse prints "LogicError: ..." and exits with status 1.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace csec_logic {

enum class Kind {
    True, False, Prop, Not, And, Or, Implies, Iff, Forall, Exists, Cmp,  // formulas
    IntVar, Num, Add, Sub, Mul, Neg,                                    // integer terms
};

enum Cmp { Eq = 0, Ne = 1, Lt = 2, Le = 3, Gt = 4, Ge = 5 };

struct Node {
    Kind kind;
    std::string name;      // Prop / IntVar name, bound variable of a quantifier
    long long value = 0;   // Num value, Cmp operator
    std::vector<long long> kids;
};

[[noreturn]] void fail(const std::string& message) {
    std::fflush(stdout);
    std::fprintf(stderr, "LogicError: %s\n", message.c_str());
    std::fflush(stderr);
    std::exit(1);
}

std::recursive_mutex& mutex() {
    static std::recursive_mutex m;
    return m;
}

std::vector<Node>& nodes() {
    static auto* table = new std::vector<Node>();
    return *table;
}

std::unordered_map<std::string, long long>& interned() {
    static auto* table = new std::unordered_map<std::string, long long>();
    return *table;
}

const Node& node(long long h) {
    if (h < 1 || static_cast<size_t>(h) > nodes().size()) fail("invalid formula or term handle");
    return nodes()[static_cast<size_t>(h - 1)];
}

bool isTerm(Kind k) { return k == Kind::IntVar || k == Kind::Num || k == Kind::Add || k == Kind::Sub || k == Kind::Mul || k == Kind::Neg; }

long long make(Kind kind, const std::string& name, long long value, std::vector<long long> kids) {
    std::lock_guard<std::recursive_mutex> lock(mutex());
    std::string key = std::to_string(static_cast<int>(kind)) + "|" + name + "|" + std::to_string(value);
    for (long long k : kids) key += "|" + std::to_string(k);
    auto it = interned().find(key);
    if (it != interned().end()) return it->second;
    nodes().push_back(Node{kind, name, value, std::move(kids)});
    const long long h = static_cast<long long>(nodes().size());
    interned()[key] = h;
    return h;
}

long long formula(long long h, const char* what) {
    if (isTerm(node(h).kind)) fail(std::string(what) + " needs a formula, got the term " + std::to_string(h));
    return h;
}

long long term(long long h, const char* what) {
    if (!isTerm(node(h).kind)) fail(std::string(what) + " needs an integer term, got a formula");
    return h;
}

// ---- printing ----

int precedence(const Node& n) {
    switch (n.kind) {
        case Kind::Iff: return 1;
        case Kind::Implies: return 2;
        case Kind::Or: return 3;
        case Kind::And: return 4;
        case Kind::Not: return 5;
        case Kind::Cmp: return 6;
        case Kind::Add: case Kind::Sub: return 7;
        case Kind::Mul: return 8;
        case Kind::Neg: return 9;
        case Kind::Forall: case Kind::Exists: return 0;
        default: return 10;
    }
}

std::string text(long long h);

std::string child(long long h, int parent, bool parenEqual) {
    const int own = precedence(node(h));
    const std::string t = text(h);
    return own < parent || (parenEqual && own == parent) ? "(" + t + ")" : t;
}

std::string text(long long h) {
    const Node& n = node(h);
    static const char* cmpText[] = {"==", "!=", "<", "<=", ">", ">="};
    switch (n.kind) {
        case Kind::True: return "true";
        case Kind::False: return "false";
        case Kind::Prop: case Kind::IntVar: return n.name;
        case Kind::Num: return std::to_string(n.value);
        case Kind::Not: return "!" + child(n.kids[0], 6, false);
        case Kind::And: return child(n.kids[0], 4, false) + " & " + child(n.kids[1], 4, false);
        case Kind::Or: return child(n.kids[0], 3, false) + " | " + child(n.kids[1], 3, false);
        case Kind::Implies: return child(n.kids[0], 2, true) + " -> " + child(n.kids[1], 2, false);
        case Kind::Iff: return child(n.kids[0], 1, true) + " <-> " + child(n.kids[1], 1, true);
        case Kind::Forall: return "forall " + n.name + ". " + text(n.kids[0]);
        case Kind::Exists: return "exists " + n.name + ". " + text(n.kids[0]);
        case Kind::Cmp: return child(n.kids[0], 7, false) + " " + cmpText[n.value] + " " + child(n.kids[1], 7, false);
        case Kind::Add: return child(n.kids[0], 7, false) + " + " + child(n.kids[1], 7, true);
        case Kind::Sub: return child(n.kids[0], 7, false) + " - " + child(n.kids[1], 7, true);
        case Kind::Mul: return child(n.kids[0], 8, false) + " * " + child(n.kids[1], 8, true);
        case Kind::Neg: return "-" + child(n.kids[0], 9, true);
    }
    return "?";
}

// ---- free variables and substitution ----

void freeVars(long long h, std::set<std::string>& out, std::set<std::string>& bound) {
    const Node& n = node(h);
    if (n.kind == Kind::IntVar) {
        if (!bound.count(n.name)) out.insert(n.name);
        return;
    }
    if (n.kind == Kind::Forall || n.kind == Kind::Exists) {
        const bool wasBound = bound.count(n.name) > 0;
        bound.insert(n.name);
        freeVars(n.kids[0], out, bound);
        if (!wasBound) bound.erase(n.name);
        return;
    }
    for (long long k : n.kids) freeVars(k, out, bound);
}

std::set<std::string> freeVars(long long h) {
    std::set<std::string> out, bound;
    freeVars(h, out, bound);
    return out;
}

std::string freshName(const std::string& base, const std::set<std::string>& avoid) {
    std::string name = base + "'";
    while (avoid.count(name)) name += "'";
    return name;
}

// f[x := t], renaming bound variables that would capture a free variable of t.
long long substitute(long long f, const std::string& x, long long t) {
    const Node n = node(f);
    if (n.kind == Kind::IntVar) return n.name == x ? t : f;
    if (n.kind == Kind::Forall || n.kind == Kind::Exists) {
        if (n.name == x) return f;
        long long body = n.kids[0];
        std::string bound = n.name;
        const std::set<std::string> termVars = freeVars(t);
        if (termVars.count(bound)) {
            std::set<std::string> avoid = termVars;
            for (const auto& v : freeVars(body)) avoid.insert(v);
            avoid.insert(x);
            const std::string renamed = freshName(bound, avoid);
            body = substitute(body, bound, make(Kind::IntVar, renamed, 0, {}));
            bound = renamed;
        }
        return make(n.kind, bound, 0, {substitute(body, x, t)});
    }
    if (n.kids.empty()) return f;
    std::vector<long long> kids;
    for (long long k : n.kids) kids.push_back(substitute(k, x, t));
    return make(n.kind, n.name, n.value, kids);
}

// Replace every occurrence of term a by term b where no variable of a or b is bound.
long long replace(long long f, long long a, long long b, const std::set<std::string>& guarded) {
    if (f == a) return b;
    const Node n = node(f);
    if (n.kind == Kind::Forall || n.kind == Kind::Exists) {
        if (guarded.count(n.name)) return f;
        return make(n.kind, n.name, 0, {replace(n.kids[0], a, b, guarded)});
    }
    if (n.kids.empty()) return f;
    std::vector<long long> kids;
    for (long long k : n.kids) kids.push_back(replace(k, a, b, guarded));
    return make(n.kind, n.name, n.value, kids);
}

// ---- SMT-LIB and Z3 ----

std::string symbol(const std::string& name) {
    std::string out = "|";
    for (char c : name) out += (c == '|' || c == '\\') ? '_' : c;
    return out + "|";
}

std::string smt(long long h) {
    const Node& n = node(h);
    static const char* cmpSmt[] = {"=", "distinct", "<", "<=", ">", ">="};
    switch (n.kind) {
        case Kind::True: return "true";
        case Kind::False: return "false";
        case Kind::Prop: case Kind::IntVar: return symbol(n.name);
        case Kind::Num: {
            if (n.value >= 0) return std::to_string(n.value);
            const unsigned long long magnitude = 0ULL - static_cast<unsigned long long>(n.value);
            return "(- " + std::to_string(magnitude) + ")";
        }
        case Kind::Not: return "(not " + smt(n.kids[0]) + ")";
        case Kind::And: return "(and " + smt(n.kids[0]) + " " + smt(n.kids[1]) + ")";
        case Kind::Or: return "(or " + smt(n.kids[0]) + " " + smt(n.kids[1]) + ")";
        case Kind::Implies: return "(=> " + smt(n.kids[0]) + " " + smt(n.kids[1]) + ")";
        case Kind::Iff: return "(= " + smt(n.kids[0]) + " " + smt(n.kids[1]) + ")";
        case Kind::Forall: return "(forall ((" + symbol(n.name) + " Int)) " + smt(n.kids[0]) + ")";
        case Kind::Exists: return "(exists ((" + symbol(n.name) + " Int)) " + smt(n.kids[0]) + ")";
        case Kind::Cmp: return "(" + std::string(cmpSmt[n.value]) + " " + smt(n.kids[0]) + " " + smt(n.kids[1]) + ")";
        case Kind::Add: return "(+ " + smt(n.kids[0]) + " " + smt(n.kids[1]) + ")";
        case Kind::Sub: return "(- " + smt(n.kids[0]) + " " + smt(n.kids[1]) + ")";
        case Kind::Mul: return "(* " + smt(n.kids[0]) + " " + smt(n.kids[1]) + ")";
        case Kind::Neg: return "(- " + smt(n.kids[0]) + ")";
    }
    return "true";
}

void collectProps(long long h, std::set<std::string>& props) {
    const Node& n = node(h);
    if (n.kind == Kind::Prop) props.insert(n.name);
    for (long long k : n.kids) collectProps(k, props);
}

using Z3Config = void*;
using Z3Context = void*;
struct Z3Api {
    bool loaded = false;
    std::string error;
    Z3Config (*mkConfig)() = nullptr;
    Z3Context (*mkContext)(Z3Config) = nullptr;
    void (*delConfig)(Z3Config) = nullptr;
    const char* (*evalSmtlib)(Z3Context, const char*) = nullptr;
    void (*delContext)(Z3Context) = nullptr;
};

Z3Api& z3() {
    static Z3Api api;
    static bool tried = false;
    std::lock_guard<std::recursive_mutex> lock(mutex());
    if (tried) return api;
    tried = true;
    std::vector<std::string> candidates;
    if (const char* path = std::getenv("CSEC_Z3_LIBRARY")) candidates.push_back(path);
#if defined(_WIN32)
    candidates.push_back("libz3.dll");
    auto open = [](const std::string& p) -> void* { return reinterpret_cast<void*>(LoadLibraryA(p.c_str())); };
    auto symbolOf = [](void* lib, const char* s) -> void* {
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(lib), s));
    };
#else
#if defined(__APPLE__)
    candidates.push_back("libz3.dylib");
#endif
    candidates.push_back("libz3.so");
    candidates.push_back("libz3.so.4");
    auto open = [](const std::string& p) -> void* { return dlopen(p.c_str(), RTLD_NOW); };
    auto symbolOf = [](void* lib, const char* s) -> void* { return dlsym(lib, s); };
#endif
    void* lib = nullptr;
    for (const auto& candidate : candidates) {
        lib = open(candidate);
        if (lib) break;
    }
    if (!lib) {
        api.error = "the Z3 solver library (libz3) was not found; put it next to the program or set CSEC_Z3_LIBRARY";
        return api;
    }
    api.mkConfig = reinterpret_cast<Z3Config (*)()>(symbolOf(lib, "Z3_mk_config"));
    api.mkContext = reinterpret_cast<Z3Context (*)(Z3Config)>(symbolOf(lib, "Z3_mk_context"));
    api.delConfig = reinterpret_cast<void (*)(Z3Config)>(symbolOf(lib, "Z3_del_config"));
    api.evalSmtlib = reinterpret_cast<const char* (*)(Z3Context, const char*)>(symbolOf(lib, "Z3_eval_smtlib2_string"));
    api.delContext = reinterpret_cast<void (*)(Z3Context)>(symbolOf(lib, "Z3_del_context"));
    api.loaded = api.mkConfig && api.mkContext && api.delConfig && api.evalSmtlib && api.delContext;
    if (!api.loaded) api.error = "libz3 does not export the expected Z3 C API";
    return api;
}

// Runs a script; returns Z3's output.
std::string runZ3(const std::string& script) {
    Z3Api& api = z3();
    if (!api.loaded) fail(api.error);
    Z3Config config = api.mkConfig();
    Z3Context context = api.mkContext(config);
    api.delConfig(config);
    const char* raw = api.evalSmtlib(context, script.c_str());
    std::string output = raw ? raw : "";
    api.delContext(context);
    return output;
}

// Satisfiability of h: 1 sat, 0 unsat, -1 unknown; `model` receives "x = 3, p = true" when sat.
int satisfiable(long long h, std::string* model) {
    std::string script = "(set-option :timeout 10000)\n";
    std::set<std::string> props;
    collectProps(h, props);
    const std::set<std::string> ints = freeVars(h);
    for (const auto& p : props) script += "(declare-const " + symbol(p) + " Bool)\n";
    for (const auto& v : ints) script += "(declare-const " + symbol(v) + " Int)\n";
    script += "(assert " + smt(h) + ")\n(check-sat)\n";
    const bool wantModel = model && (!props.empty() || !ints.empty());
    if (wantModel) {
        script += "(get-value (";
        for (const auto& p : props) script += symbol(p) + " ";
        for (const auto& v : ints) script += symbol(v) + " ";
        script += "))\n";
    }
    const std::string output = runZ3(script);
    if (output.rfind("unsat", 0) == 0) return 0;
    if (output.rfind("sat", 0) != 0) return -1;
    if (wantModel) {
        // ((|x| 3) (|p| true) (|y| (- 2)))
        std::string formatted;
        size_t cursor = output.find("((");
        while (cursor != std::string::npos) {
            const size_t bar = output.find('|', cursor);
            if (bar == std::string::npos) break;
            const size_t barEnd = output.find('|', bar + 1);
            if (barEnd == std::string::npos) break;
            const std::string name = output.substr(bar + 1, barEnd - bar - 1);
            size_t valueStart = barEnd + 1;
            while (valueStart < output.size() && output[valueStart] == ' ') ++valueStart;
            int depth = 0;
            size_t valueEnd = valueStart;
            for (; valueEnd < output.size(); ++valueEnd) {
                if (output[valueEnd] == '(') ++depth;
                else if (output[valueEnd] == ')') {
                    if (depth == 0) break;
                    --depth;
                }
            }
            std::string value = output.substr(valueStart, valueEnd - valueStart);
            if (value.rfind("(- ", 0) == 0) value = "-" + value.substr(3, value.size() - 4);
            if (!formatted.empty()) formatted += ", ";
            formatted += name + " = " + value;
            cursor = valueEnd;
        }
        *model = formatted;
    }
    else if (model) {
        model->clear();
    }
    return 1;
}

// ---- theorems ----

struct Theorem {
    std::vector<long long> hyps;  // sorted, unique
    long long conclusion = 0;
    bool oracle = false;
};

std::vector<Theorem>& theorems() {
    static auto* table = new std::vector<Theorem>();
    return *table;
}

const Theorem& thm(long long h) {
    if (h < 1 || static_cast<size_t>(h) > theorems().size()) fail("invalid theorem handle");
    return theorems()[static_cast<size_t>(h - 1)];
}

long long record(std::vector<long long> hyps, long long conclusion, bool oracle) {
    std::sort(hyps.begin(), hyps.end());
    hyps.erase(std::unique(hyps.begin(), hyps.end()), hyps.end());
    std::lock_guard<std::recursive_mutex> lock(mutex());
    theorems().push_back(Theorem{std::move(hyps), conclusion, oracle});
    return static_cast<long long>(theorems().size());
}

std::vector<long long> unite(const std::vector<long long>& a, const std::vector<long long>& b) {
    std::vector<long long> out = a;
    out.insert(out.end(), b.begin(), b.end());
    return out;
}

std::vector<long long> without(const std::vector<long long>& a, long long f) {
    std::vector<long long> out;
    for (long long h : a) if (h != f) out.push_back(h);
    return out;
}

const Node& shape(long long thmHandle, Kind kind, const char* rule, const char* expected) {
    const Node& n = node(thm(thmHandle).conclusion);
    if (n.kind != kind) {
        fail(std::string(rule) + " needs a theorem of the form " + expected + ", got " + text(thm(thmHandle).conclusion));
    }
    return n;
}

void requireSame(long long actual, long long expected, const char* rule) {
    if (actual != expected) fail(std::string(rule) + ": expected " + text(expected) + ", got " + text(actual));
}

char* ownedString(const std::string& s) {
    char* out = static_cast<char*>(std::malloc(s.size() + 1));
    if (out) std::memcpy(out, s.c_str(), s.size() + 1);
    return out;
}

} // namespace csec_logic

extern "C" {

// ---- formulas and terms ----

long long csec_logic_true(void) { return csec_logic::make(csec_logic::Kind::True, "", 0, {}); }
long long csec_logic_false(void) { return csec_logic::make(csec_logic::Kind::False, "", 0, {}); }
long long csec_logic_prop(const char* name) {
    if (!name || !*name) csec_logic::fail("a proposition needs a name");
    return csec_logic::make(csec_logic::Kind::Prop, name, 0, {});
}
long long csec_logic_not(long long a) {
    return csec_logic::make(csec_logic::Kind::Not, "", 0, {csec_logic::formula(a, "not")});
}
long long csec_logic_binary(int connective, long long a, long long b) {
    using csec_logic::Kind;
    static const Kind kinds[] = {Kind::And, Kind::Or, Kind::Implies, Kind::Iff};
    if (connective < 0 || connective > 3) csec_logic::fail("unknown connective");
    return csec_logic::make(kinds[connective], "", 0, {csec_logic::formula(a, "a connective"), csec_logic::formula(b, "a connective")});
}
long long csec_logic_quantifier(int universal, long long variable, long long body) {
    const auto& v = csec_logic::node(variable);
    if (v.kind != csec_logic::Kind::IntVar) csec_logic::fail("a quantifier binds an integer variable made by intVar(name)");
    return csec_logic::make(universal ? csec_logic::Kind::Forall : csec_logic::Kind::Exists, v.name, 0,
                            {csec_logic::formula(body, "a quantifier")});
}
long long csec_logic_int_var(const char* name) {
    if (!name || !*name) csec_logic::fail("an integer variable needs a name");
    return csec_logic::make(csec_logic::Kind::IntVar, name, 0, {});
}
long long csec_logic_num(long long value) { return csec_logic::make(csec_logic::Kind::Num, "", value, {}); }
long long csec_logic_arith(int op, long long a, long long b) {
    using csec_logic::Kind;
    static const Kind kinds[] = {Kind::Add, Kind::Sub, Kind::Mul};
    if (op < 0 || op > 2) csec_logic::fail("unknown arithmetic operator");
    return csec_logic::make(kinds[op], "", 0, {csec_logic::term(a, "arithmetic"), csec_logic::term(b, "arithmetic")});
}
long long csec_logic_neg(long long a) { return csec_logic::make(csec_logic::Kind::Neg, "", 0, {csec_logic::term(a, "negation")}); }
long long csec_logic_cmp(int op, long long a, long long b) {
    if (op < 0 || op > 5) csec_logic::fail("unknown comparison");
    return csec_logic::make(csec_logic::Kind::Cmp, "", op, {csec_logic::term(a, "a comparison"), csec_logic::term(b, "a comparison")});
}
char* csec_logic_to_string(long long h) { return csec_logic::ownedString(csec_logic::text(h)); }
long long csec_logic_substitute(long long f, long long variable, long long t) {
    const auto& v = csec_logic::node(variable);
    if (v.kind != csec_logic::Kind::IntVar) csec_logic::fail("substitute needs an integer variable");
    return csec_logic::substitute(f, v.name, csec_logic::term(t, "substitute"));
}

// ---- decision (Z3) ----

int csec_logic_z3_available(void) { return csec_logic::z3().loaded ? 1 : 0; }

// 1 valid, 0 not valid, -1 unknown.
int csec_logic_is_valid(long long f) {
    const int sat = csec_logic::satisfiable(csec_logic_not(csec_logic::formula(f, "isValid")), nullptr);
    return sat == 0 ? 1 : (sat == 1 ? 0 : -1);
}

int csec_logic_is_satisfiable(long long f) {
    return csec_logic::satisfiable(csec_logic::formula(f, "isSatisfiable"), nullptr);
}

// An assignment that makes f false ("" when f is valid).
char* csec_logic_counterexample(long long f) {
    std::string model;
    const int sat = csec_logic::satisfiable(csec_logic_not(csec_logic::formula(f, "counterexample")), &model);
    return csec_logic::ownedString(sat == 1 ? (model.empty() ? "(any assignment)" : model) : "");
}

// An assignment that makes f true ("" when f is unsatisfiable).
char* csec_logic_model(long long f) {
    std::string model;
    const int sat = csec_logic::satisfiable(csec_logic::formula(f, "model"), &model);
    return csec_logic::ownedString(sat == 1 ? (model.empty() ? "(any assignment)" : model) : "");
}

// ---- the proof kernel ----

long long csec_thm_assume(long long f) {
    csec_logic::formula(f, "assume");
    return csec_logic::record({f}, f, false);
}

long long csec_thm_true_intro(void) { return csec_logic::record({}, csec_logic_true(), false); }

long long csec_thm_imp_intro(long long f, long long t) {
    csec_logic::formula(f, "impIntro");
    const auto& th = csec_logic::thm(t);
    return csec_logic::record(csec_logic::without(th.hyps, f), csec_logic_binary(2, f, th.conclusion), th.oracle);
}

long long csec_thm_imp_elim(long long tImp, long long tA) {
    const auto& imp = csec_logic::shape(tImp, csec_logic::Kind::Implies, "impElim (modus ponens)", "A -> B");
    const auto& a = csec_logic::thm(tA);
    csec_logic::requireSame(a.conclusion, imp.kids[0], "impElim (modus ponens)");
    const auto& ti = csec_logic::thm(tImp);
    return csec_logic::record(csec_logic::unite(ti.hyps, a.hyps), imp.kids[1], ti.oracle || a.oracle);
}

long long csec_thm_and_intro(long long t1, long long t2) {
    const auto& a = csec_logic::thm(t1);
    const auto& b = csec_logic::thm(t2);
    return csec_logic::record(csec_logic::unite(a.hyps, b.hyps), csec_logic_binary(0, a.conclusion, b.conclusion), a.oracle || b.oracle);
}

long long csec_thm_and_elim(long long t, int which) {
    const auto& n = csec_logic::shape(t, csec_logic::Kind::And, which ? "andElim2" : "andElim1", "A & B");
    const auto& th = csec_logic::thm(t);
    return csec_logic::record(th.hyps, n.kids[which ? 1 : 0], th.oracle);
}

// which = 0: from A derive A | other; which = 1: from B derive other | B.
long long csec_thm_or_intro(long long t, long long other, int which) {
    const auto& th = csec_logic::thm(t);
    csec_logic::formula(other, "orIntro");
    const long long f = which ? csec_logic_binary(1, other, th.conclusion) : csec_logic_binary(1, th.conclusion, other);
    return csec_logic::record(th.hyps, f, th.oracle);
}

long long csec_thm_or_elim(long long tOr, long long tA, long long tB) {
    const auto& n = csec_logic::shape(tOr, csec_logic::Kind::Or, "orElim", "A | B");
    const auto& a = csec_logic::thm(tA);
    const auto& b = csec_logic::thm(tB);
    csec_logic::requireSame(b.conclusion, a.conclusion, "orElim (both cases must prove the same formula)");
    const auto& o = csec_logic::thm(tOr);
    auto hyps = csec_logic::unite(o.hyps, csec_logic::without(a.hyps, n.kids[0]));
    hyps = csec_logic::unite(hyps, csec_logic::without(b.hyps, n.kids[1]));
    return csec_logic::record(hyps, a.conclusion, o.oracle || a.oracle || b.oracle);
}

long long csec_thm_not_intro(long long f, long long t) {
    csec_logic::shape(t, csec_logic::Kind::False, "notIntro", "false");
    const auto& th = csec_logic::thm(t);
    return csec_logic::record(csec_logic::without(th.hyps, f), csec_logic_not(f), th.oracle);
}

long long csec_thm_not_elim(long long tA, long long tNotA) {
    const auto& n = csec_logic::shape(tNotA, csec_logic::Kind::Not, "notElim", "!A");
    const auto& a = csec_logic::thm(tA);
    csec_logic::requireSame(a.conclusion, n.kids[0], "notElim");
    const auto& na = csec_logic::thm(tNotA);
    return csec_logic::record(csec_logic::unite(a.hyps, na.hyps), csec_logic_false(), a.oracle || na.oracle);
}

long long csec_thm_ex_falso(long long t, long long f) {
    csec_logic::shape(t, csec_logic::Kind::False, "exFalso", "false");
    const auto& th = csec_logic::thm(t);
    return csec_logic::record(th.hyps, csec_logic::formula(f, "exFalso"), th.oracle);
}

// Classical reasoning: from false under the hypothesis !f, conclude f.
long long csec_thm_by_contradiction(long long f, long long t) {
    csec_logic::shape(t, csec_logic::Kind::False, "byContradiction", "false");
    const auto& th = csec_logic::thm(t);
    return csec_logic::record(csec_logic::without(th.hyps, csec_logic_not(f)), f, th.oracle);
}

long long csec_thm_iff_intro(long long tAB, long long tBA) {
    const auto& ab = csec_logic::shape(tAB, csec_logic::Kind::Implies, "iffIntro", "A -> B");
    const auto& ba = csec_logic::shape(tBA, csec_logic::Kind::Implies, "iffIntro", "B -> A");
    if (ab.kids[0] != ba.kids[1] || ab.kids[1] != ba.kids[0]) csec_logic::fail("iffIntro needs A -> B and B -> A");
    const auto& x = csec_logic::thm(tAB);
    const auto& y = csec_logic::thm(tBA);
    return csec_logic::record(csec_logic::unite(x.hyps, y.hyps), csec_logic_binary(3, ab.kids[0], ab.kids[1]), x.oracle || y.oracle);
}

long long csec_thm_iff_elim(long long t, int which) {
    const auto& n = csec_logic::shape(t, csec_logic::Kind::Iff, "iffElim", "A <-> B");
    const auto& th = csec_logic::thm(t);
    const long long f = which ? csec_logic_binary(2, n.kids[1], n.kids[0]) : csec_logic_binary(2, n.kids[0], n.kids[1]);
    return csec_logic::record(th.hyps, f, th.oracle);
}

long long csec_thm_all_intro(long long variable, long long t) {
    const auto& v = csec_logic::node(variable);
    if (v.kind != csec_logic::Kind::IntVar) csec_logic::fail("allIntro needs an integer variable");
    const auto& th = csec_logic::thm(t);
    for (long long h : th.hyps) {
        if (csec_logic::freeVars(h).count(v.name)) {
            csec_logic::fail("allIntro: " + v.name + " is free in the hypothesis " + csec_logic::text(h));
        }
    }
    return csec_logic::record(th.hyps, csec_logic::make(csec_logic::Kind::Forall, v.name, 0, {th.conclusion}), th.oracle);
}

long long csec_thm_all_elim(long long t, long long witness) {
    const auto& n = csec_logic::shape(t, csec_logic::Kind::Forall, "allElim", "forall x. P");
    const auto& th = csec_logic::thm(t);
    return csec_logic::record(th.hyps, csec_logic::substitute(n.kids[0], n.name, csec_logic::term(witness, "allElim")), th.oracle);
}

long long csec_thm_exists_intro(long long f, long long witness, long long t) {
    const auto& n = csec_logic::node(f);
    if (n.kind != csec_logic::Kind::Exists) csec_logic::fail("existsIntro needs a formula of the form exists x. P");
    const auto& th = csec_logic::thm(t);
    csec_logic::requireSame(th.conclusion, csec_logic::substitute(n.kids[0], n.name, csec_logic::term(witness, "existsIntro")), "existsIntro");
    return csec_logic::record(th.hyps, f, th.oracle);
}

long long csec_thm_refl(long long t) {
    csec_logic::term(t, "refl");
    return csec_logic::record({}, csec_logic_cmp(csec_logic::Eq, t, t), false);
}

// From a == b and P, derive P with a replaced by b (where no variable of a or b is bound).
long long csec_thm_rewrite(long long tEq, long long t) {
    const auto& eq = csec_logic::shape(tEq, csec_logic::Kind::Cmp, "rewrite", "a == b");
    if (eq.value != csec_logic::Eq) csec_logic::fail("rewrite needs an equation a == b");
    std::set<std::string> guarded = csec_logic::freeVars(eq.kids[0]);
    for (const auto& v : csec_logic::freeVars(eq.kids[1])) guarded.insert(v);
    const auto& e = csec_logic::thm(tEq);
    const auto& th = csec_logic::thm(t);
    return csec_logic::record(csec_logic::unite(e.hyps, th.hyps), csec_logic::replace(th.conclusion, eq.kids[0], eq.kids[1], guarded),
                              e.oracle || th.oracle);
}

// The oracle rule: f holds because Z3 proves it valid. The theorem records that it used the oracle.
long long csec_thm_decide(long long f) {
    std::string model;
    const int sat = csec_logic::satisfiable(csec_logic_not(csec_logic::formula(f, "decide")), &model);
    if (sat == 1) csec_logic::fail("decide: " + csec_logic::text(f) + " is not valid" + (model.empty() ? "" : " (counterexample: " + model + ")"));
    if (sat != 0) csec_logic::fail("decide: the solver could not decide " + csec_logic::text(f));
    return csec_logic::record({}, f, true);
}

long long csec_thm_conclusion(long long t) { return csec_logic::thm(t).conclusion; }
long long csec_thm_hypothesis_count(long long t) { return static_cast<long long>(csec_logic::thm(t).hyps.size()); }
long long csec_thm_hypothesis(long long t, long long i) {
    const auto& th = csec_logic::thm(t);
    if (i < 0 || static_cast<size_t>(i) >= th.hyps.size()) csec_logic::fail("hypothesis index out of range");
    return th.hyps[static_cast<size_t>(i)];
}
int csec_thm_uses_oracle(long long t) { return csec_logic::thm(t).oracle ? 1 : 0; }

char* csec_thm_to_string(long long t) {
    const auto& th = csec_logic::thm(t);
    std::string out;
    for (size_t i = 0; i < th.hyps.size(); ++i) out += (i ? ", " : "") + csec_logic::text(th.hyps[i]);
    out += (out.empty() ? "|- " : " |- ") + csec_logic::text(th.conclusion);
    if (th.oracle) out += "   [by Z3]";
    return csec_logic::ownedString(out);
}

} // extern "C"
