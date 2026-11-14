#pragma once

// Static verification of contracts (`csec++ --verify`): requires/ensures on functions,
// invariant/decreases on loops, and assert(...) statements, proved with the Z3 SMT solver.
// See LANGUAGE_SPEC.md §12.

#include <string>

class ASTNode;

struct VerifierOptions {
    // false (default): integers are mathematical integers, and every operation must provably stay in
    // its type's range (overflow is an error). true (--verify-wrap): integers are bit-vectors that wrap
    // around like the compiled program; overflow is reported only with reportOverflow.
    bool wrapAround = false;
    bool reportOverflow = false;
    // Per-query solver timeout.
    unsigned timeoutMilliseconds = 10000;
};

VerifierOptions& verifierOptions();

// The source-like text of an expression, for diagnostics (also used by assert's runtime message).
std::string verifierExpressionText(ASTNode* node);

// Verifies every function defined in `mainFile` (imported modules are only used for their
// contracts). Prints diagnostics; returns 0 when no error was found, 1 otherwise.
int verifyProgram(ASTNode* program, const std::string& mainFile);
