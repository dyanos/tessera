//! Fast, compiler-independent problems found while typing: characters the
//! `tess` lexer rejects, unterminated strings and comments, and unbalanced
//! brackets.

use crate::lexer::{Tok, TokKind};
use std::ops::Range;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Severity {
    Error,
    Warning,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Problem {
    pub range: Range<usize>,
    pub severity: Severity,
    pub message: String,
    /// Stable id, e.g. `unknown-char`.
    pub code: &'static str,
}

pub fn lex_problems(src: &str, toks: &[Tok]) -> Vec<Problem> {
    let mut out = Vec::new();
    let mut stack: Vec<(char, Range<usize>)> = Vec::new();

    for t in toks {
        match t.kind {
            TokKind::Unknown => {
                let ch = t.text(src);
                let hint = match ch {
                    "?" => " (Tessera has no `?` operator; use `if (...) { ... } else { ... }`)",
                    "`" => "",
                    _ => "",
                };
                out.push(Problem {
                    range: t.range.clone(),
                    severity: Severity::Error,
                    message: format!("unknown token '{}'{}", ch.escape_debug(), hint),
                    code: "unknown-token",
                });
            }
            TokKind::Str | TokKind::Regex if !t.terminated => out.push(Problem {
                range: t.range.start..(t.range.start + 1).min(src.len()),
                severity: Severity::Error,
                message: "unterminated string literal".into(),
                code: "unterminated-string",
            }),
            TokKind::BlockComment | TokKind::DocComment if !t.terminated => out.push(Problem {
                range: t.range.start..(t.range.start + 2).min(src.len()),
                severity: Severity::Error,
                message: "unterminated block comment".into(),
                code: "unterminated-comment",
            }),
            TokKind::Op => {
                let w = t.text(src);
                match w {
                    "(" => stack.push(('(', t.range.clone())),
                    "[" | "[@" => stack.push(('[', t.range.clone())),
                    "{" => stack.push(('{', t.range.clone())),
                    ")" | "]" | "}" => {
                        let want = match w {
                            ")" => '(',
                            "]" => '[',
                            _ => '{',
                        };
                        match stack.last() {
                            Some((open, _)) if *open == want => {
                                stack.pop();
                            }
                            _ => {
                                // Recover: if the opener is further down, the ones above it are unclosed.
                                if let Some(pos) = stack.iter().rposition(|(o, _)| *o == want) {
                                    for (o, r) in stack.drain(pos + 1..) {
                                        out.push(unclosed(o, r));
                                    }
                                    stack.pop();
                                } else {
                                    out.push(Problem {
                                        range: t.range.clone(),
                                        severity: Severity::Error,
                                        message: format!("unmatched '{w}'"),
                                        code: "unbalanced",
                                    });
                                }
                            }
                        }
                    }
                    "&&" | "||" => {}
                    _ => {}
                }
            }
            _ => {}
        }
    }
    for (o, r) in stack {
        out.push(unclosed(o, r));
    }
    // `&&` / `||` lex as two operators and are not logical operators.
    let sig: Vec<&Tok> = toks.iter().filter(|t| !t.is_trivia()).collect();
    for w in sig.windows(2) {
        let (a, b) = (w[0].text(src), w[1].text(src));
        if (a == "&" && b == "&" || a == "|" && b == "|") && w[0].range.end == w[1].range.start {
            let word = if a == "&" { "and" } else { "or" };
            out.push(Problem {
                range: w[0].range.start..w[1].range.end,
                severity: Severity::Warning,
                message: format!("'{a}{b}' is not a logical operator in Tessera; use '{word}'"),
                code: "c-logical-operator",
            });
        }
    }
    out.sort_by_key(|p| p.range.start);
    out
}

fn unclosed(open: char, range: Range<usize>) -> Problem {
    let close = match open {
        '(' => ')',
        '[' => ']',
        _ => '}',
    };
    Problem {
        range,
        severity: Severity::Error,
        message: format!("'{open}' is never closed (expected '{close}')"),
        code: "unbalanced",
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lexer::tokenize;

    fn codes(src: &str) -> Vec<(&'static str, String)> {
        lex_problems(src, &tokenize(src)).into_iter().map(|p| (p.code, src[p.range].to_string())).collect()
    }

    #[test]
    fn finds_problems() {
        assert_eq!(codes("val x = a ` b;"), vec![("unknown-token", "`".into())]);
        assert_eq!(codes("def f() { (1 + 2 }"), vec![("unbalanced", "(".into())]);
        assert_eq!(codes("x)"), vec![("unbalanced", ")".into())]);
        assert_eq!(codes("val s = \"abc"), vec![("unterminated-string", "\"".into())]);
        assert_eq!(codes("if (a && b) {}"), vec![("c-logical-operator", "&&".into())]);
        assert!(codes("def f(): Int { val t = [1, 2]; return t[0]; } // ok ?").is_empty());
    }
}
