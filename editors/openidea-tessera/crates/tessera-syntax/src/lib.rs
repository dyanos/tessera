//! Editor support for Tessera (`.csec`): a lossless lexer mirroring the
//! `csec++` lexer, highlighting classes, declarations/outline, folding and
//! lexical diagnostics. No dependencies, so it builds for both the OpenIDEA
//! plugin (wasm32-wasip2) and the native language server.

pub mod diagnostics;
pub mod docs;
pub mod folding;
pub mod highlight;
pub mod lexer;
pub mod outline;

pub use diagnostics::{lex_problems, Problem, Severity};
pub use folding::{folds, Fold, FoldKind, Lines};
pub use highlight::{highlight, Class};
pub use lexer::{tokenize, Tok, TokKind};
pub use outline::{declarations, Decl, DeclKind};

/// Everything an editor needs about one version of a document.
pub struct Analysis {
    pub tokens: Vec<Tok>,
    pub decls: Vec<Decl>,
}

impl Analysis {
    pub fn new(src: &str) -> Self {
        let tokens = tokenize(src);
        let decls = declarations(src, &tokens);
        Analysis { tokens, decls }
    }

    /// The token containing (or ending at) byte `offset`, preferring identifiers.
    pub fn token_at(&self, offset: usize) -> Option<&Tok> {
        let i = self.tokens.partition_point(|t| t.range.end < offset);
        let here = self.tokens.get(i).filter(|t| t.range.start <= offset);
        match here {
            Some(t) if t.range.end == offset => {
                // Between two tokens: prefer the next one if it starts here and is a word.
                match self.tokens.get(i + 1) {
                    Some(n) if n.range.start == offset && matches!(n.kind, TokKind::Ident | TokKind::Keyword) => Some(n),
                    _ => Some(t),
                }
            }
            other => other,
        }
    }

    /// Visible declarations named `name` at `offset`, innermost first.
    pub fn resolve(&self, name: &str, offset: usize) -> Vec<&Decl> {
        let mut v: Vec<&Decl> = self
            .decls
            .iter()
            .filter(|d| d.name == name && d.scope.start <= offset && offset <= d.scope.end)
            .collect();
        // Innermost = narrowest scope; then later declarations shadow earlier ones.
        v.sort_by_key(|d| (d.scope.end - d.scope.start, std::cmp::Reverse(d.name_range.start)));
        v
    }

    /// Declarations visible from outside the file (top level and members).
    pub fn exported(&self) -> impl Iterator<Item = &Decl> {
        self.decls.iter().filter(|d| {
            matches!(
                d.kind,
                DeclKind::Function
                    | DeclKind::External
                    | DeclKind::Class
                    | DeclKind::Struct
                    | DeclKind::Enum
                    | DeclKind::Interface
                    | DeclKind::Object
                    | DeclKind::Dsl
                    | DeclKind::Global
                    | DeclKind::Field
                    | DeclKind::Method
                    | DeclKind::Operator
            )
        })
    }
}
