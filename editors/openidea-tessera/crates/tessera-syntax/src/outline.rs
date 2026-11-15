//! Declarations in a Tessera document: functions, classes, structs, objects,
//! DSL blocks, fields, globals, parameters and locals. A forgiving
//! token-level scan (not the compiler's parser): it never fails and keeps
//! working on half-typed code.

use crate::lexer::{Tok, TokKind};
use std::ops::Range;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum DeclKind {
    Function,
    Method,
    /// `def operator +(...)`.
    Operator,
    /// `external def` (FFI import).
    External,
    Class,
    Struct,
    Object,
    Enum,
    Interface,
    /// `molecule` / `cfd` / `ode` / `protein` block with a name.
    Dsl,
    /// `val`/`var` in a class body, or a constructor parameter.
    Field,
    /// Top-level `val`/`var`.
    Global,
    /// `val`/`var` inside a function or block.
    Local,
    Parameter,
    /// Loop / comprehension binder (`for (i <- ...)`).
    Binder,
    TypeParameter,
}

impl DeclKind {
    /// Shown in the structure view / document symbols.
    pub fn in_outline(self) -> bool {
        !matches!(self, DeclKind::Local | DeclKind::Parameter | DeclKind::Binder | DeclKind::TypeParameter)
    }

    pub fn is_type(self) -> bool {
        matches!(self, DeclKind::Class | DeclKind::Struct | DeclKind::Object | DeclKind::Enum | DeclKind::Interface | DeclKind::TypeParameter)
    }

    pub fn is_callable(self) -> bool {
        matches!(self, DeclKind::Function | DeclKind::Method | DeclKind::Operator | DeclKind::External)
    }

    pub fn label(self) -> &'static str {
        match self {
            DeclKind::Function => "function",
            DeclKind::Method => "method",
            DeclKind::Operator => "operator",
            DeclKind::External => "external function",
            DeclKind::Class => "class",
            DeclKind::Struct => "struct",
            DeclKind::Object => "object",
            DeclKind::Enum => "enum",
            DeclKind::Interface => "interface",
            DeclKind::Dsl => "simulation block",
            DeclKind::Field => "field",
            DeclKind::Global => "global",
            DeclKind::Local => "local",
            DeclKind::Parameter => "parameter",
            DeclKind::Binder => "loop variable",
            DeclKind::TypeParameter => "type parameter",
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Decl {
    pub name: String,
    pub kind: DeclKind,
    /// The name token.
    pub name_range: Range<usize>,
    /// The whole declaration (for functions/classes: through the closing `}`).
    pub range: Range<usize>,
    /// Where the name is visible.
    pub scope: Range<usize>,
    /// Enclosing declaration (index into the same list, always smaller).
    pub parent: Option<usize>,
    /// Header text, whitespace-collapsed: `def f(x: Int): Int`, `val n: Int = 3`.
    pub signature: String,
    /// `//` / `///` / `/** */` comment lines directly above the declaration.
    pub doc: Option<String>,
    /// `var` (mutable) vs `val`.
    pub mutable: bool,
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum Frame {
    /// A plain block.
    Block,
    /// The body of declaration `usize`.
    Body(usize),
}

struct Scanner<'a> {
    src: &'a str,
    toks: &'a [Tok],
    /// Indices of significant tokens.
    sig: Vec<usize>,
    decls: Vec<Decl>,
    /// Open brace frames with the decls whose scope ends with them.
    stack: Vec<(Frame, Vec<usize>)>,
    /// A declaration waiting for its `{` body.
    pending_body: Option<usize>,
    /// Declaration with an `= expr` body, finished by `;` at `depth`.
    open_expr: Option<(usize, usize)>,
    /// Template parameters waiting for the declaration they belong to.
    pending_tparams: Vec<usize>,
}

fn collapse_ws(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    let mut space = false;
    for c in s.chars() {
        if c.is_whitespace() {
            space = true;
        } else {
            if space && !out.is_empty() && !matches!(c, ')' | ',' | ']' | '>' | ':') && !out.ends_with(['(', '[', '<']) {
                out.push(' ');
            }
            space = false;
            out.push(c);
        }
    }
    out
}

fn truncate(mut s: String, max: usize) -> String {
    if s.chars().count() > max {
        s = s.chars().take(max).collect::<String>() + "…";
    }
    s
}

impl<'a> Scanner<'a> {
    fn t(&self, si: usize) -> &'a Tok {
        &self.toks[self.sig[si]]
    }

    fn text(&self, si: usize) -> &'a str {
        if si < self.sig.len() { self.toks[self.sig[si]].text(self.src) } else { "" }
    }

    fn is_ident(&self, si: usize) -> bool {
        si < self.sig.len() && self.t(si).kind == TokKind::Ident
    }

    /// Innermost declaration whose body we're in.
    fn enclosing_decl(&self) -> Option<usize> {
        self.stack.iter().rev().find_map(|(f, _)| match f {
            Frame::Body(d) => Some(*d),
            Frame::Block => None,
        })
    }

    /// Are we directly in a class/struct/object body (not inside a method)?
    fn in_type_body(&self) -> Option<usize> {
        match self.stack.last() {
            Some((Frame::Body(d), _)) if self.decls[*d].kind.is_type() || self.decls[*d].kind == DeclKind::Dsl => Some(*d),
            _ => None,
        }
    }

    fn in_function(&self) -> bool {
        self.stack.iter().any(|(f, _)| match f {
            Frame::Body(d) => self.decls[*d].kind.is_callable(),
            Frame::Block => true,
        })
    }

    /// Comments directly above byte offset `at` (no blank line between).
    fn doc_above(&self, at: usize) -> Option<String> {
        let first = self.toks.partition_point(|t| t.range.start < at);
        let mut lines: Vec<String> = Vec::new();
        let mut boundary = at;
        let mut i = first;
        while i > 0 {
            let t = &self.toks[i - 1];
            if !t.is_trivia() {
                break;
            }
            let gap = &self.src[t.range.end..boundary];
            if gap.matches('\n').count() > 1 || gap.trim().len() > 0 {
                break;
            }
            // A trailing comment on the previous code line belongs to that line.
            let line_start = self.src[..t.range.start].rfind('\n').map_or(0, |p| p + 1);
            if !self.src[line_start..t.range.start].trim().is_empty() {
                break;
            }
            let raw = t.text(self.src);
            let cleaned: Vec<String> = if raw.starts_with("/*") {
                raw.trim_start_matches("/**")
                    .trim_start_matches("/*")
                    .trim_end_matches("*/")
                    .lines()
                    .map(|l| l.trim().trim_start_matches('*').trim().to_string())
                    .collect()
            } else {
                vec![raw.trim_start_matches('/').trim().to_string()]
            };
            for l in cleaned.into_iter().rev() {
                lines.push(l);
            }
            boundary = t.range.start;
            i -= 1;
        }
        lines.reverse();
        while lines.first().is_some_and(|l| l.is_empty()) {
            lines.remove(0);
        }
        while lines.last().is_some_and(|l| l.is_empty()) {
            lines.pop();
        }
        if lines.is_empty() { None } else { Some(lines.join("\n")) }
    }

    fn push_decl(&mut self, name_si: usize, kind: DeclKind, start: usize, parent: Option<usize>) -> usize {
        let nt = self.t(name_si);
        let idx = self.decls.len();
        self.decls.push(Decl {
            name: nt.text(self.src).to_string(),
            kind,
            name_range: nt.range.clone(),
            range: start..nt.range.end,
            scope: 0..self.src.len(),
            parent,
            signature: String::new(),
            doc: None,
            mutable: false,
        });
        idx
    }

    /// Registers `d` to go out of scope when the innermost frame closes.
    fn scope_to_frame(&mut self, d: usize) {
        self.decls[d].scope.start = self.decls[d].name_range.start;
        if let Some((_, owned)) = self.stack.last_mut() {
            owned.push(d);
        } else {
            self.decls[d].scope.end = self.src.len();
        }
    }

    /// Index of the significant token matching the `(` at `open`, or the end.
    fn matching_paren(&self, open: usize) -> usize {
        let mut depth = 0i32;
        let mut i = open;
        while i < self.sig.len() {
            match self.text(i) {
                "(" => depth += 1,
                ")" => {
                    depth -= 1;
                    if depth == 0 {
                        return i;
                    }
                }
                "{" | "}" | ";" if depth > 0 && i > open => return i.saturating_sub(1),
                _ => {}
            }
            i += 1;
        }
        self.sig.len().saturating_sub(1)
    }

    /// `name: Type` pairs at the top level of the parenthesised list at `open`.
    fn params(&mut self, open: usize, kind: DeclKind, parent: Option<usize>) -> usize {
        let close = self.matching_paren(open);
        let mut depth = 0i32;
        let mut i = open + 1;
        while i < close {
            match self.text(i) {
                "(" | "[" | "<" => depth += 1,
                ">>" => depth -= 2,
                ")" | "]" | ">" => depth -= 1,
                _ => {}
            }
            if depth == 0 && self.is_ident(i) && self.text(i + 1) == ":" {
                let start = self.t(i).range.start;
                let d = self.push_decl(i, kind, start, parent);
                // Signature: `name: Type` up to the next top-level `,` or the `)`.
                let mut j = i + 2;
                let mut dd = 0i32;
                while j < close {
                    match self.text(j) {
                        "(" | "[" | "<" => dd += 1,
                        ">>" => dd -= 2,
                        ")" | "]" | ">" => dd -= 1,
                        "," if dd == 0 => break,
                        _ => {}
                    }
                    j += 1;
                }
                let end = self.t(j.saturating_sub(1).max(i)).range.end;
                self.decls[d].range = start..end;
                self.decls[d].signature = collapse_ws(&self.src[start..end]);
                if kind == DeclKind::Field {
                    self.decls[d].signature = format!("val {}", self.decls[d].signature);
                }
            }
            i += 1;
        }
        close
    }

    /// Start of the declaration's modifiers (`async unsafe inline constexpr
    /// override external`, `[@Attr(...)]`, `template<...>`) before `kw_si`.
    fn decl_start(&self, kw_si: usize) -> usize {
        let mut i = kw_si;
        loop {
            if i == 0 {
                break;
            }
            match self.text(i - 1) {
                "async" | "unsafe" | "inline" | "constexpr" | "override" | "external" => i -= 1,
                "]" => {
                    // `[@Attr(...)]`
                    let mut j = i - 1;
                    while j > 0 && self.text(j) != "[@" && self.text(j) != ";" && self.text(j) != "}" {
                        j -= 1;
                    }
                    if self.text(j) == "[@" { i = j } else { break }
                }
                ">" => {
                    // `template<...>`
                    let mut j = i - 1;
                    while j > 0 && self.text(j) != "template" && self.text(j) != ";" && self.text(j) != "}" {
                        j -= 1;
                    }
                    if self.text(j) == "template" { i = j } else { break }
                }
                _ => break,
            }
        }
        self.t(i).range.start
    }

    fn finish_expr_body(&mut self, end: usize) {
        if let Some((d, _)) = self.open_expr.take() {
            self.decls[d].range.end = end.max(self.decls[d].range.end);
            self.finish_callable(d);
        }
    }

    /// A function's parameters are visible in its whole declaration.
    fn finish_callable(&mut self, d: usize) {
        let range = self.decls[d].range.clone();
        for p in d + 1..self.decls.len() {
            if self.decls[p].parent == Some(d) && self.decls[p].kind == DeclKind::Parameter {
                self.decls[p].scope = self.decls[p].name_range.start..range.end;
            }
        }
    }

    /// Header of a declaration: from its start up to the body.
    fn header(&self, start: usize, end_si: usize) -> String {
        let end = self.t(end_si).range.end;
        truncate(collapse_ws(&self.src[start..end]), 160)
    }

    fn run(&mut self) {
        let mut si = 0usize;
        while si < self.sig.len() {
            let tok = self.t(si);
            let word = tok.text(self.src);
            let is_kw = tok.kind == TokKind::Keyword;
            match word {
                "{" if tok.kind == TokKind::Op => {
                    let frame = match self.pending_body.take() {
                        Some(d) => Frame::Body(d),
                        None => Frame::Block,
                    };
                    self.stack.push((frame, Vec::new()));
                }
                "}" if tok.kind == TokKind::Op => {
                    let end = tok.range.end;
                    // An expression body inside this frame ends here.
                    if let Some((_, depth)) = self.open_expr {
                        if depth >= self.stack.len() {
                            let prev_end = if si > 0 { self.t(si - 1).range.end } else { end };
                            self.finish_expr_body(prev_end);
                        }
                    }
                    if let Some((frame, owned)) = self.stack.pop() {
                        for d in owned {
                            self.decls[d].scope.end = end;
                        }
                        if let Frame::Body(d) = frame {
                            self.decls[d].range.end = end;
                            if self.decls[d].kind.is_callable() {
                                self.finish_callable(d);
                            }
                        }
                    }
                }
                ";" if tok.kind == TokKind::Op => {
                    if let Some(d) = self.pending_body.take() {
                        self.decls[d].range.end = tok.range.end;
                        if self.decls[d].kind.is_callable() {
                            self.finish_callable(d);
                        }
                    }
                    if let Some((_, depth)) = self.open_expr {
                        if depth == self.stack.len() {
                            self.finish_expr_body(tok.range.end);
                        }
                    }
                }
                "=" if tok.kind == TokKind::Op => {
                    // `def f(...): T = expr` — an expression body.
                    if let Some(d) = self.pending_body {
                        if self.decls[d].kind.is_callable() {
                            self.pending_body = None;
                            self.open_expr = Some((d, self.stack.len()));
                        }
                    }
                }
                "template" if is_kw => {
                    // template<typename T, Int N>
                    if self.text(si + 1) == "<" {
                        let mut j = si + 2;
                        while j < self.sig.len() && self.text(j) != ">" && self.text(j) != "{" {
                            if self.is_ident(j) && matches!(self.text(j + 1), "," | ">") {
                                let start = self.t(j - 1).range.start;
                                let d = self.push_decl(j, DeclKind::TypeParameter, start, None);
                                self.decls[d].signature = collapse_ws(&self.src[start..self.t(j).range.end]);
                                self.pending_tparams.push(d);
                            }
                            j += 1;
                        }
                        si = j;
                    }
                }
                "def" if is_kw => {
                    si = self.function(si);
                    continue;
                }
                "class" | "struct" | "object" | "enum" if is_kw => {
                    si = self.type_decl(si, word);
                    continue;
                }
                // `interface` is a word only before a name and `{`.
                "interface" if self.is_ident(si + 1) && self.text(si + 2) == "{" => {
                    si = self.type_decl(si, word);
                    continue;
                }
                "molecule" | "cfd" | "ode" | "protein" if is_kw => {
                    if self.is_ident(si + 1) {
                        let start = tok.range.start;
                        let parent = self.enclosing_decl();
                        let d = self.push_decl(si + 1, DeclKind::Dsl, start, parent);
                        self.decls[d].signature = format!("{} {}", word, self.decls[d].name);
                        self.decls[d].doc = self.doc_above(start);
                        self.pending_body = Some(d);
                        si += 2;
                        continue;
                    }
                }
                "val" | "var" if is_kw => {
                    si = self.variable(si, word == "var");
                    continue;
                }
                "for" | "map" | "filter" | "reduce" | "pmap" | "preduce" if is_kw => {
                    if self.text(si + 1) == "(" {
                        self.binders(si + 1, matches!(word, "pmap" | "preduce"));
                    }
                }
                "(" if tok.kind == TokKind::Op && si > 0 && self.text(si - 1) == "]" => {
                    // Lambda `[captures](a: Int) -> { ... }`.
                    let close = self.matching_paren(si);
                    if self.text(close + 1) == "->" {
                        let parent = self.enclosing_decl();
                        let first = self.decls.len();
                        self.params(si, DeclKind::Parameter, parent);
                        for d in first..self.decls.len() {
                            self.decls[d].kind = DeclKind::Binder;
                            self.decls[d].signature = format!("(lambda) {}", self.decls[d].signature);
                            // Visible until the enclosing frame closes (approximation).
                            self.scope_to_frame(d);
                        }
                    }
                }
                _ => {}
            }
            si += 1;
        }
        // Unclosed frames / bodies run to the end of the file.
        let end = self.src.len();
        self.finish_expr_body(end);
        while let Some((frame, owned)) = self.stack.pop() {
            for d in owned {
                self.decls[d].scope.end = end;
            }
            if let Frame::Body(d) = frame {
                self.decls[d].range.end = end;
                if self.decls[d].kind.is_callable() {
                    self.finish_callable(d);
                }
            }
        }
    }

    fn function(&mut self, def_si: usize) -> usize {
        let start = self.decl_start(def_si);
        // `external` among the modifiers directly before `def`.
        let external = (0..def_si)
            .rev()
            .take_while(|&i| self.t(i).range.start >= start)
            .any(|i| self.t(i).kind == TokKind::Keyword && self.text(i) == "external");
        let mut si = def_si + 1;
        let (name_si, kind) = if self.text(si) == "operator" {
            // `def operator + (` — the name is the operator token(s) up to `(`.
            si += 1;
            (si, DeclKind::Operator)
        } else if self.is_ident(si) {
            let kind = if external {
                DeclKind::External
            } else if self.in_type_body().is_some() {
                DeclKind::Method
            } else {
                DeclKind::Function
            };
            (si, kind)
        } else {
            return def_si + 1;
        };
        if name_si >= self.sig.len() {
            return name_si;
        }
        let parent = self.enclosing_decl();
        let d = self.push_decl(name_si, kind, start, parent);
        if kind == DeclKind::Operator {
            // Join multi-token operator names such as `[` `]`.
            let mut j = name_si + 1;
            while j < self.sig.len() && self.text(j) != "(" && j < name_si + 3 {
                j += 1;
            }
            let end = self.t(j - 1).range.end;
            let name_start = self.t(name_si).range.start;
            self.decls[d].name = format!("operator {}", collapse_ws(&self.src[name_start..end]));
            self.decls[d].name_range = name_start..end;
            si = j;
        } else {
            si = name_si + 1;
        }
        self.decls[d].doc = self.doc_above(start);
        if self.in_function() && kind == DeclKind::Function {
            self.scope_to_frame(d);
        }
        for tp in std::mem::take(&mut self.pending_tparams) {
            self.decls[tp].parent = Some(d);
            self.decls[tp].scope = self.decls[d].range.start..usize::MAX;
            self.reparent_scope_later(tp, d);
        }
        // Parameters.
        let mut header_end = name_si;
        if self.text(si) == "(" {
            let close = self.params(si, DeclKind::Parameter, Some(d));
            header_end = close;
            si = close + 1;
        }
        // Return type `: T` up to the body / `=` / `;`.
        if self.text(si) == ":" {
            let mut j = si + 1;
            let mut depth = 0i32;
            while j < self.sig.len() {
                match self.text(j) {
                    "<" | "(" | "[" => depth += 1,
                    ">>" => depth -= 2,
                    ">" | ")" | "]" => depth -= 1,
                    "{" | "=" | ";" if depth <= 0 => break,
                    "def" | "class" | "val" | "var" | "}" => break,
                    _ => {}
                }
                j += 1;
            }
            header_end = j.saturating_sub(1).max(header_end);
            si = j;
        }
        self.decls[d].signature = self.header(start, header_end.min(self.sig.len() - 1));
        self.decls[d].range.end = self.t(header_end.min(self.sig.len() - 1)).range.end;
        match self.text(si) {
            "{" | ";" => self.pending_body = Some(d),
            "=" => {
                self.open_expr = Some((d, self.stack.len()));
                si += 1;
            }
            _ => self.finish_callable(d),
        }
        si
    }

    /// Template parameters are visible in the declaration they precede.
    fn reparent_scope_later(&mut self, tp: usize, _d: usize) {
        // Resolved in `fix_tparam_scopes` once the declaration's range is known.
        self.decls[tp].scope.end = usize::MAX;
    }

    fn type_decl(&mut self, kw_si: usize, word: &str) -> usize {
        if !self.is_ident(kw_si + 1) {
            return kw_si + 1;
        }
        let kind = match word {
            "class" => DeclKind::Class,
            "struct" => DeclKind::Struct,
            "enum" => DeclKind::Enum,
            "interface" => DeclKind::Interface,
            _ => DeclKind::Object,
        };
        let start = self.decl_start(kw_si);
        let parent = self.enclosing_decl();
        let d = self.push_decl(kw_si + 1, kind, start, parent);
        self.decls[d].doc = self.doc_above(start);
        for tp in std::mem::take(&mut self.pending_tparams) {
            self.decls[tp].parent = Some(d);
            self.reparent_scope_later(tp, d);
        }
        let mut si = kw_si + 2;
        let mut header_end = kw_si + 1;
        if self.text(si) == "(" {
            let close = self.params(si, DeclKind::Field, Some(d));
            header_end = close;
            si = close + 1;
        }
        if self.text(si) == "extends" {
            let mut j = si + 1;
            while j < self.sig.len() && !matches!(self.text(j), "{" | ";" | "def" | "val" | "var" | "}") {
                j += 1;
            }
            header_end = j - 1;
            si = j;
        }
        self.decls[d].signature = self.header(start, header_end);
        self.decls[d].range.end = self.t(header_end).range.end;
        if matches!(self.text(si), "{" | ";") {
            self.pending_body = Some(d);
        }
        si
    }

    fn variable(&mut self, kw_si: usize, mutable: bool) -> usize {
        let kind = if self.in_type_body().is_some() {
            DeclKind::Field
        } else if self.in_function() {
            DeclKind::Local
        } else {
            DeclKind::Global
        };
        let start = self.t(kw_si).range.start;
        let parent = self.enclosing_decl();
        // Statement end: `;`, or the end of the line when `;` is omitted.
        let line_end = self.src[start..].find('\n').map_or(self.src.len(), |p| start + p);
        let mut stmt_end_si = kw_si;
        let mut depth = 0i32;
        let mut j = kw_si + 1;
        while j < self.sig.len() {
            let t = self.t(j);
            match self.text(j) {
                "(" | "[" | "{" => depth += 1,
                ")" | "]" | "}" => {
                    depth -= 1;
                    if depth < 0 {
                        break;
                    }
                }
                ";" if depth == 0 => {
                    stmt_end_si = j;
                    break;
                }
                _ => {}
            }
            if depth == 0 && t.range.start > line_end && j > kw_si + 1 {
                break;
            }
            stmt_end_si = j;
            j += 1;
        }
        let stmt_end = self.t(stmt_end_si).range.end;
        let signature = truncate(collapse_ws(self.src[start..stmt_end].trim_end_matches(';')), 160);
        let doc = self.doc_above(start);
        // Names: `val a, b, _ = ...` (destructuring).
        let mut si = kw_si + 1;
        loop {
            if self.is_ident(si) && self.text(si) != "_" {
                let d = self.push_decl(si, kind, start, parent);
                self.decls[d].range = start..stmt_end;
                self.decls[d].signature = signature.clone();
                self.decls[d].doc = doc.clone();
                self.decls[d].mutable = mutable;
                if kind == DeclKind::Local {
                    self.scope_to_frame(d);
                }
            }
            if self.text(si + 1) == "," && (self.is_ident(si + 2)) {
                si += 2;
            } else {
                break;
            }
        }
        si + 1
    }

    /// `for (i <- ...)`, `map (x <- ...)`, `reduce (acc, x <- ...)`,
    /// `pmap (cpu, x <- ...)`, `for (text, ordinal <- ...)`.
    fn binders(&mut self, open: usize, policy: bool) {
        let mut names = Vec::new();
        let mut j = open + 1;
        while j < self.sig.len() {
            let w = self.text(j);
            if w == "<-" {
                break;
            }
            if self.is_ident(j) {
                names.push(j);
            } else if !matches!(w, "," | "|") {
                return;
            }
            j += 1;
        }
        if j >= self.sig.len() {
            return;
        }
        let parent = self.enclosing_decl();
        for n in names {
            if policy && crate::highlight::POLICY_WORDS.contains(&self.text(n)) {
                continue;
            }
            let start = self.t(n).range.start;
            let d = self.push_decl(n, DeclKind::Binder, start, parent);
            self.decls[d].signature = format!("{} <- …", self.decls[d].name);
            // Visible in the loop's block: approximated by the enclosing frame.
            self.scope_to_frame(d);
        }
    }

    fn fix_tparam_scopes(&mut self) {
        for i in 0..self.decls.len() {
            if self.decls[i].kind == DeclKind::TypeParameter {
                if let Some(p) = self.decls[i].parent {
                    self.decls[i].scope = self.decls[p].range.clone();
                } else {
                    self.decls[i].scope.end = self.decls[i].scope.end.min(self.src.len());
                }
            }
        }
    }
}

/// All declarations in source order (parents before children).
pub fn declarations(src: &str, toks: &[Tok]) -> Vec<Decl> {
    let sig = (0..toks.len()).filter(|&i| !toks[i].is_trivia()).collect();
    let mut s = Scanner {
        src,
        toks,
        sig,
        decls: Vec::new(),
        stack: Vec::new(),
        pending_body: None,
        open_expr: None,
        pending_tparams: Vec::new(),
    };
    s.run();
    s.fix_tparam_scopes();
    s.decls
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lexer::tokenize;

    fn decls(src: &str) -> Vec<Decl> {
        declarations(src, &tokenize(src))
    }

    fn summary(src: &str) -> Vec<(String, DeclKind, Option<String>)> {
        let ds = decls(src);
        ds.iter()
            .map(|d| (d.name.clone(), d.kind, d.parent.map(|p| ds[p].name.clone())))
            .collect()
    }

    #[test]
    fn functions_classes_fields() {
        let src = r#"
// Adds numbers.
def add(a: Int, b: Int): Int {
    val s = a + b;
    return s;
}

class Point(x: Double, y: Double) extends Base {
    var tag: Int = 0;
    def norm(): Double = sqrt(x * x + y * y);
    override def operator +(o: Point): Point { return new Point(x + o.x, y + o.y); }
}

struct Pair(a: Int, b: Int);
val G: Int = 3;
"#;
        assert_eq!(
            summary(src),
            vec![
                ("add".into(), DeclKind::Function, None),
                ("a".into(), DeclKind::Parameter, Some("add".into())),
                ("b".into(), DeclKind::Parameter, Some("add".into())),
                ("s".into(), DeclKind::Local, Some("add".into())),
                ("Point".into(), DeclKind::Class, None),
                ("x".into(), DeclKind::Field, Some("Point".into())),
                ("y".into(), DeclKind::Field, Some("Point".into())),
                ("tag".into(), DeclKind::Field, Some("Point".into())),
                ("norm".into(), DeclKind::Method, Some("Point".into())),
                ("operator +".into(), DeclKind::Operator, Some("Point".into())),
                ("o".into(), DeclKind::Parameter, Some("operator +".into())),
                ("Pair".into(), DeclKind::Struct, None),
                ("a".into(), DeclKind::Field, Some("Pair".into())),
                ("b".into(), DeclKind::Field, Some("Pair".into())),
                ("G".into(), DeclKind::Global, None),
            ]
        );
        let ds = decls(src);
        assert_eq!(ds[0].signature, "def add(a: Int, b: Int): Int");
        assert_eq!(ds[0].doc.as_deref(), Some("Adds numbers."));
        assert!(src[ds[0].range.clone()].ends_with('}'));
        assert_eq!(ds[4].signature, "class Point(x: Double, y: Double) extends Base");
        assert_eq!(ds[8].signature, "def norm(): Double");
        assert!(src[ds[8].range.clone()].ends_with("y * y);"));
        // A local is only visible inside its block.
        let s = &ds[3];
        assert!(s.scope.end <= ds[0].range.end);
    }

    #[test]
    fn binders_externals_templates() {
        let src = r#"
[@DllImport("m", "cos")]
external def ccos(x: Double): Double;
template<typename T, Int N>
def first(xs: T): T { return xs; }
def main(): Int {
    for (i <- 0 until 10) { println(i); }
    val total = preduce(cpu|simd, acc, x <- xs, 0) { acc + x };
    val a, _, c = triple();
    return 0;
}
"#;
        let s = summary(src);
        assert!(s.contains(&("ccos".into(), DeclKind::External, None)));
        assert!(s.contains(&("T".into(), DeclKind::TypeParameter, Some("first".into()))));
        assert!(s.contains(&("N".into(), DeclKind::TypeParameter, Some("first".into()))));
        assert!(s.contains(&("i".into(), DeclKind::Binder, Some("main".into()))));
        assert!(s.contains(&("acc".into(), DeclKind::Binder, Some("main".into()))));
        assert!(s.contains(&("x".into(), DeclKind::Binder, Some("main".into()))));
        assert!(!s.iter().any(|(n, _, _)| n == "cpu" || n == "simd" || n == "_"));
        assert!(s.contains(&("c".into(), DeclKind::Local, Some("main".into()))));
        let ds = decls(src);
        let ext = ds.iter().find(|d| d.name == "ccos").unwrap();
        assert!(src[ext.range.clone()].starts_with("[@DllImport"));
    }

    #[test]
    fn survives_garbage() {
        for src in ["def", "def (", "class", "val", "def f(a: ", "class C(x: Int", "}}}{{{", "template<", "for (x <-"] {
            let _ = decls(src);
        }
    }
}
