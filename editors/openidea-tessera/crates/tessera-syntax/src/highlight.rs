//! Token classification for syntax highlighting. Editor-neutral: the OpenIDEA
//! plugin maps [`Class`] to its `Highlight`, the language server to LSP
//! semantic token types.

use crate::lexer::{string_parts, StrPart, Tok, TokKind};
use std::ops::Range;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Class {
    Keyword,
    /// `true`, `false`, `null`, ALL_CAPS names.
    Constant,
    String,
    Escape,
    /// `${` / `}` of a string interpolation.
    Interpolation,
    Regex,
    Number,
    Comment,
    DocComment,
    /// The name in a `def`.
    FunctionDecl,
    FunctionCall,
    /// `ofMethod(` after a `.`.
    MethodCall,
    /// Built-in and user type names.
    Type,
    /// Name in a `class` / `struct` / `object` / DSL declaration.
    TypeDecl,
    /// `System`, `Console`, `Math`, `IO`, `File`, and builtin functions.
    Builtin,
    /// A parameter at its declaration.
    Parameter,
    /// Interpolated name, plain variables.
    Variable,
    Property,
    /// `this`, `super`.
    SelfRef,
    /// `[@Name`.
    Attribute,
    /// LaTeX `\command` inside `$...$`.
    Macro,
    Operator,
    Punctuation,
}

/// Built-in type names (`docs/grammar.md` §3 plus runtime types).
pub const BUILTIN_TYPES: &[&str] = &[
    "Int", "Long", "Short", "Byte", "Float", "Double", "Real", "Bool", "Boolean", "Char", "String",
    "Nat", "Integer", "Natural", "Unit", "Void", "Tensor", "Vector", "Matrix", "Array", "Dict", "Set",
    "Shared", "Weak", "Regex",
];

/// Namespaces / objects provided by the runtime.
pub const BUILTIN_NAMESPACES: &[&str] = &["System", "Console", "Math", "IO", "File"];

/// Words that are keywords only in context (`docs/grammar.md` §1.1, §4, §5).
pub const DSL_WORDS: &[&str] = &[
    "atom", "bond", "at", "steps", "dt", "temperature", "grid", "viscosity", "velocity", "euler",
    "from", "step", "lattice", "spacing", "chain", "mcmc",
];
pub const POLICY_WORDS: &[&str] = &["cpu", "simd", "openmp", "gpu"];
/// Infix word operators: `a inner b`, `xs union ys`, `0 to n step 2`, `t is keyword("if")`.
pub const INFIX_WORDS: &[&str] = &[
    "inner", "outer", "tensor", "union", "intersect", "diff", "implies", "step", "is",
];
pub const DSL_ENTRY: &[&str] = &["molecule", "cfd", "ode", "protein"];
/// Contract clauses (`docs/grammar.md`, LANGUAGE_SPEC.md §12): keywords only between a signature or
/// loop header and its body, e.g. `def f(x: Int): Int requires x > 0 ensures result > x = x + 1`.
/// `result` is the return value inside an `ensures` clause.
pub const CONTRACT_WORDS: &[&str] = &["requires", "ensures", "invariant", "decreases"];

const PUNCT: &[&str] = &["(", ")", "{", "}", "[", "]", ",", ";", ".", ":"];

pub fn is_builtin_type(w: &str) -> bool {
    BUILTIN_TYPES.contains(&w)
}

fn is_all_caps(w: &str) -> bool {
    w.chars().count() > 1
        && w.chars().any(|c| c.is_ascii_uppercase())
        && w.chars().all(|c| c.is_ascii_uppercase() || c.is_ascii_digit() || c == '_')
}

fn starts_upper(w: &str) -> bool {
    w.chars().next().is_some_and(|c| c.is_ascii_uppercase())
}

/// Does this token end an operand (so a following word is infix)?
fn ends_operand(src: &str, t: &Tok) -> bool {
    match t.kind {
        TokKind::Ident | TokKind::Int | TokKind::Float | TokKind::Str | TokKind::Char | TokKind::Bool => true,
        TokKind::Keyword => matches!(t.text(src), "this" | "null"),
        TokKind::Op => matches!(t.text(src), ")" | "]"),
        _ => false,
    }
}

/// Does this token start an operand?
fn starts_operand(src: &str, t: &Tok) -> bool {
    match t.kind {
        TokKind::Ident | TokKind::Int | TokKind::Float | TokKind::Str | TokKind::Char | TokKind::Bool | TokKind::Regex => true,
        TokKind::Keyword => matches!(t.text(src), "this" | "null" | "new" | "if" | "build"),
        TokKind::Op => matches!(t.text(src), "(" | "[" | "-" | "!" | "~" | "&" | "<-" | "$" | "$$"),
        _ => false,
    }
}

/// Classifies every token. Ranges never overlap and are sorted.
pub fn highlight(src: &str, toks: &[Tok]) -> Vec<(Range<usize>, Class)> {
    let mut out = Vec::with_capacity(toks.len());
    // Indices of significant (non-comment) tokens, for neighbour lookups.
    let sig: Vec<usize> = (0..toks.len()).filter(|&i| !toks[i].is_trivia()).collect();
    let text = |si: usize| toks[sig[si]].text(src);

    // Brace depth at which each open DSL block started; a `molecule`/`cfd`/...
    // keyword arms the next `{`.
    let mut brace_depth = 0usize;
    let mut dsl_stack: Vec<usize> = Vec::new();
    let mut dsl_armed = false;
    // `$ ... $` LaTeX region.
    let mut in_latex = false;
    // Parameter lists of `def`, `class`, `struct` and lambdas: (paren depth at open).
    let mut paren_depth = 0usize;
    let mut param_list_depth: Option<usize> = None;
    let mut def_armed = false;
    // Inside a contract clause (until the body's `{`, `=`, or a `;`): `result` is a keyword.
    let mut in_contract = false;

    let mut si = 0usize;
    for (i, t) in toks.iter().enumerate() {
        match t.kind {
            TokKind::LineComment | TokKind::BlockComment => {
                out.push((t.range.clone(), Class::Comment));
                continue;
            }
            TokKind::DocComment => {
                out.push((t.range.clone(), Class::DocComment));
                continue;
            }
            _ => {}
        }
        debug_assert_eq!(sig[si], i);
        let prev = si.checked_sub(1).map(|p| &toks[sig[p]]);
        let prev_text = si.checked_sub(1).map(text).unwrap_or("");
        let prev2_text = si.checked_sub(2).map(text).unwrap_or("");
        let next = sig.get(si + 1).map(|&n| &toks[n]);
        let next_text = next.map(|n| n.text(src)).unwrap_or("");
        let word = t.text(src);

        let class = match t.kind {
            TokKind::Str => {
                for (r, part) in string_parts(src, t.range.clone()) {
                    let c = match part {
                        StrPart::Text => Class::String,
                        StrPart::Escape => Class::Escape,
                        StrPart::InterpDelim => Class::Interpolation,
                        StrPart::InterpName => Class::Variable,
                    };
                    out.push((r, c));
                }
                si += 1;
                continue;
            }
            TokKind::Regex => Some(Class::Regex),
            TokKind::Char => Some(Class::String),
            TokKind::Int | TokKind::Float => Some(Class::Number),
            TokKind::Bool => Some(Class::Constant),
            TokKind::Keyword => {
                if DSL_ENTRY.contains(&word) {
                    dsl_armed = true;
                }
                if word == "def" || word == "class" || word == "struct" {
                    def_armed = true;
                }
                Some(match word {
                    "null" => Class::Constant,
                    "this" | "super" => Class::SelfRef,
                    _ => Class::Keyword,
                })
            }
            TokKind::Ident => {
                let in_dsl = !dsl_stack.is_empty();
                let after_dot = prev_text == ".";
                Some(if prev_text == "\\" && prev.is_some_and(|p| p.range.end == t.range.start) {
                    Class::Macro
                } else if in_latex {
                    Class::Variable
                } else if prev_text == "[@" {
                    Class::Attribute
                } else if after_dot {
                    if next_text == "(" { Class::MethodCall } else { Class::Property }
                } else if prev_text == "def" || (prev_text == "operator" && prev2_text == "def") {
                    Class::FunctionDecl
                } else if matches!(prev_text, "class" | "struct" | "object" | "typename")
                    || (DSL_ENTRY.contains(&prev_text) && prev.is_some_and(|p| p.kind == TokKind::Keyword))
                {
                    Class::TypeDecl
                } else if CONTRACT_WORDS.contains(&word)
                    && prev.is_some_and(|p| ends_operand(src, p))
                    && next.is_some_and(|n| starts_operand(src, n))
                {
                    in_contract = true;
                    Class::Keyword
                } else if in_contract && word == "result" {
                    Class::Keyword
                } else if in_dsl && DSL_WORDS.contains(&word) {
                    Class::Keyword
                } else if POLICY_WORDS.contains(&word)
                    && ((prev_text == "(" && matches!(prev2_text, "pmap" | "preduce"))
                        || (prev_text == "|" && POLICY_WORDS.contains(&prev2_text)))
                {
                    Class::Keyword
                } else if INFIX_WORDS.contains(&word)
                    && prev.is_some_and(|p| ends_operand(src, p))
                    && next.is_some_and(|n| starts_operand(src, n))
                {
                    Class::Keyword
                } else if is_builtin_type(word) {
                    Class::Type
                } else if BUILTIN_NAMESPACES.contains(&word) {
                    Class::Builtin
                } else if param_list_depth == Some(paren_depth) && next_text == ":" {
                    Class::Parameter
                } else if is_all_caps(word) {
                    Class::Constant
                } else if starts_upper(word) {
                    Class::Type
                } else if next_text == "(" && crate::docs::BUILTIN_FUNCTIONS.contains(&word) {
                    Class::Builtin
                } else if next_text == "(" {
                    Class::FunctionCall
                } else {
                    // Plain identifiers stay uncoloured.
                    si += 1;
                    continue;
                })
            }
            TokKind::Op => {
                match word {
                    "{" => {
                        in_contract = false;
                        brace_depth += 1;
                        if dsl_armed {
                            dsl_stack.push(brace_depth);
                            dsl_armed = false;
                        }
                        def_armed = false;
                    }
                    "}" => {
                        if dsl_stack.last() == Some(&brace_depth) {
                            dsl_stack.pop();
                        }
                        brace_depth = brace_depth.saturating_sub(1);
                    }
                    "(" => {
                        paren_depth += 1;
                        // `def f(`, `class C(`, `struct S(`, `def operator +(`, lambda `](`.
                        let lambda = prev_text == "]";
                        if (def_armed && param_list_depth.is_none()) || lambda {
                            param_list_depth = Some(paren_depth);
                        }
                    }
                    ")" => {
                        if param_list_depth == Some(paren_depth) {
                            param_list_depth = None;
                            def_armed = false;
                        }
                        paren_depth = paren_depth.saturating_sub(1);
                    }
                    ";" => {
                        def_armed = false;
                        in_contract = false;
                    }
                    "=" => in_contract = false,
                    "$" | "$$" => in_latex = !in_latex,
                    _ => {}
                }
                Some(match word {
                    "[@" => Class::Attribute,
                    "\\" if next.is_some_and(|n| n.kind == TokKind::Ident && n.range.start == t.range.end) => Class::Macro,
                    "$" | "$$" => Class::Interpolation,
                    w if PUNCT.contains(&w) => Class::Punctuation,
                    _ => Class::Operator,
                })
            }
            TokKind::Unknown => None,
            _ => None,
        };
        if let Some(c) = class {
            out.push((t.range.clone(), c));
        }
        si += 1;
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lexer::tokenize;

    fn classes(src: &str) -> Vec<(&str, Class)> {
        let toks = tokenize(src);
        highlight(src, &toks).into_iter().map(|(r, c)| (&src[r], c)).collect()
    }

    fn class_of(src: &str, word: &str) -> Option<Class> {
        classes(src).into_iter().find(|(w, _)| *w == word).map(|(_, c)| c)
    }

    #[test]
    fn declarations_and_calls() {
        let src = "def area(r: Double): Double { return PI * r ** 2 + sqrt(r); }";
        assert_eq!(class_of(src, "area"), Some(Class::FunctionDecl));
        assert_eq!(class_of(src, "r"), Some(Class::Parameter));
        assert_eq!(class_of(src, "Double"), Some(Class::Type));
        assert_eq!(class_of(src, "PI"), Some(Class::Constant));
        assert_eq!(class_of(src, "sqrt"), Some(Class::Builtin));
        assert_eq!(class_of("val y = myFn(2);", "myFn"), Some(Class::FunctionCall));
        assert_eq!(class_of(src, "return"), Some(Class::Keyword));
        assert_eq!(class_of(src, "**"), Some(Class::Operator));
    }

    #[test]
    fn contextual_words() {
        assert_eq!(class_of("val p = preduce(cpu|simd, a, x <- xs, 0) { a + x };", "simd"), Some(Class::Keyword));
        assert_eq!(class_of("val cpu = 1;", "cpu"), None);
        assert_eq!(class_of("val c = a inner b;", "inner"), Some(Class::Keyword));
        assert_eq!(class_of("val inner = 3;", "inner"), None);
        assert_eq!(class_of("molecule Water { atom O at (0, 0, 0) }", "atom"), Some(Class::Keyword));
        assert_eq!(class_of("molecule Water { atom O at (0, 0, 0) }", "Water"), Some(Class::TypeDecl));
        assert_eq!(class_of("val atom = 1;", "atom"), None);
    }

    #[test]
    fn contract_clauses() {
        let src = "def f(x: Int): Int requires x > 0 ensures result > x = x + 1;";
        assert_eq!(class_of(src, "requires"), Some(Class::Keyword));
        assert_eq!(class_of(src, "ensures"), Some(Class::Keyword));
        assert_eq!(class_of(src, "result"), Some(Class::Keyword));
        let lp = "while (i < n) invariant 0 <= i decreases n - i { i += 1; }";
        assert_eq!(class_of(lp, "invariant"), Some(Class::Keyword));
        assert_eq!(class_of(lp, "decreases"), Some(Class::Keyword));
        assert_eq!(class_of("val requires = 1;", "requires"), None);
        assert_eq!(class_of("val result = f(1);", "result"), None);
    }

    #[test]
    fn members_strings_attributes() {
        let src = r#"[@DllImport("m", "cos")] external def c(x: Double): Double; val s = "v=${x}\n"; t.x; t.len();"#;
        assert_eq!(class_of(src, "DllImport"), Some(Class::Attribute));
        assert_eq!(class_of(src, "len"), Some(Class::MethodCall));
        let cl = classes(src);
        assert!(cl.contains(&("${", Class::Interpolation)));
        assert!(cl.contains(&("\\n", Class::Escape)));
        assert!(cl.contains(&("x", Class::Property)));
    }

    #[test]
    fn no_overlaps() {
        let src = "class P(x: Int) { def operator +(o: P): P = new P(x + o.x); } // c\n$ \\frac{a}{b} $";
        let toks = tokenize(src);
        let hl = highlight(src, &toks);
        for w in hl.windows(2) {
            assert!(w[0].0.end <= w[1].0.start, "{:?}", w);
        }
        assert_eq!(class_of(src, "frac"), Some(Class::Macro));
    }
}
