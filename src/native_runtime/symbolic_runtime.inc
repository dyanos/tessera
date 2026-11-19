// symbolic_runtime.inc -- self-contained symbolic algebra engine for the csec runtime.
//
// Expressions are immutable, hash-consed nodes kept in a global table guarded by a
// mutex. The public handle is a 1-based `long long` index (0 = invalid / error).
// Canonical (simplified) form:
//   * ADD: n-ary, flattened, like terms collected, numeric constant (if any) last,
//          terms ordered by a graded monomial order (x^2 before x before 1).
//   * MUL: n-ary, flattened, numeric coefficient (if != 1) first, equal bases merged
//          by summing exponents, factors ordered by base.
//   * POW: base^exponent; never with exponent 0 or 1.
//   * FUNC: sin cos tan exp ln sqrt abs, folded when the argument is numeric.
// Subtraction is a + (-1)*b and division is a * b^(-1); the printer renders these as
// "a - b" and "a/b".
//
// Included last from NativeRuntime.cpp (after windows.h, so std::min/std::max are
// deliberately avoided). Compiles standalone as well.

#ifndef CSEC_NATIVE_API
#define CSEC_NATIVE_API
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace csec_symbolic {

typedef long long Id;

enum Kind { K_NUM = 0, K_VAR = 1, K_FUNC = 2, K_POW = 3, K_MUL = 4, K_ADD = 5 };
enum Prec { P_ADD = 1, P_NEG = 2, P_MUL = 3, P_POW = 4, P_ATOM = 5 };

// Recursion guards (stack safety). The byte budget is measured from the public API entry
// point, so it holds on small stacks too (runtime fibers get 512 KB, see process_runtime.inc).
static const int kMaxDepth = 4000;
static const size_t kMaxStackBytes = 192 * 1024;
static const size_t kMaxExpandTerms = 20000; // expansion blow-up guard
static const int kMaxExpandPow = 16;         // (a+b)^n expanded only for 2 <= n <= 16

struct Node {
    Kind kind;
    double num;
    std::string name;
    std::vector<Id> ch;
};

struct SymError {};

struct Store {
    std::deque<Node> nodes; // deque: references stay valid across push_back
    std::unordered_map<std::string, Id> index;
    std::mutex mtx;
    int depth = 0;
    uintptr_t stackBase = 0; // address of a local at API entry (stack grows down)
};

inline Store& store() {
    static Store s;
    return s;
}

struct DepthGuard {
    DepthGuard() {
        Store& s = store();
        char here = 0;
        uintptr_t cur = reinterpret_cast<uintptr_t>(&here);
        uintptr_t used = s.stackBase > cur ? s.stackBase - cur : cur - s.stackBase;
        if (s.depth + 1 > kMaxDepth || used > kMaxStackBytes) throw SymError();
        ++s.depth;
    }
    ~DepthGuard() { --store().depth; }
    DepthGuard(const DepthGuard&) = delete;
    DepthGuard& operator=(const DepthGuard&) = delete;
};

inline bool valid(Id id) {
    return id >= 1 && static_cast<size_t>(id) <= store().nodes.size();
}
inline const Node& node(Id id) { return store().nodes[static_cast<size_t>(id - 1)]; }

// ---------------------------------------------------------------------------
// Interning (hash-consing)
// ---------------------------------------------------------------------------

inline Id intern(Kind k, double v, const std::string& name, const std::vector<Id>& ch) {
    Store& s = store();
    if (k != K_NUM) v = 0.0;
    if (v == 0.0) v = 0.0; // normalize -0.0
    std::string key;
    key.reserve(24 + name.size() + ch.size() * 8);
    key += static_cast<char>('0' + static_cast<int>(k));
    key += '|';
    if (k == K_NUM) {
        unsigned long long bits = 0;
        std::memcpy(&bits, &v, sizeof bits);
        char buf[24];
        std::snprintf(buf, sizeof buf, "%016llx", bits);
        key += buf;
    } else {
        key += std::to_string(name.size());
        key += ':';
        key += name;
    }
    for (Id c : ch) {
        key += ',';
        key += std::to_string(c);
    }
    auto it = s.index.find(key);
    if (it != s.index.end()) return it->second;
    Node n;
    n.kind = k;
    n.num = v;
    n.name = name;
    n.ch = ch;
    s.nodes.push_back(std::move(n));
    Id id = static_cast<Id>(s.nodes.size());
    s.index.emplace(std::move(key), id);
    return id;
}

inline Id mkNum(double v) { return intern(K_NUM, v, std::string(), std::vector<Id>()); }
inline Id mkVar(const std::string& n) { return intern(K_VAR, 0.0, n, std::vector<Id>()); }
inline Id mk(Kind k, const std::vector<Id>& ch) { return intern(k, 0.0, std::string(), ch); }
inline Id mk2(Kind k, Id a, Id b) {
    std::vector<Id> v;
    v.push_back(a);
    v.push_back(b);
    return mk(k, v);
}
inline Id mkFunc(const std::string& f, Id a) {
    return intern(K_FUNC, 0.0, f, std::vector<Id>(1, a));
}

inline bool isNum(Id e) { return node(e).kind == K_NUM; }
inline double numVal(Id e) { return node(e).num; }
inline bool isNumV(Id e, double v) {
    const Node& n = node(e);
    return n.kind == K_NUM && n.num == v;
}
inline bool isIntVal(double v) {
    return std::isfinite(v) && v == std::floor(v) && std::fabs(v) < 1e15;
}

inline bool isKnownFunc(const std::string& f) {
    return f == "sin" || f == "cos" || f == "tan" || f == "exp" || f == "ln" || f == "sqrt" ||
           f == "abs";
}

inline double applyFunc(const std::string& f, double x) {
    if (f == "sin") return std::sin(x);
    if (f == "cos") return std::cos(x);
    if (f == "tan") return std::tan(x);
    if (f == "exp") return std::exp(x);
    if (f == "ln") return std::log(x);
    if (f == "sqrt") return std::sqrt(x);
    if (f == "abs") return std::fabs(x);
    return std::nan("");
}

// ---------------------------------------------------------------------------
// Ordering
// ---------------------------------------------------------------------------

inline int cmpNode(Id a, Id b) {
    if (a == b) return 0;
    DepthGuard g;
    const Node& x = node(a);
    const Node& y = node(b);
    if (x.kind != y.kind) return x.kind < y.kind ? -1 : 1;
    if (x.kind == K_NUM) {
        const bool xNan = std::isnan(x.num);
        const bool yNan = std::isnan(y.num);
        if (xNan || yNan) return xNan == yNan ? 0 : (xNan ? 1 : -1);
        return x.num < y.num ? -1 : (x.num > y.num ? 1 : 0);
    }
    if (x.kind == K_VAR || x.kind == K_FUNC) {
        int c = x.name.compare(y.name);
        if (c != 0) return c < 0 ? -1 : 1;
        if (x.kind == K_VAR) return 0;
    }
    size_t n = x.ch.size() < y.ch.size() ? x.ch.size() : y.ch.size();
    for (size_t i = 0; i < n; ++i) {
        int c = cmpNode(x.ch[i], y.ch[i]);
        if (c != 0) return c;
    }
    if (x.ch.size() != y.ch.size()) return x.ch.size() < y.ch.size() ? -1 : 1;
    return 0;
}

inline Id baseOf(Id e) {
    const Node& n = node(e);
    return n.kind == K_POW ? n.ch[0] : e;
}
inline Id expOf(Id e) {
    const Node& n = node(e);
    return n.kind == K_POW ? n.ch[1] : mkNum(1.0);
}

// Splits a canonical term into numeric coefficient and the remaining monomial.
inline Id splitCoef(Id t, double& k) {
    const Node& n = node(t);
    if (n.kind == K_MUL && n.ch.size() >= 2 && isNum(n.ch[0])) {
        k = numVal(n.ch[0]);
        if (n.ch.size() == 2) return n.ch[1];
        return mk(K_MUL, std::vector<Id>(n.ch.begin() + 1, n.ch.end()));
    }
    k = 1.0;
    return t;
}

inline void factorList(Id r, std::vector<Id>& out) {
    const Node& n = node(r);
    if (n.kind == K_MUL)
        out = n.ch;
    else
        out.assign(1, r);
}

// Graded monomial order: for the first differing factor, the smaller base comes first;
// with equal bases, the larger exponent comes first; longer monomials come first.
inline int monoCmp(Id ra, Id rb) {
    if (ra == rb) return 0;
    std::vector<Id> fa, fb;
    factorList(ra, fa);
    factorList(rb, fb);
    size_t n = fa.size() < fb.size() ? fa.size() : fb.size();
    for (size_t i = 0; i < n; ++i) {
        int c = cmpNode(baseOf(fa[i]), baseOf(fb[i]));
        if (c != 0) return c;
        Id ea = expOf(fa[i]);
        Id eb = expOf(fb[i]);
        if (isNum(ea) && isNum(eb)) {
            double x = numVal(ea), y = numVal(eb);
            if (x > y) return -1;
            if (x < y) return 1;
        } else {
            c = cmpNode(ea, eb);
            if (c != 0) return c;
        }
    }
    if (fa.size() != fb.size()) return fa.size() > fb.size() ? -1 : 1;
    return cmpNode(ra, rb);
}

// ---------------------------------------------------------------------------
// Canonical builders (inputs must already be canonical)
// ---------------------------------------------------------------------------

inline Id sAdd(const std::vector<Id>& in);
inline Id sMul(const std::vector<Id>& in);
inline Id sPow(Id b, Id e);
inline Id sFunc(const std::string& f, Id a);

inline Id sMul2(Id a, Id b) {
    std::vector<Id> v;
    v.push_back(a);
    v.push_back(b);
    return sMul(v);
}
inline Id sMul3(Id a, Id b, Id c) {
    std::vector<Id> v;
    v.push_back(a);
    v.push_back(b);
    v.push_back(c);
    return sMul(v);
}
inline Id sAdd2(Id a, Id b) {
    std::vector<Id> v;
    v.push_back(a);
    v.push_back(b);
    return sAdd(v);
}
inline Id sNeg(Id a) { return sMul2(mkNum(-1.0), a); }
inline Id sRecip(Id a) { return sPow(a, mkNum(-1.0)); }

inline Id scaleTerm(double k, Id r) {
    if (k == 1.0) return r;
    const Node& n = node(r);
    std::vector<Id> v;
    v.push_back(mkNum(k));
    if (n.kind == K_MUL)
        v.insert(v.end(), n.ch.begin(), n.ch.end());
    else
        v.push_back(r);
    return mk(K_MUL, v);
}

inline Id sAdd(const std::vector<Id>& in) {
    DepthGuard g;
    std::vector<Id> flat;
    for (Id t : in) {
        const Node& n = node(t);
        if (n.kind == K_ADD)
            flat.insert(flat.end(), n.ch.begin(), n.ch.end());
        else
            flat.push_back(t);
    }
    double c = 0.0;
    std::vector<Id> rests;
    std::vector<double> coefs;
    std::unordered_map<Id, size_t> pos;
    for (Id t : flat) {
        if (isNum(t)) {
            c += numVal(t);
            continue;
        }
        double k = 1.0;
        Id r = splitCoef(t, k);
        auto it = pos.find(r);
        if (it == pos.end()) {
            pos.emplace(r, rests.size());
            rests.push_back(r);
            coefs.push_back(k);
        } else {
            coefs[it->second] += k;
        }
    }
    std::vector<size_t> order;
    for (size_t i = 0; i < rests.size(); ++i)
        if (coefs[i] != 0.0) order.push_back(i);
    std::stable_sort(order.begin(), order.end(),
                     [&rests](size_t a, size_t b) { return monoCmp(rests[a], rests[b]) < 0; });
    std::vector<Id> terms;
    for (size_t i : order) terms.push_back(scaleTerm(coefs[i], rests[i]));
    if (terms.empty()) return mkNum(c);
    if (c != 0.0) terms.push_back(mkNum(c));
    if (terms.size() == 1) return terms[0];
    return mk(K_ADD, terms);
}

inline Id sMul(const std::vector<Id>& in) {
    DepthGuard g;
    std::vector<Id> flat;
    for (Id f : in) {
        const Node& n = node(f);
        if (n.kind == K_MUL)
            flat.insert(flat.end(), n.ch.begin(), n.ch.end());
        else
            flat.push_back(f);
    }
    double c = 1.0;
    std::vector<Id> bases;
    std::vector<std::vector<Id>> exps;
    std::unordered_map<Id, size_t> pos;
    for (Id f : flat) {
        if (isNum(f)) {
            c *= numVal(f);
            continue;
        }
        Id b = baseOf(f);
        Id e = expOf(f);
        auto it = pos.find(b);
        if (it == pos.end()) {
            pos.emplace(b, bases.size());
            bases.push_back(b);
            exps.push_back(std::vector<Id>(1, e));
        } else {
            exps[it->second].push_back(e);
        }
    }
    if (c == 0.0) return mkNum(0.0);
    std::vector<Id> facs;
    bool again = false;
    for (size_t i = 0; i < bases.size(); ++i) {
        Id e = exps[i].size() == 1 ? exps[i][0] : sAdd(exps[i]);
        Id p = sPow(bases[i], e);
        const Node& pn = node(p);
        if (pn.kind == K_NUM) {
            c *= pn.num;
        } else {
            if (pn.kind == K_MUL) again = true;
            facs.push_back(p);
        }
    }
    if (c == 0.0) return mkNum(0.0);
    if (again) {
        facs.push_back(mkNum(c));
        return sMul(facs);
    }
    std::sort(facs.begin(), facs.end(),
              [](Id a, Id b) { return cmpNode(baseOf(a), baseOf(b)) < 0; });
    if (facs.empty()) return mkNum(c);
    if (c != 1.0) facs.insert(facs.begin(), mkNum(c));
    if (facs.size() == 1) return facs[0];
    return mk(K_MUL, facs);
}

inline Id sPow(Id b, Id e) {
    DepthGuard g;
    if (isNum(e)) {
        double ev = numVal(e);
        if (ev == 0.0) return mkNum(1.0);
        if (ev == 1.0) return b;
        if (isNum(b)) {
            double r = std::pow(numVal(b), ev);
            if (std::isfinite(r)) return mkNum(r);
        }
        const Node& bn = node(b);
        if (isIntVal(ev)) {
            if (bn.kind == K_POW) return sPow(bn.ch[0], sMul2(bn.ch[1], e));
            if (bn.kind == K_MUL) {
                std::vector<Id> f;
                for (Id c : bn.ch) f.push_back(sPow(c, e));
                return sMul(f);
            }
            if (bn.kind == K_FUNC && bn.name == "sqrt" && isIntVal(ev / 2.0))
                return sPow(bn.ch[0], mkNum(ev / 2.0));
        }
    }
    if (isNum(b)) {
        double bv = numVal(b);
        if (bv == 1.0) return mkNum(1.0);
        if (bv == 0.0 && isNum(e) && numVal(e) > 0.0) return mkNum(0.0);
    }
    return mk2(K_POW, b, e);
}

inline Id sFunc(const std::string& f, Id a) {
    if (isNum(a)) {
        double r = applyFunc(f, numVal(a));
        if (std::isfinite(r)) return mkNum(r);
    }
    const Node& an = node(a);
    if (an.kind == K_FUNC) {
        if (f == "ln" && an.name == "exp") return an.ch[0];
        if (f == "exp" && an.name == "ln") return an.ch[0];
        if (f == "abs" && (an.name == "abs" || an.name == "exp" || an.name == "sqrt")) return a;
    }
    return mkFunc(f, a);
}

// ---------------------------------------------------------------------------
// Simplify / expand
// ---------------------------------------------------------------------------

inline Id simpRec(Id e, std::unordered_map<Id, Id>& memo) {
    auto it = memo.find(e);
    if (it != memo.end()) return it->second;
    DepthGuard g;
    const Node& n = node(e);
    Id r = e;
    switch (n.kind) {
    case K_NUM:
    case K_VAR:
        r = e;
        break;
    case K_FUNC:
        r = sFunc(n.name, simpRec(n.ch[0], memo));
        break;
    case K_POW: {
        Id b = simpRec(n.ch[0], memo);
        Id x = simpRec(n.ch[1], memo);
        r = sPow(b, x);
        break;
    }
    case K_MUL:
    case K_ADD: {
        std::vector<Id> v;
        for (Id c : n.ch) v.push_back(simpRec(c, memo));
        r = n.kind == K_ADD ? sAdd(v) : sMul(v);
        break;
    }
    }
    memo[e] = r;
    return r;
}

inline Id simp(Id e) {
    std::unordered_map<Id, Id> memo;
    return simpRec(e, memo);
}

inline Id mulDistribute(const std::vector<Id>& factors) {
    std::vector<Id> acc(1, mkNum(1.0));
    for (Id f : factors) {
        std::vector<Id> parts;
        const Node& fn = node(f);
        if (fn.kind == K_ADD)
            parts = fn.ch;
        else
            parts.assign(1, f);
        if (acc.size() * parts.size() > kMaxExpandTerms) throw SymError();
        std::vector<Id> next;
        next.reserve(acc.size() * parts.size());
        for (Id a : acc) {
            for (Id p : parts) {
                Id t = sMul2(a, p);
                const Node& tn = node(t);
                if (tn.kind == K_ADD)
                    next.insert(next.end(), tn.ch.begin(), tn.ch.end());
                else
                    next.push_back(t);
            }
        }
        acc.swap(next);
    }
    return sAdd(acc);
}

inline Id expandRec(Id e) {
    DepthGuard g;
    const Node& n = node(e);
    switch (n.kind) {
    case K_NUM:
    case K_VAR:
        return e;
    case K_FUNC:
        return sFunc(n.name, expandRec(n.ch[0]));
    case K_ADD: {
        std::vector<Id> v;
        for (Id c : n.ch) v.push_back(expandRec(c));
        return sAdd(v);
    }
    case K_MUL: {
        std::vector<Id> v;
        for (Id c : n.ch) v.push_back(expandRec(c));
        return mulDistribute(v);
    }
    case K_POW: {
        Id b = expandRec(n.ch[0]);
        Id x = expandRec(n.ch[1]);
        if (node(b).kind == K_ADD && isNum(x)) {
            double xv = numVal(x);
            if (isIntVal(xv) && xv >= 2.0 && xv <= static_cast<double>(kMaxExpandPow)) {
                std::vector<Id> f(static_cast<size_t>(xv), b);
                return mulDistribute(f);
            }
        }
        Id r = sPow(b, x);
        const Node& rn = node(r);
        if (rn.kind == K_MUL) {
            std::vector<Id> v;
            for (Id c : rn.ch) v.push_back(expandRec(c));
            return mulDistribute(v);
        }
        return r;
    }
    }
    return e;
}

inline Id expandFull(Id e) { return simp(expandRec(simp(e))); }

// ---------------------------------------------------------------------------
// Calculus
// ---------------------------------------------------------------------------

inline bool depends(Id e, const std::string& v) {
    DepthGuard g;
    const Node& n = node(e);
    if (n.kind == K_VAR) return n.name == v;
    for (Id c : n.ch)
        if (depends(c, v)) return true;
    return false;
}

// e must be canonical; result is canonical.
inline Id diffRec(Id e, const std::string& v) {
    DepthGuard g;
    const Node& n = node(e);
    switch (n.kind) {
    case K_NUM:
        return mkNum(0.0);
    case K_VAR:
        return mkNum(n.name == v ? 1.0 : 0.0);
    case K_ADD: {
        std::vector<Id> t;
        for (Id c : n.ch) t.push_back(diffRec(c, v));
        return sAdd(t);
    }
    case K_MUL: {
        std::vector<Id> terms;
        for (size_t i = 0; i < n.ch.size(); ++i) {
            Id d = diffRec(n.ch[i], v);
            if (isNumV(d, 0.0)) continue;
            std::vector<Id> f = n.ch;
            f[i] = d;
            terms.push_back(sMul(f));
        }
        return sAdd(terms);
    }
    case K_POW: {
        Id b = n.ch[0], x = n.ch[1];
        bool db = depends(b, v), dx = depends(x, v);
        if (!db && !dx) return mkNum(0.0);
        if (!dx) // x * b^(x-1) * b'
            return sMul3(x, sPow(b, sAdd2(x, mkNum(-1.0))), diffRec(b, v));
        if (!db) // b^x * ln(b) * x'
            return sMul3(e, sFunc("ln", b), diffRec(x, v));
        // b^x * (x' ln b + x b'/b)
        Id t1 = sMul2(diffRec(x, v), sFunc("ln", b));
        Id t2 = sMul3(x, diffRec(b, v), sRecip(b));
        return sMul2(e, sAdd2(t1, t2));
    }
    case K_FUNC: {
        Id a = n.ch[0];
        Id da = diffRec(a, v);
        if (isNumV(da, 0.0)) return mkNum(0.0);
        const std::string& f = n.name;
        if (f == "sin") return sMul2(sFunc("cos", a), da);
        if (f == "cos") return sMul3(mkNum(-1.0), sFunc("sin", a), da);
        if (f == "tan") return sMul2(sPow(sFunc("cos", a), mkNum(-2.0)), da);
        if (f == "exp") return sMul2(e, da);
        if (f == "ln") return sMul2(da, sRecip(a));
        if (f == "sqrt") return sMul3(mkNum(0.5), da, sRecip(e));
        if (f == "abs") return sMul3(da, a, sRecip(e));
        throw SymError();
    }
    }
    throw SymError();
}

// Returns d(u)/d(var) if u is linear in var with a nonzero var-free slope, else 0.
inline Id linearSlope(Id u, const std::string& v) {
    Id d = diffRec(u, v);
    if (isNumV(d, 0.0) || depends(d, v)) return 0;
    return d;
}

// e canonical. Returns 0 if no antiderivative found.
inline Id integRec(Id e, const std::string& v) {
    DepthGuard g;
    Id x = mkVar(v);
    if (!depends(e, v)) return sMul2(e, x);
    const Node& n = node(e);
    switch (n.kind) {
    case K_VAR:
        return sMul2(mkNum(0.5), sPow(x, mkNum(2.0)));
    case K_ADD: {
        std::vector<Id> rs;
        for (Id c : n.ch) {
            Id r = integRec(c, v);
            if (r == 0) return 0;
            rs.push_back(r);
        }
        return sAdd(rs);
    }
    case K_MUL: {
        std::vector<Id> consts, deps;
        for (Id c : n.ch) (depends(c, v) ? deps : consts).push_back(c);
        if (deps.size() != 1 || consts.empty()) return 0;
        Id r = integRec(deps[0], v);
        if (r == 0) return 0;
        consts.push_back(r);
        return sMul(consts);
    }
    case K_POW: {
        Id b = n.ch[0], p = n.ch[1];
        if (!depends(p, v)) {
            Id a = linearSlope(b, v);
            if (a == 0) return 0;
            if (isNumV(p, -1.0)) return sMul2(sFunc("ln", sFunc("abs", b)), sRecip(a));
            Id p1 = sAdd2(p, mkNum(1.0));
            return sMul2(sPow(b, p1), sRecip(sMul2(a, p1)));
        }
        if (!depends(b, v)) {
            Id a = linearSlope(p, v);
            if (a == 0) return 0;
            return sMul2(e, sRecip(sMul2(a, sFunc("ln", b))));
        }
        return 0;
    }
    case K_FUNC: {
        Id u = n.ch[0];
        Id a = linearSlope(u, v);
        if (a == 0) return 0;
        Id inv = sRecip(a);
        const std::string& f = n.name;
        if (f == "sin") return sMul3(mkNum(-1.0), sFunc("cos", u), inv);
        if (f == "cos") return sMul2(sFunc("sin", u), inv);
        if (f == "exp") return sMul2(e, inv);
        if (f == "ln") return sMul2(sAdd2(sMul2(u, e), sNeg(u)), inv);
        return 0;
    }
    default:
        return 0;
    }
}

inline Id substRec(Id e, const std::string& v, Id rep, std::unordered_map<Id, Id>& memo) {
    auto it = memo.find(e);
    if (it != memo.end()) return it->second;
    DepthGuard g;
    const Node& n = node(e);
    Id r = e;
    switch (n.kind) {
    case K_NUM:
        break;
    case K_VAR:
        r = n.name == v ? rep : e;
        break;
    case K_FUNC:
        r = sFunc(n.name, substRec(n.ch[0], v, rep, memo));
        break;
    case K_POW:
        r = sPow(substRec(n.ch[0], v, rep, memo), substRec(n.ch[1], v, rep, memo));
        break;
    case K_MUL:
    case K_ADD: {
        std::vector<Id> c;
        for (Id k : n.ch) c.push_back(substRec(k, v, rep, memo));
        r = n.kind == K_ADD ? sAdd(c) : sMul(c);
        break;
    }
    }
    memo[e] = r;
    return r;
}

inline double evalRec(Id e, const std::map<std::string, double>& env) {
    DepthGuard g;
    const Node& n = node(e);
    switch (n.kind) {
    case K_NUM:
        return n.num;
    case K_VAR: {
        auto it = env.find(n.name);
        return it == env.end() ? std::nan("") : it->second;
    }
    case K_FUNC:
        return applyFunc(n.name, evalRec(n.ch[0], env));
    case K_POW:
        return std::pow(evalRec(n.ch[0], env), evalRec(n.ch[1], env));
    case K_MUL: {
        double r = 1.0;
        for (Id c : n.ch) r *= evalRec(c, env);
        return r;
    }
    case K_ADD: {
        double r = 0.0;
        for (Id c : n.ch) r += evalRec(c, env);
        return r;
    }
    }
    return std::nan("");
}

// ---------------------------------------------------------------------------
// Polynomials
// ---------------------------------------------------------------------------

// Decomposes canonical expanded e into coefficient term lists per degree of v.
inline bool polyDecompose(Id e, const std::string& v, std::map<int, std::vector<Id>>& out) {
    std::vector<Id> terms;
    const Node& n = node(e);
    if (n.kind == K_ADD)
        terms = n.ch;
    else
        terms.assign(1, e);
    Id x = mkVar(v);
    for (Id t : terms) {
        std::vector<Id> fs;
        factorList(t, fs);
        int deg = 0;
        std::vector<Id> rest;
        for (Id f : fs) {
            const Node& fn = node(f);
            if (f == x) {
                deg += 1;
            } else if (fn.kind == K_POW && fn.ch[0] == x && isNum(fn.ch[1]) &&
                       isIntVal(numVal(fn.ch[1])) && numVal(fn.ch[1]) >= 0.0 &&
                       numVal(fn.ch[1]) < 1e6) {
                deg += static_cast<int>(numVal(fn.ch[1]));
            } else if (depends(f, v)) {
                return false;
            } else {
                rest.push_back(f);
            }
        }
        out[deg].push_back(rest.empty() ? mkNum(1.0) : sMul(rest));
    }
    return true;
}

// Returns canonical coefficient per degree (zero coefficients removed); false if not polynomial.
inline bool polyCoefs(Id e, const std::string& v, std::map<int, Id>& coefs) {
    std::map<int, std::vector<Id>> raw;
    if (!polyDecompose(expandFull(e), v, raw)) return false;
    for (auto& kv : raw) {
        Id c = sAdd(kv.second);
        if (!isNumV(c, 0.0)) coefs[kv.first] = c;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Printing
// ---------------------------------------------------------------------------

inline std::string fmtNum(double v) {
    if (std::isnan(v)) return "nan";
    if (std::isinf(v)) return v > 0 ? "inf" : "-inf";
    if (v == 0.0) return "0";
    char buf[64];
    if (v == std::floor(v) && std::fabs(v) < 1e15)
        std::snprintf(buf, sizeof buf, "%.0f", v);
    else
        std::snprintf(buf, sizeof buf, "%.12g", v);
    return buf;
}

inline std::string wrapParen(const std::string& s, bool tex) {
    return tex ? "\\left(" + s + "\\right)" : "(" + s + ")";
}

// Each node kind is printed by its own function so debug-build stack frames stay small.
inline std::string pr(Id e, bool tex, int& prec);

inline std::string prFunc(const Node& n, bool tex, int& prec) {
    int p = 0;
    std::string a = pr(n.ch[0], tex, p);
    prec = P_ATOM;
    if (!tex) return n.name + "(" + a + ")";
    if (n.name == "sqrt") return "\\sqrt{" + a + "}";
    if (n.name == "abs") return "\\left|" + a + "\\right|";
    if (n.name == "exp") {
        prec = P_POW;
        return "e^{" + a + "}";
    }
    return "\\" + n.name + "\\left(" + a + "\\right)";
}

inline std::string prPow(const Node& n, bool tex, int& prec) {
    Id b = n.ch[0], x = n.ch[1];
    if (isNum(x) && numVal(x) < 0.0) {
        double ne = -numVal(x);
        Id pos = ne == 1.0 ? b : mk2(K_POW, b, mkNum(ne));
        int p = 0;
        std::string d = pr(pos, tex, p);
        prec = P_MUL;
        if (tex) return "\\frac{1}{" + d + "}";
        if (p < P_POW) d = wrapParen(d, false);
        return "1/" + d;
    }
    int pb = 0, px = 0;
    std::string bs = pr(b, tex, pb);
    if (pb < P_ATOM) bs = wrapParen(bs, tex);
    std::string xs = pr(x, tex, px);
    prec = P_POW;
    if (tex) return bs + "^{" + xs + "}";
    if (px < P_POW) xs = wrapParen(xs, false);
    return bs + "^" + xs;
}

inline void prFactor(Id f, bool tex, std::string& out, bool afterCoef) {
    int p = 0;
    std::string s = pr(f, tex, p);
    if (p < P_MUL) s = wrapParen(s, tex);
    if (!out.empty()) {
        if (!tex)
            out += "*";
        else if (afterCoef && !(s[0] >= '0' && s[0] <= '9') && s[0] != '.')
            out += " ";
        else
            out += " \\cdot ";
    }
    out += s;
}

inline std::string prMul(const Node& n, bool tex, int& prec) {
    double coef = 1.0;
    size_t start = 0;
    if (isNum(n.ch[0])) {
        coef = numVal(n.ch[0]);
        start = 1;
    }
    std::vector<Id> numer, denom;
    for (size_t i = start; i < n.ch.size(); ++i) {
        Id f = n.ch[i];
        const Node& fn = node(f);
        if (fn.kind == K_POW && isNum(fn.ch[1]) && numVal(fn.ch[1]) < 0.0) {
            double ne = -numVal(fn.ch[1]);
            denom.push_back(ne == 1.0 ? fn.ch[0] : mk2(K_POW, fn.ch[0], mkNum(ne)));
        } else {
            numer.push_back(f);
        }
    }
    bool neg = false;
    if (coef < 0.0) {
        neg = true;
        coef = -coef;
    }
    std::string num;
    if (coef != 1.0) num = fmtNum(coef);
    bool afterCoef = !num.empty();
    for (Id f : numer) {
        prFactor(f, tex, num, afterCoef);
        afterCoef = false;
    }
    if (num.empty()) num = "1";
    std::string body;
    if (denom.empty()) {
        body.swap(num);
    } else {
        std::string den;
        for (Id f : denom) prFactor(f, tex, den, false);
        if (tex) {
            body = "\\frac{" + num + "}{" + den + "}";
        } else {
            bool simpleDen = false;
            if (denom.size() == 1) {
                Kind k = node(denom[0]).kind;
                simpleDen = k == K_VAR || k == K_FUNC || k == K_POW ||
                            (k == K_NUM && numVal(denom[0]) >= 0.0);
            }
            body = num + "/" + (simpleDen ? den : "(" + den + ")");
        }
    }
    prec = neg ? P_NEG : P_MUL;
    return neg ? "-" + body : body;
}

inline std::string prAdd(const Node& n, bool tex, int& prec) {
    std::string out;
    for (size_t i = 0; i < n.ch.size(); ++i) {
        int p = 0;
        std::string s = pr(n.ch[i], tex, p);
        if (i == 0)
            out.swap(s);
        else if (!s.empty() && s[0] == '-')
            out.append(" - ").append(s, 1, std::string::npos);
        else
            out.append(" + ").append(s);
    }
    prec = P_ADD;
    return out;
}

inline std::string pr(Id e, bool tex, int& prec) {
    DepthGuard g;
    const Node& n = node(e);
    switch (n.kind) {
    case K_NUM: {
        std::string s = fmtNum(n.num);
        prec = s[0] == '-' ? P_NEG : P_ATOM;
        return s;
    }
    case K_VAR:
        prec = P_ATOM;
        return n.name;
    case K_FUNC:
        return prFunc(n, tex, prec);
    case K_POW:
        return prPow(n, tex, prec);
    case K_MUL:
        return prMul(n, tex, prec);
    case K_ADD:
        return prAdd(n, tex, prec);
    }
    prec = P_ATOM;
    return std::string();
}

inline std::string toStr(Id e, bool tex) {
    int p = 0;
    return pr(e, tex, p);
}

// ---------------------------------------------------------------------------
// Parser (raw trees; n-ary sums/products)
// ---------------------------------------------------------------------------

inline Id rawNeg(Id x) {
    if (isNum(x)) return mkNum(-numVal(x));
    return mk2(K_MUL, mkNum(-1.0), x);
}

struct Parser {
    const char* s;
    size_t i;

    void ws() {
        while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r') ++i;
    }
    static bool isAlpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    static bool isDigit(char c) { return c >= '0' && c <= '9'; }

    Id expr() {
        std::vector<Id> terms;
        terms.push_back(term());
        for (;;) {
            ws();
            if (s[i] == '+') {
                ++i;
                terms.push_back(term());
            } else if (s[i] == '-') {
                ++i;
                terms.push_back(rawNeg(term()));
            } else {
                break;
            }
        }
        return terms.size() == 1 ? terms[0] : mk(K_ADD, terms);
    }
    Id term() {
        std::vector<Id> f;
        f.push_back(unary());
        for (;;) {
            ws();
            if (s[i] == '*' && s[i + 1] != '*') {
                ++i;
                f.push_back(unary());
            } else if (s[i] == '/') {
                ++i;
                f.push_back(mk2(K_POW, unary(), mkNum(-1.0)));
            } else {
                break;
            }
        }
        return f.size() == 1 ? f[0] : mk(K_MUL, f);
    }
    Id unary() {
        DepthGuard g;
        ws();
        if (s[i] == '-') {
            ++i;
            return rawNeg(unary());
        }
        if (s[i] == '+') {
            ++i;
            return unary();
        }
        return power();
    }
    Id power() {
        Id b = primary();
        ws();
        if (s[i] == '^') {
            ++i;
            return mk2(K_POW, b, unary());
        }
        if (s[i] == '*' && s[i + 1] == '*') {
            i += 2;
            return mk2(K_POW, b, unary());
        }
        return b;
    }
    Id primary() {
        DepthGuard g;
        ws();
        char c = s[i];
        if (isDigit(c) || (c == '.' && isDigit(s[i + 1]))) {
            size_t st = i;
            while (isDigit(s[i])) ++i;
            if (s[i] == '.') {
                ++i;
                while (isDigit(s[i])) ++i;
            }
            if (s[i] == 'e' || s[i] == 'E') {
                size_t j = i + 1;
                if (s[j] == '+' || s[j] == '-') ++j;
                if (isDigit(s[j])) {
                    i = j;
                    while (isDigit(s[i])) ++i;
                }
            }
            std::string lit(s + st, i - st);
            return mkNum(std::strtod(lit.c_str(), nullptr));
        }
        if (isAlpha(c)) {
            size_t st = i;
            while (isAlpha(s[i]) || isDigit(s[i])) ++i;
            std::string name(s + st, i - st);
            ws();
            if (s[i] == '(') {
                if (!isKnownFunc(name)) throw SymError();
                ++i;
                Id a = expr();
                ws();
                if (s[i] != ')') throw SymError();
                ++i;
                return mkFunc(name, a);
            }
            return mkVar(name);
        }
        if (c == '(') {
            ++i;
            Id a = expr();
            ws();
            if (s[i] != ')') throw SymError();
            ++i;
            return a;
        }
        throw SymError();
    }
};

inline Id parse(const char* text) {
    Parser p;
    p.s = text;
    p.i = 0;
    Id r = p.expr();
    p.ws();
    if (text[p.i] != '\0') throw SymError();
    return r;
}

// ---------------------------------------------------------------------------
// API helpers
// ---------------------------------------------------------------------------

inline char* dupStr(const std::string& s) {
    char* out = static_cast<char*>(csecAlloc(s.size() + 1));
    if (!out) return nullptr;
    std::memcpy(out, s.c_str(), s.size() + 1);
    return out;
}

template <class R, class F>
R guarded(R fallback, F f) {
    std::lock_guard<std::mutex> lock(store().mtx);
    char marker = 0;
    store().stackBase = reinterpret_cast<uintptr_t>(&marker);
    store().depth = 0;
    try {
        return f();
    } catch (...) {
        store().depth = 0;
        return fallback;
    }
}

inline Id flatRaw(Kind k, Id a, Id b) {
    std::vector<Id> v;
    const Node& na = node(a);
    if (na.kind == k)
        v = na.ch;
    else
        v.push_back(a);
    const Node& nb = node(b);
    if (nb.kind == k)
        v.insert(v.end(), nb.ch.begin(), nb.ch.end());
    else
        v.push_back(b);
    return mk(k, v);
}

inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\n' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\n' || s[b - 1] == '\r'))
        --b;
    return s.substr(a, b - a);
}

inline double snapRoot(double r) {
    double k = std::floor(r + 0.5);
    double scale = std::fabs(r) > 1.0 ? std::fabs(r) : 1.0;
    if (std::fabs(r - k) <= 1e-12 * scale) return k == 0.0 ? 0.0 : k;
    return r;
}

inline const double kNaN = std::nan("");

} // namespace csec_symbolic

// ===========================================================================
// Public C API
// ===========================================================================

extern "C" {

CSEC_NATIVE_API long long csec_sym_num(double value) {
    using namespace csec_symbolic;
    return guarded<Id>(0, [&]() { return mkNum(value); });
}

CSEC_NATIVE_API long long csec_sym_var(const char* name) {
    using namespace csec_symbolic;
    if (!name || !*name) return 0;
    return guarded<Id>(0, [&]() { return mkVar(std::string(name)); });
}

CSEC_NATIVE_API long long csec_sym_add(long long a, long long b) {
    using namespace csec_symbolic;
    return guarded<Id>(0, [&]() -> Id {
        if (!valid(a) || !valid(b)) return 0;
        return flatRaw(K_ADD, a, b);
    });
}

CSEC_NATIVE_API long long csec_sym_sub(long long a, long long b) {
    using namespace csec_symbolic;
    return guarded<Id>(0, [&]() -> Id {
        if (!valid(a) || !valid(b)) return 0;
        return flatRaw(K_ADD, a, rawNeg(b));
    });
}

CSEC_NATIVE_API long long csec_sym_mul(long long a, long long b) {
    using namespace csec_symbolic;
    return guarded<Id>(0, [&]() -> Id {
        if (!valid(a) || !valid(b)) return 0;
        return flatRaw(K_MUL, a, b);
    });
}

CSEC_NATIVE_API long long csec_sym_div(long long a, long long b) {
    using namespace csec_symbolic;
    return guarded<Id>(0, [&]() -> Id {
        if (!valid(a) || !valid(b)) return 0;
        return flatRaw(K_MUL, a, mk2(K_POW, b, mkNum(-1.0)));
    });
}

CSEC_NATIVE_API long long csec_sym_pow(long long a, long long b) {
    using namespace csec_symbolic;
    return guarded<Id>(0, [&]() -> Id {
        if (!valid(a) || !valid(b)) return 0;
        return mk2(K_POW, a, b);
    });
}

CSEC_NATIVE_API long long csec_sym_neg(long long a) {
    using namespace csec_symbolic;
    return guarded<Id>(0, [&]() -> Id {
        if (!valid(a)) return 0;
        return rawNeg(a);
    });
}

CSEC_NATIVE_API long long csec_sym_func(const char* name, long long a) {
    using namespace csec_symbolic;
    if (!name) return 0;
    return guarded<Id>(0, [&]() -> Id {
        std::string f(name);
        if (!valid(a) || !isKnownFunc(f)) return 0;
        return mkFunc(f, a);
    });
}

CSEC_NATIVE_API long long csec_sym_parse(const char* text) {
    using namespace csec_symbolic;
    if (!text) return 0;
    return guarded<Id>(0, [&]() { return parse(text); });
}

CSEC_NATIVE_API long long csec_sym_simplify(long long e) {
    using namespace csec_symbolic;
    return guarded<Id>(0, [&]() -> Id { return valid(e) ? simp(e) : 0; });
}

CSEC_NATIVE_API long long csec_sym_expand(long long e) {
    using namespace csec_symbolic;
    return guarded<Id>(0, [&]() -> Id { return valid(e) ? expandFull(e) : 0; });
}

CSEC_NATIVE_API long long csec_sym_diff(long long e, const char* var) {
    using namespace csec_symbolic;
    if (!var || !*var) return 0;
    return guarded<Id>(0, [&]() -> Id {
        if (!valid(e)) return 0;
        return simp(diffRec(simp(e), std::string(var)));
    });
}

CSEC_NATIVE_API long long csec_sym_integrate(long long e, const char* var) {
    using namespace csec_symbolic;
    if (!var || !*var) return 0;
    return guarded<Id>(0, [&]() -> Id {
        if (!valid(e)) return 0;
        std::string v(var);
        Id s = simp(e);
        Id r = integRec(s, v);
        if (r == 0) {
            Id x = expandFull(s);
            if (x != s) r = integRec(x, v);
        }
        return r == 0 ? 0 : simp(r);
    });
}

CSEC_NATIVE_API long long csec_sym_subst(long long e, const char* var, long long replacement) {
    using namespace csec_symbolic;
    if (!var || !*var) return 0;
    return guarded<Id>(0, [&]() -> Id {
        if (!valid(e) || !valid(replacement)) return 0;
        std::unordered_map<Id, Id> memo;
        return simp(substRec(simp(e), std::string(var), simp(replacement), memo));
    });
}

CSEC_NATIVE_API double csec_sym_eval(long long e, const char* var, double value) {
    using namespace csec_symbolic;
    return guarded<double>(kNaN, [&]() -> double {
        if (!valid(e)) return kNaN;
        std::map<std::string, double> env;
        if (var && *var) env[var] = value;
        return evalRec(e, env);
    });
}

CSEC_NATIVE_API double csec_sym_eval_env(long long e, const char* bindings) {
    using namespace csec_symbolic;
    return guarded<double>(kNaN, [&]() -> double {
        if (!valid(e)) return kNaN;
        std::map<std::string, double> env;
        std::string all = bindings ? bindings : "";
        size_t p = 0;
        while (p <= all.size()) {
            size_t q = all.find_first_of(";,", p);
            if (q == std::string::npos) q = all.size();
            std::string item = trim(all.substr(p, q - p));
            p = q + 1;
            if (item.empty()) continue;
            size_t eq = item.find('=');
            if (eq == std::string::npos) return kNaN;
            std::string name = trim(item.substr(0, eq));
            std::string val = trim(item.substr(eq + 1));
            if (name.empty() || val.empty()) return kNaN;
            char* end = nullptr;
            double d = std::strtod(val.c_str(), &end);
            if (!end || *end != '\0') return kNaN;
            env[name] = d;
        }
        return evalRec(e, env);
    });
}

CSEC_NATIVE_API int csec_sym_is_number(long long e) {
    using namespace csec_symbolic;
    return guarded<int>(0, [&]() -> int { return valid(e) && isNum(simp(e)) ? 1 : 0; });
}

CSEC_NATIVE_API double csec_sym_number_value(long long e) {
    using namespace csec_symbolic;
    return guarded<double>(kNaN, [&]() -> double {
        if (!valid(e)) return kNaN;
        Id s = simp(e);
        return isNum(s) ? numVal(s) : kNaN;
    });
}

CSEC_NATIVE_API int csec_sym_equals(long long a, long long b) {
    using namespace csec_symbolic;
    return guarded<int>(0, [&]() -> int {
        if (!valid(a) || !valid(b)) return 0;
        Id sa = simp(a), sb = simp(b);
        if (sa == sb) return 1;
        Id d = expandFull(sAdd2(sa, sNeg(sb)));
        return isNumV(d, 0.0) ? 1 : 0;
    });
}

CSEC_NATIVE_API int csec_sym_depends_on(long long e, const char* var) {
    using namespace csec_symbolic;
    if (!var || !*var) return 0;
    return guarded<int>(0, [&]() -> int {
        return valid(e) && depends(simp(e), std::string(var)) ? 1 : 0;
    });
}

CSEC_NATIVE_API long long csec_sym_coefficient(long long e, const char* var, int degree) {
    using namespace csec_symbolic;
    if (!var || !*var) return 0;
    return guarded<Id>(0, [&]() -> Id {
        if (e == 0 || degree < 0) return mkNum(0.0);
        if (!valid(e)) return 0;
        std::map<int, Id> c;
        if (!polyCoefs(e, std::string(var), c)) return 0;
        auto it = c.find(degree);
        return it == c.end() ? mkNum(0.0) : it->second;
    });
}

CSEC_NATIVE_API int csec_sym_degree(long long e, const char* var) {
    using namespace csec_symbolic;
    if (!var || !*var) return -1;
    return guarded<int>(-1, [&]() -> int {
        if (!valid(e)) return -1;
        std::map<int, Id> c;
        if (!polyCoefs(e, std::string(var), c)) return -1;
        return c.empty() ? 0 : c.rbegin()->first;
    });
}

CSEC_NATIVE_API char* csec_sym_solve(long long e, const char* var) {
    using namespace csec_symbolic;
    std::string out = guarded<std::string>(std::string(), [&]() -> std::string {
        if (!var || !*var || !valid(e)) return std::string();
        std::map<int, Id> c;
        if (!polyCoefs(e, std::string(var), c) || c.empty()) return std::string();
        int deg = c.rbegin()->first;
        Id zero = mkNum(0.0);
        Id c0 = c.count(0) ? c[0] : zero;
        Id c1 = c.count(1) ? c[1] : zero;
        if (deg == 1) return toStr(sMul3(mkNum(-1.0), c0, sRecip(c1)), false);
        if (deg != 2) return std::string();
        Id c2 = c[2];
        if (!isNum(c0) || !isNum(c1) || !isNum(c2)) return std::string();
        double a = numVal(c2), b = numVal(c1), k = numVal(c0);
        double d = b * b - 4.0 * a * k;
        if (d >= 0.0) {
            double sq = std::sqrt(d);
            double r1 = snapRoot((-b - sq) / (2.0 * a));
            double r2 = snapRoot((-b + sq) / (2.0 * a));
            if (r1 > r2) {
                double t = r1;
                r1 = r2;
                r2 = t;
            }
            if (d == 0.0 || r1 == r2) return fmtNum(r1);
            return fmtNum(r1) + ";" + fmtNum(r2);
        }
        double re = snapRoot(-b / (2.0 * a));
        double im = snapRoot(std::sqrt(-d) / (2.0 * std::fabs(a)));
        std::string ims = im == 1.0 ? std::string() : fmtNum(im);
        if (re == 0.0) return ims + "i;-" + ims + "i";
        std::string rs = fmtNum(re);
        return rs + "+" + ims + "i;" + rs + "-" + ims + "i";
    });
    return dupStr(out);
}

CSEC_NATIVE_API char* csec_sym_to_string(long long e) {
    using namespace csec_symbolic;
    std::string out = guarded<std::string>(std::string(), [&]() -> std::string {
        return valid(e) ? toStr(e, false) : std::string();
    });
    return dupStr(out);
}

CSEC_NATIVE_API char* csec_sym_to_latex(long long e) {
    using namespace csec_symbolic;
    std::string out = guarded<std::string>(std::string(), [&]() -> std::string {
        return valid(e) ? toStr(e, true) : std::string();
    });
    return dupStr(out);
}

} // extern "C"
