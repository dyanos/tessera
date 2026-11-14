// Static verification with Z3 (`csec++ --verify`).
//
// Each function of the main file is executed symbolically. Integer values are bit-vectors of their
// machine width (Int = 32 bits, Long = 64, ...), so arithmetic wraps exactly as the compiled
// program does, and Bool values are Booleans. Execution splits at `if`, keeping a path condition;
// every check asks Z3 whether the path condition together with the negated property is satisfiable
// (a model is a counterexample). Calls are modular: the callee's `requires` is checked and only its
// `ensures` is assumed about the result. Loops are cut with their invariants: the invariant must hold
// on entry and be preserved by an iteration that starts from an arbitrary state satisfying it.
//
// Anything outside the modelled fragment (other types, method calls, collections, ...) becomes an
// unconstrained value, so the verifier can report a spurious failure there but never proves a false
// property; such places are listed as notes.

#include "verifier.h"
#include "source_map.h"
#include "all_ast.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

VerifierOptions& verifierOptions() {
    static VerifierOptions options;
    return options;
}

// ---------------------------------------------------------------------------------------------
// Expression text for messages
// ---------------------------------------------------------------------------------------------

static std::string exprText(ASTNode* node);

std::string verifierExpressionText(ASTNode* node) { return exprText(node); }

static std::string exprText(ASTNode* node) {
    if (!node) return "?";
    if (auto* value = dynamic_cast<ValueNode*>(node)) {
        return value->valueType == TokenType::STRING_LITERAL ? "\"" + value->value + "\"" : value->value;
    }
    if (auto* id = dynamic_cast<IdentifierNode*>(node)) return id->value;
    if (auto* binary = dynamic_cast<BinaryExpressionNode*>(node)) {
        auto precedence = [](const std::string& op) {
            if (op == "or" || op == "implies" || op == "iff") return 1;
            if (op == "and") return 2;
            if (op == "|" || op == "^" || op == "xor") return 3;
            if (op == "&") return 4;
            if (op == "==" || op == "!=") return 5;
            if (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "in" || op == "notin") return 6;
            if (op == "<<" || op == ">>") return 7;
            if (op == "+" || op == "-") return 8;
            return 9;
        };
        const int own = precedence(binary->op);
        auto side = [&](ASTNode* n, bool right) {
            const std::string text = exprText(n);
            auto* child = dynamic_cast<BinaryExpressionNode*>(n);
            if (!child) return text;
            const int inner = precedence(child->op);
            return inner < own || (right && inner == own) ? "(" + text + ")" : text;
        };
        return side(binary->left.get(), false) + " " + binary->op + " " + side(binary->right.get(), true);
    }
    if (auto* unary = dynamic_cast<UnaryExpressionNode*>(node)) {
        const std::string text = exprText(unary->expression.get());
        return unary->op + (dynamic_cast<BinaryExpressionNode*>(unary->expression.get()) ? "(" + text + ")" : text);
    }
    if (auto* call = dynamic_cast<FunctionCallNode*>(node)) {
        std::string text = call->functionName + "(";
        for (size_t i = 0; i < call->arguments.size(); ++i) {
            if (i > 0) text += ", ";
            text += exprText(call->arguments[i].get());
        }
        return text + ")";
    }
    if (auto* method = dynamic_cast<MethodCallNode*>(node)) {
        return exprText(method->object.get()) + "." + method->methodName + "(...)";
    }
    return "<expression>";
}


#if !defined(CSEC_HAVE_Z3)

int verifyProgram(ASTNode*, const std::string&) {
    std::cerr << "error: this csec++ was built without Z3, so --verify is unavailable. Run "
                 "scripts/fetch_z3.ps1 (Windows) or scripts/fetch_z3.sh, then rebuild." << std::endl;
    return 2;
}

#else

#include <z3++.h>

namespace {

// ---------------------------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------------------------

enum class Severity { Error, Warning, Note };

struct Diagnostic {
    Severity severity = Severity::Error;
    int line = 0;
    std::string message;
    std::string detail;
};

std::string locationText(int expandedLine) {
    const SourceLocation location = sourceLocationOf(expandedLine);
    if (location.file.empty()) return "<input>:" + std::to_string(expandedLine);
    std::string file = location.file;
    const size_t slash = file.find_last_of("/\\");
    if (slash != std::string::npos) file = file.substr(slash + 1);
    return file + ":" + std::to_string(location.line);
}

// ---------------------------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------------------------

// 0 width + !isBool: a value the verifier does not model.
struct Sort {
    bool isBool = false;
    unsigned width = 0;
    bool supported() const { return isBool || width > 0; }
    bool operator==(const Sort& o) const { return isBool == o.isBool && width == o.width; }
};

Sort sortOf(const Type* type) {
    if (!type) return {};
    if (type->getKind() == Type::Kind::BORROW || type->getKind() == Type::Kind::MUTABLE_BORROW) {
        auto* borrow = dynamic_cast<const BorrowType*>(type);
        return borrow && borrow->baseType ? sortOf(borrow->baseType.get()) : Sort{};
    }
    if (!type->isBasicType()) return {};
    const std::string& name = type->getName();
    if (name == "Bool" || name == "Boolean") return {true, 0};
    if (name == "Int") return {false, 32};
    if (name == "Long" || name == "Natural" || name == "Integer") return {false, 64};
    if (name == "Short") return {false, 16};
    if (name == "Byte" || name == "Char") return {false, 8};
    return {};
}

std::string sortName(const Sort& sort) {
    if (sort.isBool) return "Bool";
    switch (sort.width) {
        case 8: return "Byte";
        case 16: return "Short";
        case 32: return "Int";
        case 64: return "Long";
        default: return "?";
    }
}

struct Val {
    Sort sort;
    std::optional<z3::expr> e;
    bool known() const { return sort.supported() && e.has_value(); }
};

// ---------------------------------------------------------------------------------------------
// Symbolic states
// ---------------------------------------------------------------------------------------------

struct Frame {
    // Bindings shadowed by declarations in this block, restored when it ends.
    std::vector<std::pair<std::string, std::optional<Val>>> saved;
};

struct State {
    z3::expr pc;
    std::map<std::string, Val> env;
    std::vector<Frame> frames;
    explicit State(z3::context& ctx) : pc(ctx.bool_val(true)) {}
};

// ---------------------------------------------------------------------------------------------
// Program-wide information
// ---------------------------------------------------------------------------------------------

struct ProgramInfo {
    std::map<std::string, std::vector<FunctionDeclarationNode*>> functions;
};

void collectAssigned(ASTNode* node, std::set<std::string>& names) {
    if (!node) return;
    if (auto* assign = dynamic_cast<AssignmentExpressionNode*>(node)) {
        if (auto* id = dynamic_cast<IdentifierNode*>(assign->left.get())) names.insert(id->value);
        collectAssigned(assign->right.get(), names);
        return;
    }
    if (auto* postfix = dynamic_cast<PostfixExpressionNode*>(node)) {
        if (auto* id = dynamic_cast<IdentifierNode*>(postfix->expression.get())) names.insert(id->value);
        return;
    }
    if (auto* destructuring = dynamic_cast<DestructuringAssignmentNode*>(node)) {
        for (const auto& target : destructuring->targets) names.insert(target.name);
        return;
    }
    if (auto* block = dynamic_cast<BlockNode*>(node)) {
        for (auto& s : block->statements) collectAssigned(s.get(), names);
        return;
    }
    if (auto* ifNode = dynamic_cast<IfStatementNode*>(node)) {
        collectAssigned(ifNode->thenBlock.get(), names);
        collectAssigned(ifNode->elseBlock.get(), names);
        return;
    }
    if (auto* loop = dynamic_cast<WhileStatementNode*>(node)) {
        collectAssigned(loop->body.get(), names);
        return;
    }
    if (auto* loop = dynamic_cast<ForStatementNode*>(node)) {
        collectAssigned(loop->body.get(), names);
        return;
    }
    if (auto* decl = dynamic_cast<VariableDeclarationNode*>(node)) {
        collectAssigned(decl->initializer.get(), names);
        return;
    }
}

// ---------------------------------------------------------------------------------------------
// The verifier for one function
// ---------------------------------------------------------------------------------------------

class FunctionVerifier {
public:
    FunctionVerifier(z3::context& ctx, const ProgramInfo& program, FunctionDeclarationNode& function,
                     std::vector<Diagnostic>& diagnostics)
        : ctx(ctx), program(program), function(function), diagnostics(diagnostics) {}

    int obligations = 0;
    int failures = 0;

    void run() {
        State start(ctx);
        currentLine = function.line;
        std::vector<std::string> unsupportedParameters;
        for (auto& param : function.parameters) {
            auto* p = dynamic_cast<ParameterNode*>(param.get());
            if (!p) continue;
            const Sort sort = sortOf(p->type.get());
            Val value;
            value.sort = sort;
            if (sort.supported()) {
                value.e = fresh(sort, p->name);
                parameterValues.push_back({p->name, *value.e});
            }
            else {
                unsupportedParameters.push_back(p->name);
            }
            start.env[p->name] = value;
        }
        if (!unsupportedParameters.empty()) {
            std::string list;
            for (const auto& name : unsupportedParameters) list += (list.empty() ? "" : ", ") + name;
            note(function.line, "parameters of types the verifier does not model are unconstrained: " + list);
        }
        returnSort = sortOf(function.returnType.get());

        // `requires` clauses are assumed; they must not contradict each other.
        if (function.contracts) {
            for (auto& clause : function.contracts->requiresClauses) {
                Val c = evalContract(clause.expression.get(), start);
                if (c.known() && c.sort.isBool) start.pc = start.pc && *c.e;
            }
            if (!function.contracts->requiresClauses.empty() && !satisfiable(start.pc)) {
                error(function.contracts->requiresClauses.front().line,
                      "the requires clauses of '" + function.name + "' contradict each other; it can never be called");
                return;
            }
        }

        std::vector<State> ends = execBlock(function.body.get(), {start});
        // Falling off the end of the body: for a Unit function this is the return.
        for (auto& s : ends) {
            if (returnSort.supported()) continue;  // the type checker requires a return
            checkEnsures(s, std::nullopt, function.line);
        }
        reportBranchFacts();
    }

private:
    z3::context& ctx;
    const ProgramInfo& program;
    FunctionDeclarationNode& function;
    std::vector<Diagnostic>& diagnostics;
    std::vector<std::pair<std::string, z3::expr>> parameterValues;
    std::vector<z3::expr> background;  // range facts of every introduced integer
    Sort returnSort;
    int currentLine = 0;
    int freshCounter = 0;
    size_t liveStates = 0;
    bool tooManyPaths = false;
    bool suppressSideChecks = false;
    std::set<std::string> notedUnsupported;

    struct BranchFacts {
        bool thenPossible = false;
        bool elsePossible = false;
        int line = 0;
        std::string text;
        bool hasElse = false;
    };
    std::map<ASTNode*, BranchFacts> branchFacts;

    struct Invariant {
        z3::expr condition;
        std::string text;
        int line = 0;
    };
    std::vector<ASTNode*> branchOrder;

    // ---- solver helpers ----

    // ---- the integer encoding ----
    // Default: mathematical integers, every value constrained to its type's range and every result
    // checked to stay in it (so the model agrees with the machine). --verify-wrap: bit-vectors of the
    // machine width, with the compiled program's wrap-around.

    bool intMode() const { return !verifierOptions().wrapAround; }

    z3::expr num(long long value, unsigned width) {
        if (intMode()) return ctx.int_val(static_cast<int64_t>(value));
        return ctx.bv_val(static_cast<uint64_t>(value), width);
    }

    long long minOf(unsigned width) { return width >= 64 ? static_cast<long long>(1ULL << 63) : -(1LL << (width - 1)); }
    long long maxOf(unsigned width) { return width >= 64 ? static_cast<long long>((1ULL << 63) - 1) : (1LL << (width - 1)) - 1; }

    z3::expr inRange(const z3::expr& e, unsigned width) {
        if (!intMode()) return ctx.bool_val(true);
        return e >= ctx.int_val(static_cast<int64_t>(minOf(width))) && e <= ctx.int_val(static_cast<int64_t>(maxOf(width)));
    }

    // Sign-extension / truncation between integer widths (a no-op on mathematical integers; narrowing
    // there is checked by the caller).
    z3::expr resize(const z3::expr& e, unsigned from, unsigned to) {
        if (intMode() || from == to) return e;
        if (from < to) return z3::sext(e, to - from);
        return e.extract(to - 1, 0);
    }

    // C-style division and remainder (truncating toward zero).
    z3::expr divide(const z3::expr& a, const z3::expr& b) {
        if (!intMode()) return a / b;
        const z3::expr zero = ctx.int_val(0);
        const z3::expr absA = z3::ite(a >= zero, a, -a);
        const z3::expr absB = z3::ite(b >= zero, b, -b);
        const z3::expr q = absA / absB;  // both non-negative: floor == truncation
        return z3::ite((a >= zero) == (b >= zero), q, -q);
    }

    z3::expr remainder(const z3::expr& a, const z3::expr& b) {
        if (!intMode()) return z3::srem(a, b);
        return a - b * divide(a, b);
    }

    z3::expr toBits(const z3::expr& e, unsigned width) { return intMode() ? z3::int2bv(width, e) : e; }
    z3::expr fromBits(const z3::expr& e) { return intMode() ? z3::bv2int(e, true) : e; }

    z3::expr fresh(const Sort& sort, const std::string& hint) {
        const std::string name = hint + "!" + std::to_string(freshCounter++);
        if (sort.isBool) return ctx.bool_const(name.c_str());
        if (!intMode()) return ctx.bv_const(name.c_str(), sort.width);
        z3::expr value = ctx.int_const(name.c_str());
        background.push_back(inRange(value, sort.width));  // every value of a type lies in its range
        return value;
    }

    z3::check_result solve(const z3::expr& formula, std::optional<z3::model>* model = nullptr) {
        z3::solver solver(ctx);
        z3::params params(ctx);
        params.set("timeout", verifierOptions().timeoutMilliseconds);
        solver.set(params);
        solver.add(formula);
        for (const auto& fact : background) solver.add(fact);
        const z3::check_result result = solver.check();
        if (result == z3::sat && model) *model = solver.get_model();
        return result;
    }

    bool satisfiable(const z3::expr& formula) {
        // Unknown counts as satisfiable: never conclude "impossible" without a proof.
        return solve(formula) != z3::unsat;
    }

    std::string valueText(const z3::model& model, const z3::expr& e, const Sort& sort) {
        z3::expr v = model.eval(e, true);
        if (sort.isBool) return v.is_true() ? "true" : "false";
        if (v.is_int()) {
            int64_t value = 0;
            return v.is_numeral_i64(value) ? std::to_string(value) : v.to_string();
        }
        uint64_t bits = 0;
        if (!v.is_numeral_u64(bits)) return v.to_string();
        if (sort.width < 64 && (bits >> (sort.width - 1)) & 1) {
            const long long signedValue = static_cast<long long>(bits) - (1LL << sort.width);
            return std::to_string(signedValue);
        }
        return std::to_string(static_cast<long long>(bits));
    }

    // The parameters' values, then the locals (and `result`) at the failing point.
    std::string counterexample(const z3::model& model, const State& s) {
        std::string text;
        std::set<std::string> shown;
        for (const auto& [name, e] : parameterValues) {
            Sort sort;
            sort.isBool = e.is_bool();
            sort.width = e.is_bv() ? e.get_sort().bv_size() : 64;
            if (!text.empty()) text += ", ";
            text += name + " = " + valueText(model, e, sort);
            shown.insert(name);
        }
        int locals = 0;
        for (const auto& [name, value] : s.env) {
            if (shown.count(name) || !value.known() || locals >= 10) continue;
            ++locals;
            if (!text.empty()) text += ", ";
            text += name + " = " + valueText(model, *value.e, value.sort);
        }
        return text.empty() ? "" : "counterexample: " + text;
    }

    // Proves `property` on the path; reports `message` with a counterexample otherwise.
    bool check(const State& s, const z3::expr& property, int line, const std::string& message, bool asError = true) {
        ++obligations;
        std::optional<z3::model> model;
        const z3::check_result result = solve(s.pc && !property, &model);
        if (result == z3::unsat) return true;
        ++failures;
        Diagnostic d;
        d.severity = asError ? Severity::Error : Severity::Warning;
        d.line = line;
        if (result == z3::unknown) {
            d.message = message + " (the solver could not decide it within the time limit; see --verify-timeout)";
        }
        else {
            d.message = message;
            if (model) d.detail = counterexample(*model, s);
        }
        add(d);
        return false;
    }

    void add(const Diagnostic& d) {
        for (const auto& existing : diagnostics) {
            if (existing.line == d.line && existing.message == d.message) return;  // one report per place
        }
        diagnostics.push_back(d);
    }

    void error(int line, const std::string& message) { add({Severity::Error, line, message, ""}); }
    void warning(int line, const std::string& message) { add({Severity::Warning, line, message, ""}); }
    void note(int line, const std::string& message) { add({Severity::Note, line, message, ""}); }

    void unsupported(const std::string& what) {
        if (notedUnsupported.insert(what).second) {
            note(currentLine, what + " is not modelled; its value is treated as unknown");
        }
    }

    // ---- values ----

    Val unknown(const Sort& sort, const std::string& hint) {
        Val v;
        v.sort = sort;
        if (sort.supported()) v.e = fresh(sort, hint);
        return v;
    }

    Val unknownOf(ASTNode* node, const std::string& what) {
        unsupported(what);
        auto type = node ? node->getType() : nullptr;
        return unknown(sortOf(type.get()), "unknown");
    }

    z3::expr widen(const Val& v, unsigned width) {
        if (v.sort.width >= width) return *v.e;
        return resize(*v.e, v.sort.width, width);
    }

    // Converts an integer value to `sort` the way assignments and returns do (sign-extend/truncate).
    std::optional<z3::expr> convert(const Val& v, const Sort& sort) {
        if (!v.known() || !sort.supported()) return std::nullopt;
        if (sort.isBool != v.sort.isBool) return std::nullopt;
        if (sort.isBool || v.sort.width == sort.width) return *v.e;
        return resize(*v.e, v.sort.width, sort.width);
    }

    // Converting to a narrower type must keep the value (integer encoding) -- it would wrap otherwise.
    void checkNarrowing(const State& s, const Val& v, const Sort& sort, const std::string& what) {
        if (!intMode() || !v.known() || v.sort.isBool || !sort.supported() || sort.isBool) return;
        if (sort.width >= v.sort.width) return;
        sideCheck(s, ctx.bool_val(true), inRange(*v.e, sort.width),
                  what + " may not fit in " + sortName(sort) + " (the value would wrap around)");
    }

    // The result of an arithmetic operation stays in its type's range.
    void checkRange(const State& s, const z3::expr& guard, const z3::expr& e, unsigned width, const std::string& text) {
        if (!intMode()) return;
        sideCheck(s, guard, inRange(e, width), "'" + text + "' may overflow " + sortName({false, width}));
    }

    // ---- expressions ----

    Val evalContract(ASTNode* node, State& s) {
        const bool saved = suppressSideChecks;
        suppressSideChecks = true;
        Val v = eval(node, s);
        suppressSideChecks = saved;
        return v;
    }

    Val boolVal(const z3::expr& e) {
        Val v;
        v.sort = {true, 0};
        v.e = e;
        return v;
    }

    Val intVal(const z3::expr& e, unsigned width) {
        Val v;
        v.sort = {false, width};
        v.e = e;
        return v;
    }

    Val eval(ASTNode* node, State& s) {
        if (!node) return {};
        if (auto* value = dynamic_cast<ValueNode*>(node)) return evalLiteral(*value);
        if (auto* id = dynamic_cast<IdentifierNode*>(node)) {
            auto it = s.env.find(id->value);
            if (it != s.env.end()) {
                if (it->second.known()) return it->second;
                return unknown(it->second.sort, id->value);
            }
            return unknownOf(node, "'" + id->value + "' (a global or non-local name)");
        }
        if (auto* binary = dynamic_cast<BinaryExpressionNode*>(node)) return evalBinary(*binary, s);
        if (auto* unary = dynamic_cast<UnaryExpressionNode*>(node)) return evalUnary(*unary, s);
        if (auto* call = dynamic_cast<FunctionCallNode*>(node)) return evalCall(*call, s);
        if (auto* ifNode = dynamic_cast<IfStatementNode*>(node)) return evalIfExpression(*ifNode, s);
        if (dynamic_cast<MethodCallNode*>(node)) return unknownOf(node, "method call '" + exprText(node) + "'");
        return unknownOf(node, "expression '" + exprText(node) + "'");
    }

    Val evalLiteral(ValueNode& value) {
        auto type = value.getType();
        const Sort sort = sortOf(type.get());
        if (value.valueType == TokenType::BOOLEAN_LITERAL) return boolVal(ctx.bool_val(value.value == "true"));
        if (value.valueType == TokenType::CHAR_LITERAL) {
            unsigned char ch = 0;
            const std::string& t = value.value;
            if (t.size() >= 3) {
                if (t[1] == '\\' && t.size() >= 4) {
                    switch (t[2]) {
                        case 'n': ch = '\n'; break;
                        case 't': ch = '\t'; break;
                        case 'r': ch = '\r'; break;
                        case '0': ch = '\0'; break;
                        default: ch = static_cast<unsigned char>(t[2]); break;
                    }
                }
                else {
                    ch = static_cast<unsigned char>(t[1]);
                }
            }
            return intVal(num(static_cast<signed char>(ch), 8), 8);
        }
        if (value.valueType == TokenType::INTEGER_LITERAL || value.valueType == TokenType::HEX_LITERAL ||
            value.valueType == TokenType::BINARY_LITERAL || value.valueType == TokenType::OCTAL_LITERAL) {
            if (!sort.supported() || sort.isBool) return {};
            std::string text = value.value;
            int base = 10;
            if (value.valueType == TokenType::HEX_LITERAL) { base = 16; text = text.substr(2); }
            else if (value.valueType == TokenType::BINARY_LITERAL) { base = 2; text = text.substr(2); }
            else if (value.valueType == TokenType::OCTAL_LITERAL) { base = 8; text = text.substr(2); }
            errno = 0;
            const unsigned long long parsed = std::strtoull(text.c_str(), nullptr, base);
            if (errno == ERANGE) return unknown(sort, "literal");
            // The literal's bits, read as a signed value of its type.
            long long signedValue = static_cast<long long>(parsed);
            if (sort.width < 64) {
                const unsigned long long mask = (1ULL << sort.width) - 1;
                const unsigned long long bits = parsed & mask;
                signedValue = (bits >> (sort.width - 1)) & 1 ? static_cast<long long>(bits) - (1LL << sort.width)
                                                             : static_cast<long long>(bits);
            }
            return intVal(num(signedValue, sort.width), sort.width);
        }
        unsupported("literal " + value.value);
        return unknown(sort, "literal");
    }

    void sideCheck(const State& s, const z3::expr& guard, const z3::expr& property, const std::string& message,
                   bool asError = true) {
        if (suppressSideChecks) return;
        State guarded = s;
        guarded.pc = s.pc && guard;
        check(guarded, property, currentLine, message, asError);
    }

    Val evalBinary(BinaryExpressionNode& node, State& s) {
        const std::string& op = node.op;
        // Short-circuit: the right side's checks only apply where it is evaluated.
        if (op == "and" || op == "or" || op == "implies") {
            Val left = eval(node.left.get(), s);
            if (!left.known() || !left.sort.isBool) return unknownOf(&node, "condition '" + exprText(&node) + "'");
            State rightState = s;
            rightState.pc = s.pc && (op == "or" ? !*left.e : *left.e);
            Val right = eval(node.right.get(), rightState);
            if (!right.known() || !right.sort.isBool) return unknown({true, 0}, "cond");
            if (op == "and") return boolVal(*left.e && *right.e);
            if (op == "or") return boolVal(*left.e || *right.e);
            return boolVal(z3::implies(*left.e, *right.e));
        }
        Val left = eval(node.left.get(), s);
        Val right = eval(node.right.get(), s);
        auto resultType = node.getType();
        const Sort resultSort = sortOf(resultType.get());
        if (!left.known() || !right.known()) return unknown(resultSort, "value");

        if (left.sort.isBool && right.sort.isBool) {
            if (op == "==" || op == "iff") return boolVal(*left.e == *right.e);
            if (op == "!=" || op == "xor" || op == "^") return boolVal(*left.e != *right.e);
            if (op == "&") return boolVal(*left.e && *right.e);
            if (op == "|") return boolVal(*left.e || *right.e);
            return unknownOf(&node, "operator '" + op + "' on Bool");
        }
        if (left.sort.isBool || right.sort.isBool) return unknownOf(&node, "operator '" + op + "'");

        const unsigned common = left.sort.width > right.sort.width ? left.sort.width : right.sort.width;
        const z3::expr a = widen(left, common);
        const z3::expr b = widen(right, common);
        const std::string text = exprText(&node);

        if (op == "==") return boolVal(a == b);
        if (op == "!=") return boolVal(a != b);
        if (op == "<") return boolVal(a < b);    // signed bit-vector comparison
        if (op == "<=") return boolVal(a <= b);
        if (op == ">") return boolVal(a > b);
        if (op == ">=") return boolVal(a >= b);

        const unsigned width = resultSort.supported() && !resultSort.isBool ? resultSort.width : common;
        const z3::expr x = resize(a, common, width);
        const z3::expr y = resize(b, common, width);
        const bool wrapReport = !intMode() && verifierOptions().reportOverflow;
        const std::string overflowText = "'" + text + "' may overflow " + sortName({false, width});
        const z3::expr always = ctx.bool_val(true);
        if (op == "+") {
            if (wrapReport) sideCheck(s, always, z3::bvadd_no_overflow(x, y, true) && z3::bvadd_no_underflow(x, y), overflowText);
            checkRange(s, always, x + y, width, text);
            return intVal(x + y, width);
        }
        if (op == "-") {
            if (wrapReport) sideCheck(s, always, z3::bvsub_no_overflow(x, y) && z3::bvsub_no_underflow(x, y, true), overflowText);
            checkRange(s, always, x - y, width, text);
            return intVal(x - y, width);
        }
        if (op == "*") {
            if (wrapReport) sideCheck(s, always, z3::bvmul_no_overflow(x, y, true) && z3::bvmul_no_underflow(x, y), overflowText);
            checkRange(s, always, x * y, width, text);
            return intVal(x * y, width);
        }
        if (op == "/" || op == "%") {
            const z3::expr zero = num(0, width);
            sideCheck(s, always, y != zero, "possible division by zero in '" + text + "'");
            // MIN / -1 does not fit the type (undefined behaviour in the compiled code).
            sideCheck(s, y != zero, !(x == num(minOf(width), width) && y == num(-1, width)),
                      "'" + text + "' overflows when dividing the minimum " + sortName({false, width}) + " by -1");
            return intVal(op == "/" ? divide(x, y) : remainder(x, y), width);
        }
        if (op == "&") return intVal(fromBits(toBits(x, width) & toBits(y, width)), width);
        if (op == "|") return intVal(fromBits(toBits(x, width) | toBits(y, width)), width);
        if (op == "^" || op == "xor") return intVal(fromBits(toBits(x, width) ^ toBits(y, width)), width);
        if (op == "<<" || op == ">>") {
            const z3::expr inBounds = intMode()
                ? (y >= ctx.int_val(0) && y < ctx.int_val(static_cast<int>(width)))
                : z3::ult(y, ctx.bv_val(static_cast<uint64_t>(width), width));
            sideCheck(s, always, inBounds, "shift amount in '" + text + "' may be negative or at least " + std::to_string(width));
            const z3::expr bits = toBits(x, width);
            const z3::expr amount = toBits(y, width);
            return intVal(fromBits(op == "<<" ? z3::shl(bits, amount) : z3::ashr(bits, amount)), width);
        }
        return unknownOf(&node, "operator '" + op + "'");
    }

    Val evalUnary(UnaryExpressionNode& node, State& s) {
        Val v = eval(node.expression.get(), s);
        if (!v.known()) return unknownOf(&node, "expression '" + exprText(&node) + "'");
        if (node.op == "!" && v.sort.isBool) return boolVal(!*v.e);
        if (node.op == "+" && !v.sort.isBool) return v;
        if (node.op == "-" && !v.sort.isBool) {
            if (intMode() || verifierOptions().reportOverflow) {
                sideCheck(s, ctx.bool_val(true), *v.e != num(minOf(v.sort.width), v.sort.width),
                          "'" + exprText(&node) + "' overflows for the minimum " + sortName(v.sort));
            }
            return intVal(-*v.e, v.sort.width);
        }
        if (node.op == "~" && !v.sort.isBool) return intVal(fromBits(~toBits(*v.e, v.sort.width)), v.sort.width);
        return unknownOf(&node, "operator '" + node.op + "'");
    }

    Val evalIfExpression(IfStatementNode& node, State& s) {
        auto single = [](BlockNode* block) -> ASTNode* {
            if (!block || block->statements.size() != 1) return nullptr;
            ASTNode* only = block->statements[0].get();
            if (dynamic_cast<ReturnStatementNode*>(only) || dynamic_cast<VariableDeclarationNode*>(only)) return nullptr;
            return only;
        };
        ASTNode* thenExpr = single(node.thenBlock.get());
        ASTNode* elseExpr = single(node.elseBlock.get());
        Val c = eval(node.condition.get(), s);
        if (!thenExpr || !elseExpr || !c.known() || !c.sort.isBool) return unknownOf(&node, "if-expression");
        State thenState = s;
        thenState.pc = s.pc && *c.e;
        State elseState = s;
        elseState.pc = s.pc && !*c.e;
        Val a = eval(thenExpr, thenState);
        Val b = eval(elseExpr, elseState);
        if (!a.known() || !b.known() || a.sort.isBool != b.sort.isBool) return unknownOf(&node, "if-expression");
        if (a.sort.isBool) return boolVal(z3::ite(*c.e, *a.e, *b.e));
        const unsigned width = a.sort.width > b.sort.width ? a.sort.width : b.sort.width;
        return intVal(z3::ite(*c.e, widen(a, width), widen(b, width)), width);
    }

    FunctionDeclarationNode* resolveCallee(FunctionCallNode& call) {
        auto it = program.functions.find(call.functionName);
        if (it == program.functions.end()) return nullptr;
        for (auto* candidate : it->second) {
            if (candidate->parameters.size() == call.arguments.size()) {
                if (it->second.size() == 1) return candidate;
                // Overloads: match the argument sorts exactly.
                bool matches = true;
                for (size_t i = 0; i < call.arguments.size() && matches; ++i) {
                    auto* p = dynamic_cast<ParameterNode*>(candidate->parameters[i].get());
                    auto argType = call.arguments[i] ? call.arguments[i]->getType() : nullptr;
                    matches = p && sortOf(p->type.get()) == sortOf(argType.get());
                }
                if (matches) return candidate;
            }
        }
        return nullptr;
    }

    Val evalCall(FunctionCallNode& call, State& s) {
        if (call.functionName == "assert") return unknown({}, "assert");
        FunctionDeclarationNode* callee = resolveCallee(call);
        std::vector<Val> arguments;
        for (auto& argument : call.arguments) arguments.push_back(eval(argument.get(), s));
        auto callType = call.getType();
        Sort resultSort = sortOf(callType.get());
        if (!callee) return unknownOf(&call, "call to '" + call.functionName + "'");
        if (callee->returnType) resultSort = sortOf(callee->returnType.get());
        Val result = unknown(resultSort, call.functionName);
        if (!callee->contracts) return result;

        // Evaluate the callee's clauses with its parameters bound to the arguments.
        State calleeState(ctx);
        calleeState.pc = s.pc;
        for (size_t i = 0; i < callee->parameters.size(); ++i) {
            auto* p = dynamic_cast<ParameterNode*>(callee->parameters[i].get());
            if (!p) continue;
            const Sort sort = sortOf(p->type.get());
            Val bound;
            bound.sort = sort;
            if (i < arguments.size()) {
                checkNarrowing(s, arguments[i], sort, "argument " + std::to_string(i + 1) + " of '" + callee->name + "'");
                if (auto converted = convert(arguments[i], sort)) bound.e = *converted;
            }
            if (!bound.e && sort.supported()) bound.e = fresh(sort, p->name);
            calleeState.env[p->name] = bound;
        }
        for (auto& clause : callee->contracts->requiresClauses) {
            Val c = evalContract(clause.expression.get(), calleeState);
            if (!c.known() || !c.sort.isBool || suppressSideChecks) continue;
            check(s, *c.e, currentLine,
                  "call '" + exprText(&call) + "' may violate the requirement '" + clause.text + "' of '" +
                  callee->name + "'");
        }
        calleeState.env["result"] = result;
        for (auto& clause : callee->contracts->ensuresClauses) {
            Val c = evalContract(clause.expression.get(), calleeState);
            if (c.known() && c.sort.isBool) s.pc = s.pc && *c.e;
        }
        return result;
    }

    // ---- statements ----

    void declare(State& s, const std::string& name, const Val& value) {
        if (!s.frames.empty()) {
            auto it = s.env.find(name);
            s.frames.back().saved.push_back({name, it == s.env.end() ? std::nullopt : std::optional<Val>(it->second)});
        }
        s.env[name] = value;
    }

    void assign(State& s, const std::string& name, const Val& value) {
        auto it = s.env.find(name);
        if (it == s.env.end()) {
            unsupported("assignment to '" + name + "'");
            return;
        }
        checkNarrowing(s, value, it->second.sort, "the value assigned to '" + name + "'");
        it = s.env.find(name);
        Val stored;
        stored.sort = it->second.sort;
        if (auto converted = convert(value, it->second.sort)) stored.e = *converted;
        else if (stored.sort.supported()) stored.e = fresh(stored.sort, name);
        it->second = stored;
    }

    void havoc(State& s, const std::set<std::string>& names) {
        for (const auto& name : names) {
            auto it = s.env.find(name);
            if (it == s.env.end()) continue;
            if (it->second.sort.supported()) it->second.e = fresh(it->second.sort, name);
        }
    }

    std::vector<State> execBlock(BlockNode* block, std::vector<State> states) {
        if (!block) return states;
        for (auto& s : states) s.frames.push_back({});
        for (auto& statement : block->statements) {
            std::vector<State> next;
            for (auto& s : states) {
                auto produced = execStatement(statement.get(), s);
                for (auto& p : produced) next.push_back(std::move(p));
            }
            states = std::move(next);
            if (states.empty()) break;
            if (states.size() > 256) {
                if (!tooManyPaths) {
                    warning(currentLine, "'" + function.name + "' has too many paths; only the first 256 were verified");
                    tooManyPaths = true;
                }
                states.resize(256, State(ctx));
            }
        }
        for (auto& s : states) {
            Frame frame = std::move(s.frames.back());
            s.frames.pop_back();
            for (auto it = frame.saved.rbegin(); it != frame.saved.rend(); ++it) {
                if (it->second) s.env[it->first] = *it->second;
                else s.env.erase(it->first);
            }
        }
        return states;
    }

    void checkEnsures(State& s, std::optional<Val> result, int line) {
        if (!function.contracts || function.contracts->ensuresClauses.empty()) return;
        State post = s;
        if (result) {
            Val bound;
            bound.sort = returnSort;
            if (auto converted = convert(*result, returnSort)) bound.e = *converted;
            else if (returnSort.supported()) bound.e = fresh(returnSort, "result");
            post.env["result"] = bound;
        }
        for (auto& clause : function.contracts->ensuresClauses) {
            Val c = evalContract(clause.expression.get(), post);
            if (!c.known() || !c.sort.isBool) {
                note(clause.line, "ensures '" + clause.text + "' uses values the verifier does not model; it was not checked");
                continue;
            }
            check(post, *c.e, line, "'" + function.name + "' may return without satisfying 'ensures " + clause.text + "'");
        }
    }

    std::vector<State> execStatement(ASTNode* statement, State& s) {
        if (!statement) return {s};
        if (statement->line > 0) currentLine = statement->line;

        if (auto* ret = dynamic_cast<ReturnStatementNode*>(statement)) {
            std::optional<Val> value;
            if (ret->expression) {
                value = eval(ret->expression.get(), s);
                checkNarrowing(s, *value, returnSort, "the returned value");
            }
            checkEnsures(s, value, currentLine);
            return {};
        }
        if (auto* decl = dynamic_cast<VariableDeclarationNode*>(statement)) {
            const Sort sort = sortOf(decl->type.get());
            Val value;
            value.sort = sort;
            if (decl->initializer) {
                Val init = eval(decl->initializer.get(), s);
                checkNarrowing(s, init, sort, "the initial value of '" + decl->name + "'");
                if (auto converted = convert(init, sort)) value.e = *converted;
            }
            if (!value.e && sort.supported()) value.e = fresh(sort, decl->name);
            declare(s, decl->name, value);
            return {s};
        }
        if (auto* assignment = dynamic_cast<AssignmentExpressionNode*>(statement)) {
            auto* target = dynamic_cast<IdentifierNode*>(assignment->left.get());
            if (!target) {
                eval(assignment->right.get(), s);
                unsupported("assignment to '" + exprText(assignment->left.get()) + "'");
                return {s};
            }
            Val value = eval(assignment->right.get(), s);
            if (assignment->op.size() == 2 && assignment->op[1] == '=' && assignment->op != "==") {
                // `x op= v` is `x = x op v`.
                BinaryExpressionNode combined(std::make_unique<IdentifierNode>(target->value), assignment->op.substr(0, 1),
                                              assignment->right->clone());
                value = eval(&combined, s);
            }
            assign(s, target->value, value);
            return {s};
        }
        if (auto* postfix = dynamic_cast<PostfixExpressionNode*>(statement)) {
            if (auto* target = dynamic_cast<IdentifierNode*>(postfix->expression.get())) {
                Val current = eval(target, s);
                if (current.known() && !current.sort.isBool) {
                    const z3::expr one = num(1, current.sort.width);
                    const z3::expr next = postfix->op == "++" ? *current.e + one : *current.e - one;
                    checkRange(s, ctx.bool_val(true), next, current.sort.width, exprText(statement));
                    assign(s, target->value, intVal(next, current.sort.width));
                    return {s};
                }
                assign(s, target->value, Val{});
            }
            return {s};
        }
        if (auto* call = dynamic_cast<FunctionCallNode*>(statement)) {
            if (call->functionName == "assert" && !call->arguments.empty()) {
                Val c = eval(call->arguments[0].get(), s);
                if (!c.known() || !c.sort.isBool) {
                    note(currentLine, "assert(" + exprText(call->arguments[0].get()) + ") uses values the verifier does not model; it was not checked");
                    return {s};
                }
                check(s, *c.e, currentLine, "assertion '" + exprText(call->arguments[0].get()) + "' may fail");
                s.pc = s.pc && *c.e;  // after the assert, its condition holds
                return {s};
            }
            evalCall(*call, s);
            return {s};
        }
        if (auto* block = dynamic_cast<BlockNode*>(statement)) {
            return execBlock(block, {s});
        }
        if (auto* ifNode = dynamic_cast<IfStatementNode*>(statement)) {
            return execIf(*ifNode, s);
        }
        if (auto* loop = dynamic_cast<WhileStatementNode*>(statement)) {
            return execLoop(*statement, loop->condition.get(), loop->body.get(), s);
        }
        if (auto* loop = dynamic_cast<ForStatementNode*>(statement)) {
            if (loop->isRange && !loop->tokenStream) return execRangeFor(*loop, s);
            std::set<std::string> modified;
            collectAssigned(loop->body.get(), modified);
            havoc(s, modified);
            unsupported("a for loop over a collection (line " + std::to_string(currentLine) + ")");
            return {s};
        }
        if (dynamic_cast<FunctionDeclarationNode*>(statement)) {
            return {s};  // nested functions are verified on their own
        }
        // Other statements: evaluate for their checks when they are expressions, and forget every
        // variable they may assign.
        std::set<std::string> modified;
        collectAssigned(statement, modified);
        havoc(s, modified);
        if (dynamic_cast<BinaryExpressionNode*>(statement) || dynamic_cast<UnaryExpressionNode*>(statement)) {
            eval(statement, s);
        }
        else if (!dynamic_cast<MethodCallNode*>(statement) && !dynamic_cast<ValueNode*>(statement)) {
            unsupported("statement '" + exprText(statement) + "'");
        }
        return {s};
    }

    std::vector<State> execIf(IfStatementNode& node, State& s) {
        Val c = eval(node.condition.get(), s);
        if (!c.known() || !c.sort.isBool) {
            std::set<std::string> modified;
            collectAssigned(&node, modified);
            State thenState = s;
            State elseState = s;
            auto out = execBlock(node.thenBlock.get(), {thenState});
            auto elseOut = node.elseBlock ? execBlock(node.elseBlock.get(), {elseState}) : std::vector<State>{elseState};
            for (auto& e : elseOut) out.push_back(std::move(e));
            return out;
        }
        auto& facts = branchFacts[&node];
        if (facts.line == 0) {
            facts.line = currentLine;
            facts.text = exprText(node.condition.get());
            facts.hasElse = node.elseBlock != nullptr;
            branchOrder.push_back(&node);
        }
        std::vector<State> out;
        State thenState = s;
        thenState.pc = s.pc && *c.e;
        if (satisfiable(thenState.pc)) {
            facts.thenPossible = true;
            out = execBlock(node.thenBlock.get(), {thenState});
        }
        State elseState = s;
        elseState.pc = s.pc && !*c.e;
        if (satisfiable(elseState.pc)) {
            facts.elsePossible = true;
            auto elseOut = node.elseBlock ? execBlock(node.elseBlock.get(), {elseState}) : std::vector<State>{elseState};
            for (auto& e : elseOut) out.push_back(std::move(e));
        }
        return out;
    }

    void reportBranchFacts() {
        for (ASTNode* node : branchOrder) {
            const auto& facts = branchFacts[node];
            auto* ifNode = static_cast<IfStatementNode*>(node);
            if (dynamic_cast<ValueNode*>(ifNode->condition.get())) continue;  // `if (true)` is deliberate
            if (!facts.thenPossible && facts.elsePossible) {
                warning(facts.line, "condition '" + facts.text + "' is always false here; its branch never runs");
            }
            else if (facts.thenPossible && !facts.elsePossible) {
                warning(facts.line, "condition '" + facts.text + "' is always true here" +
                                    std::string(facts.hasElse ? "; the else branch never runs" : ""));
            }
        }
    }

    // A loop cut by its invariants: [init] while (cond) invariant I decreases D { body; [step] }
    std::vector<State> execLoop(ASTNode& loopNode, ASTNode* conditionNode, BlockNode* body, State& s) {
        const int loopLine = currentLine;
        ContractClauses* contracts = loopNode.contracts.get();

        auto evalInvariants = [&](State& state, std::vector<Invariant>& out) {
            if (!contracts) return;
            for (auto& clause : contracts->invariants) {
                Val v = evalContract(clause.expression.get(), state);
                if (v.known() && v.sort.isBool) out.push_back({*v.e, clause.text, clause.line});
                else note(clause.line, "invariant '" + clause.text + "' uses values the verifier does not model");
            }
        };

        // 1. The invariants hold on entry.
        {
            std::vector<Invariant> entry;
            evalInvariants(s, entry);
            for (auto& inv : entry) {
                check(s, inv.condition, inv.line, "loop invariant '" + inv.text + "' may not hold when the loop is entered");
            }
        }

        // 2. An arbitrary iteration: forget what the loop modifies, assume the invariants.
        std::set<std::string> modified;
        collectAssigned(body, modified);
        State head = s;
        havoc(head, modified);
        std::vector<Invariant> invariants;
        evalInvariants(head, invariants);
        for (auto& inv : invariants) head.pc = head.pc && inv.condition;

        Val c = eval(conditionNode, head);
        if (!c.known() || !c.sort.isBool) {
            unsupported("loop condition '" + exprText(conditionNode) + "'");
            c = unknown({true, 0}, "loopcond");
        }

        if ((!contracts || contracts->invariants.empty()) && !modified.empty()) {
            std::string list;
            for (const auto& name : modified) list += (list.empty() ? "" : ", ") + name;
            if (!list.empty()) {
                note(loopLine, "this loop has no invariant, so after it nothing is known about " + list);
            }
        }

        State bodyState = head;
        bodyState.pc = head.pc && *c.e;
        std::optional<z3::expr> measure;
        std::string measureText;
        if (contracts && !contracts->decreases.empty()) {
            auto& clause = contracts->decreases.front();
            measureText = clause.text;
            Val d = evalContract(clause.expression.get(), bodyState);
            if (d.known() && !d.sort.isBool) {
                measure = widen(d, 64);
                check(bodyState, *measure >= num(0, 64), clause.line,
                      "'decreases " + clause.text + "' may be negative at the start of an iteration");
            }
            else {
                note(clause.line, "'decreases " + clause.text + "' uses values the verifier does not model");
            }
        }
        if (satisfiable(bodyState.pc)) {
            auto ends = execBlock(body, {bodyState});
            for (auto& end : ends) {
                std::vector<Invariant> after;
                evalInvariants(end, after);
                for (auto& inv : after) {
                    check(end, inv.condition, inv.line, "loop invariant '" + inv.text + "' may not be preserved by an iteration");
                }
                if (measure) {
                    Val d = evalContract(contracts->decreases.front().expression.get(), end);
                    if (d.known() && !d.sort.isBool) {
                        check(end, widen(d, 64) < *measure, contracts->decreases.front().line,
                              "'decreases " + measureText + "' may not decrease in an iteration (the loop may not terminate)");
                    }
                }
            }
        }

        // 3. After the loop: the invariants hold and the condition is false.
        State exit = head;
        exit.pc = head.pc && !*c.e;
        if (!satisfiable(exit.pc)) return {};
        return {exit};
    }

    std::vector<State> execRangeFor(ForStatementNode& loop, State& s) {
        auto* range = dynamic_cast<RangeExpressionNode*>(loop.iterableExpr.get());
        if (!range) return {s};
        Val first = eval(range->startExpr.get(), s);
        Val last = eval(range->endExpr.get(), s);
        if (!first.known() || first.sort.isBool || !last.known() || last.sort.isBool) {
            std::set<std::string> modified;
            collectAssigned(loop.body.get(), modified);
            havoc(s, modified);
            unsupported("range loop bounds '" + exprText(range->startExpr.get()) + "'");
            return {s};
        }
        // Mirrors the compiled loop: i = start; end' = end (to) or end - 1 (until);
        // while (i <= end') { body; i = i + 1 }, with i of the start's type.
        const unsigned width = first.sort.width;
        const z3::expr start = *first.e;
        // The end bound is converted to the counter's type, as the compiled loop does.
        checkNarrowing(s, last, Sort{false, width}, "the range end '" + exprText(range->endExpr.get()) + "'");
        z3::expr bound = last.sort.width == width ? *last.e : resize(*last.e, last.sort.width, width);
        if (!range->isInclusive) {
            checkRange(s, ctx.bool_val(true), bound - num(1, width), width, exprText(range->endExpr.get()) + " - 1");
            bound = bound - num(1, width);
        }

        const std::string counter = loop.variable;
        State entry = s;
        entry.frames.push_back({});
        declare(entry, counter, intVal(start, width));
        auto conditionFn = std::function<Val(State&)>([&](State& state) {
            return boolVal(*state.env[counter].e <= bound);
        });
        auto stepFn = std::function<void(State&)>([&](State& state) {
            Val current = state.env[counter];
            state.env[counter] = intVal(*current.e + num(1, width), width);
        });
        auto results = execLoopWithCounter(loop, entry, counter, width, start, conditionFn, stepFn);
        for (auto& r : results) {
            Frame frame = std::move(r.frames.back());
            r.frames.pop_back();
            for (auto it = frame.saved.rbegin(); it != frame.saved.rend(); ++it) {
                if (it->second) r.env[it->first] = *it->second;
                else r.env.erase(it->first);
            }
        }
        return results;
    }

    // The range loop with an implicit invariant `counter >= start`, checked like the user's (so a
    // counter that wraps around at the type's maximum is reported).
    std::vector<State> execLoopWithCounter(ForStatementNode& loop, State& entry, const std::string& counter, unsigned width,
                                           const z3::expr& start, const std::function<Val(State&)>& conditionFn,
                                           const std::function<void(State&)>& stepFn) {
        const int loopLine = currentLine;
        ContractClauses* contracts = loop.contracts.get();
        auto invariantsOf = [&](State& state, std::vector<Invariant>& out) {
            out.push_back({*state.env[counter].e >= start, counter + " >= start of the range", loopLine});
            if (!contracts) return;
            for (auto& clause : contracts->invariants) {
                Val v = evalContract(clause.expression.get(), state);
                if (v.known() && v.sort.isBool) out.push_back({*v.e, clause.text, clause.line});
                else note(clause.line, "invariant '" + clause.text + "' uses values the verifier does not model");
            }
        };
        {
            std::vector<Invariant> atEntry;
            invariantsOf(entry, atEntry);
            for (auto& inv : atEntry) {
                check(entry, inv.condition, inv.line, "loop invariant '" + inv.text + "' may not hold when the loop is entered");
            }
        }
        std::set<std::string> modified;
        collectAssigned(loop.body.get(), modified);
        modified.insert(counter);
        State head = entry;
        havoc(head, modified);
        std::vector<Invariant> invariants;
        invariantsOf(head, invariants);
        for (auto& inv : invariants) head.pc = head.pc && inv.condition;
        if ((!contracts || contracts->invariants.empty())) {
            std::string list;
            for (const auto& name : modified) if (name != counter) list += (list.empty() ? "" : ", ") + name;
            if (!list.empty()) note(loopLine, "this loop has no invariant, so after it nothing is known about " + list);
        }
        Val c = conditionFn(head);
        State bodyState = head;
        bodyState.pc = head.pc && *c.e;
        std::optional<z3::expr> measure;
        if (contracts && !contracts->decreases.empty()) {
            Val d = evalContract(contracts->decreases.front().expression.get(), bodyState);
            if (d.known() && !d.sort.isBool) {
                measure = widen(d, 64);
                check(bodyState, *measure >= num(0, 64), contracts->decreases.front().line,
                      "'decreases " + contracts->decreases.front().text + "' may be negative at the start of an iteration");
            }
        }
        if (satisfiable(bodyState.pc)) {
            auto ends = execBlock(loop.body.get(), {bodyState});
            for (auto& end : ends) {
                stepFn(end);
                std::vector<Invariant> after;
                invariantsOf(end, after);
                for (auto& inv : after) {
                    const std::string message = inv.text == counter + " >= start of the range"
                        ? "the loop counter '" + counter + "' may wrap around past the maximum " + sortName({false, width})
                        : "loop invariant '" + inv.text + "' may not be preserved by an iteration";
                    check(end, inv.condition, inv.line, message);
                }
                if (measure) {
                    Val d = evalContract(contracts->decreases.front().expression.get(), end);
                    if (d.known() && !d.sort.isBool) {
                        check(end, widen(d, 64) < *measure, contracts->decreases.front().line,
                              "'decreases " + contracts->decreases.front().text + "' may not decrease in an iteration (the loop may not terminate)");
                    }
                }
            }
        }
        State exit = head;
        exit.pc = head.pc && !*c.e;
        if (!satisfiable(exit.pc)) return {};
        return {exit};
    }
};

void collectFunctions(ASTNode* node, ProgramInfo& info) {
    if (auto* program = dynamic_cast<ProgramNode*>(node)) {
        for (auto& statement : program->statements) collectFunctions(statement.get(), info);
        return;
    }
    if (auto* function = dynamic_cast<FunctionDeclarationNode*>(node)) {
        info.functions[function->name].push_back(function);
    }
}

bool definedIn(const FunctionDeclarationNode& function, const std::string& mainFile) {
    const SourceLocation location = sourceLocationOf(function.line);
    if (location.file.empty()) return false;
    std::error_code ec;
    return std::filesystem::equivalent(location.file, mainFile, ec);
}

} // namespace

int verifyProgram(ASTNode* program, const std::string& mainFile) {
    ProgramInfo info;
    collectFunctions(program, info);
    z3::context ctx;
    std::vector<Diagnostic> diagnostics;
    int functionCount = 0;
    int obligations = 0;
    std::vector<std::string> verified;
    auto* root = dynamic_cast<ProgramNode*>(program);
    if (!root) return 1;
    for (auto& statement : root->statements) {
        auto* function = dynamic_cast<FunctionDeclarationNode*>(statement.get());
        if (!function || function->isExternal || !function->body || !definedIn(*function, mainFile)) continue;
        ++functionCount;
        const size_t before = diagnostics.size();
        FunctionVerifier verifier(ctx, info, *function, diagnostics);
        try {
            verifier.run();
        }
        catch (const z3::exception& e) {
            diagnostics.push_back({Severity::Warning, function->line,
                                   "verification of '" + function->name + "' stopped: " + std::string(e.msg()), ""});
        }
        obligations += verifier.obligations;
        bool hasError = false;
        for (size_t i = before; i < diagnostics.size(); ++i) hasError = hasError || diagnostics[i].severity == Severity::Error;
        if (!hasError && verifier.obligations > 0) {
            verified.push_back(function->name + " (" + std::to_string(verifier.obligations) + " check" +
                               (verifier.obligations == 1 ? "" : "s") + ")");
        }
    }

    std::stable_sort(diagnostics.begin(), diagnostics.end(),
                     [](const Diagnostic& a, const Diagnostic& b) { return a.line < b.line; });
    int errors = 0;
    int warnings = 0;
    for (const auto& d : diagnostics) {
        const char* label = d.severity == Severity::Error ? "error" : d.severity == Severity::Warning ? "warning" : "note";
        if (d.severity == Severity::Error) ++errors;
        if (d.severity == Severity::Warning) ++warnings;
        std::cout << locationText(d.line) << ": " << label << ": " << d.message << std::endl;
        if (!d.detail.empty()) std::cout << "    " << d.detail << std::endl;
    }
    for (const auto& name : verified) std::cout << "verified: " << name << std::endl;
    std::cout << "verify: " << functionCount << " function(s), " << obligations << " check(s), " << errors
              << " error(s), " << warnings << " warning(s)" << std::endl;
    return errors > 0 ? 1 : 0;
}

#endif
