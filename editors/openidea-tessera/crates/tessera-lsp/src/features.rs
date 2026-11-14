//! Language features over analysed sources. Everything here is synchronous
//! and works from the in-memory text of open documents and their imports.

use crate::text::LineIndex;
use lsp_types::*;
use std::collections::{HashMap, HashSet};
use std::path::PathBuf;
use std::sync::Arc;
use tessera_syntax::docs::{keyword_doc, type_doc, BUILTIN_FUNCTIONS, BUILTIN_METHODS};
use tessera_syntax::highlight::{BUILTIN_NAMESPACES, BUILTIN_TYPES, CONTRACT_WORDS};
use tessera_syntax::lexer::KEYWORDS;
use tessera_syntax::{highlight, Analysis, Class, Decl, DeclKind, FoldKind, Tok, TokKind};

/// One analysed file.
pub struct Source {
    pub uri: Url,
    pub path: Option<PathBuf>,
    pub text: String,
    pub index: LineIndex,
    pub analysis: Analysis,
}

impl Source {
    pub fn new(uri: Url, text: String) -> Self {
        let path = uri.to_file_path().ok();
        let index = LineIndex::new(&text);
        let analysis = Analysis::new(&text);
        Source { uri, path, text, index, analysis }
    }

    pub fn range(&self, r: std::ops::Range<usize>) -> Range {
        self.index.range(&self.text, r)
    }

    pub fn offset(&self, p: Position) -> usize {
        self.index.offset(&self.text, p)
    }

    fn file_name(&self) -> String {
        self.path
            .as_ref()
            .and_then(|p| p.file_name())
            .map(|n| n.to_string_lossy().into_owned())
            .unwrap_or_else(|| self.uri.to_string())
    }

    /// Index of the significant token before token `i`.
    fn prev_sig(&self, i: usize) -> Option<&Tok> {
        self.analysis.tokens[..i].iter().rev().find(|t| !t.is_trivia())
    }

    /// Identifier at `offset` with its token index.
    pub fn ident_at(&self, offset: usize) -> Option<(usize, &Tok)> {
        let toks = &self.analysis.tokens;
        let t = self.analysis.token_at(offset)?;
        let i = toks.iter().position(|x| std::ptr::eq(x, t))?;
        (t.kind == TokKind::Ident).then_some((i, t))
    }

    fn is_member_access(&self, i: usize) -> bool {
        self.prev_sig(i).is_some_and(|p| p.text(&self.text) == ".")
    }
}

/// A declaration and the file it lives in.
#[derive(Clone, Copy)]
pub struct Target<'a> {
    pub src: &'a Source,
    pub decl: &'a Decl,
}

fn is_member_kind(k: DeclKind) -> bool {
    matches!(k, DeclKind::Field | DeclKind::Method | DeclKind::Operator)
}

fn is_top_level_export(d: &Decl) -> bool {
    d.parent.is_none()
        && matches!(
            d.kind,
            DeclKind::Function | DeclKind::External | DeclKind::Class | DeclKind::Struct | DeclKind::Object | DeclKind::Dsl | DeclKind::Global
        )
}

/// Declarations an identifier refers to.
pub fn lookup<'a>(cur: &'a Source, imports: &'a [Arc<Source>], name: &str, offset: usize, member: bool) -> Vec<Target<'a>> {
    let all = std::iter::once(cur).chain(imports.iter().map(|s| s.as_ref()));
    if member {
        let v: Vec<Target> = all
            .flat_map(|s| s.analysis.decls.iter().filter(|d| d.name == name && is_member_kind(d.kind)).map(move |d| Target { src: s, decl: d }))
            .collect();
        return v;
    }
    let local = cur.analysis.resolve(name, offset);
    if let Some(first) = local.first() {
        let whole = first.scope.start == 0 && first.scope.end >= cur.text.len();
        if !whole {
            return vec![Target { src: cur, decl: first }];
        }
        // File-level: all overloads; members only when nothing else matches.
        let mut v: Vec<Target> = local.iter().filter(|d| !is_member_kind(d.kind)).map(|d| Target { src: cur, decl: d }).collect();
        if !v.is_empty() {
            return v;
        }
        v = local.iter().map(|d| Target { src: cur, decl: d }).collect();
        // Fall through to imports only if nothing at all.
        if !v.is_empty() {
            let imported: Vec<Target> = imports
                .iter()
                .flat_map(|s| s.analysis.decls.iter().filter(|d| d.name == name && is_top_level_export(d)).map(move |d| Target { src: s.as_ref(), decl: d }))
                .collect();
            return if imported.is_empty() { v } else { imported };
        }
    }
    imports
        .iter()
        .flat_map(|s| s.analysis.decls.iter().filter(|d| d.name == name && is_top_level_export(d)).map(move |d| Target { src: s.as_ref(), decl: d }))
        .collect()
}

// ---------------------------------------------------------------- symbols

fn symbol_kind(d: &Decl) -> SymbolKind {
    match d.kind {
        DeclKind::Function | DeclKind::External => SymbolKind::FUNCTION,
        DeclKind::Method => SymbolKind::METHOD,
        DeclKind::Operator => SymbolKind::OPERATOR,
        DeclKind::Class => SymbolKind::CLASS,
        DeclKind::Struct => SymbolKind::STRUCT,
        DeclKind::Object => SymbolKind::OBJECT,
        DeclKind::Dsl => SymbolKind::MODULE,
        DeclKind::Field => SymbolKind::FIELD,
        DeclKind::Global if d.mutable => SymbolKind::VARIABLE,
        DeclKind::Global => SymbolKind::CONSTANT,
        DeclKind::TypeParameter => SymbolKind::TYPE_PARAMETER,
        DeclKind::Local | DeclKind::Parameter | DeclKind::Binder => SymbolKind::VARIABLE,
    }
}

fn detail(d: &Decl) -> Option<String> {
    let s = &d.signature;
    // Show what follows the name: `(a: Int): Int`, `: Double`.
    let after = s.find(&d.name).map(|i| s[i + d.name.len()..].trim().to_string()).filter(|r| !r.is_empty());
    after.or_else(|| Some(d.kind.label().to_string()))
}

#[allow(deprecated)]
pub fn document_symbols(src: &Source) -> Vec<DocumentSymbol> {
    let decls = &src.analysis.decls;
    // Outline parent = nearest ancestor that is itself in the outline.
    let outline_parent = |mut p: Option<usize>| {
        while let Some(i) = p {
            if decls[i].kind.in_outline() {
                return Some(i);
            }
            p = decls[i].parent;
        }
        None
    };
    let mut children: HashMap<Option<usize>, Vec<usize>> = HashMap::new();
    for (i, d) in decls.iter().enumerate() {
        if d.kind.in_outline() {
            children.entry(outline_parent(d.parent)).or_default().push(i);
        }
    }
    fn build(i: usize, src: &Source, children: &HashMap<Option<usize>, Vec<usize>>) -> DocumentSymbol {
        let d = &src.analysis.decls[i];
        let full = d.range.start.min(d.name_range.start)..d.range.end.max(d.name_range.end);
        DocumentSymbol {
            name: d.name.clone(),
            detail: detail(d),
            kind: symbol_kind(d),
            tags: None,
            deprecated: None,
            range: src.range(full),
            selection_range: src.range(d.name_range.clone()),
            children: children.get(&Some(i)).map(|c| c.iter().map(|&c| build(c, src, children)).collect()),
        }
    }
    children.get(&None).map(|c| c.iter().map(|&i| build(i, src, &children)).collect()).unwrap_or_default()
}

#[allow(deprecated)]
pub fn workspace_symbols(sources: &[Arc<Source>], query: &str) -> Vec<SymbolInformation> {
    let q: Vec<char> = query.to_lowercase().chars().collect();
    let matches = |name: &str| {
        let mut qi = 0;
        for c in name.to_lowercase().chars() {
            if qi < q.len() && c == q[qi] {
                qi += 1;
            }
        }
        qi == q.len()
    };
    let mut out = Vec::new();
    for s in sources {
        for d in &s.analysis.decls {
            if !d.kind.in_outline() || !matches(&d.name) {
                continue;
            }
            out.push(SymbolInformation {
                name: d.name.clone(),
                kind: symbol_kind(d),
                tags: None,
                deprecated: None,
                location: Location::new(s.uri.clone(), s.range(d.name_range.clone())),
                container_name: d.parent.map(|p| s.analysis.decls[p].name.clone()),
            });
            if out.len() >= 500 {
                return out;
            }
        }
    }
    out
}

pub fn folding_ranges(src: &Source) -> Vec<FoldingRange> {
    tessera_syntax::folds(&src.text, &src.analysis.tokens)
        .into_iter()
        .map(|f| FoldingRange {
            start_line: f.start_line,
            start_character: None,
            end_line: f.end_line,
            end_character: None,
            kind: Some(match f.kind {
                FoldKind::Comment => FoldingRangeKind::Comment,
                FoldKind::Imports => FoldingRangeKind::Imports,
                FoldKind::Region => FoldingRangeKind::Region,
            }),
            collapsed_text: None,
        })
        .collect()
}

// ---------------------------------------------------------- semantic tokens

pub const TOKEN_TYPES: &[SemanticTokenType] = &[
    SemanticTokenType::NAMESPACE,      // 0
    SemanticTokenType::TYPE,           // 1
    SemanticTokenType::CLASS,          // 2
    SemanticTokenType::STRUCT,         // 3
    SemanticTokenType::TYPE_PARAMETER, // 4
    SemanticTokenType::PARAMETER,      // 5
    SemanticTokenType::VARIABLE,       // 6
    SemanticTokenType::PROPERTY,       // 7
    SemanticTokenType::FUNCTION,       // 8
    SemanticTokenType::METHOD,         // 9
    SemanticTokenType::MACRO,          // 10
    SemanticTokenType::KEYWORD,        // 11
    SemanticTokenType::COMMENT,        // 12
    SemanticTokenType::STRING,         // 13
    SemanticTokenType::NUMBER,         // 14
    SemanticTokenType::REGEXP,         // 15
    SemanticTokenType::OPERATOR,       // 16
    SemanticTokenType::DECORATOR,      // 17
];
pub const TOKEN_MODIFIERS: &[SemanticTokenModifier] = &[
    SemanticTokenModifier::DECLARATION,     // 1
    SemanticTokenModifier::READONLY,        // 2
    SemanticTokenModifier::DEFAULT_LIBRARY, // 4
    SemanticTokenModifier::DOCUMENTATION,   // 8
];
const DECL: u32 = 1;
const RO: u32 = 2;
const LIB: u32 = 4;
const DOC: u32 = 8;

pub fn legend() -> SemanticTokensLegend {
    SemanticTokensLegend { token_types: TOKEN_TYPES.to_vec(), token_modifiers: TOKEN_MODIFIERS.to_vec() }
}

fn class_token(c: Class, word: &str) -> Option<(u32, u32)> {
    Some(match c {
        Class::Keyword => (11, 0),
        Class::Constant if matches!(word, "true" | "false" | "null") => (11, 0),
        Class::Constant => (6, RO),
        Class::String | Class::Escape => (13, 0),
        Class::Interpolation => (16, 0),
        Class::Regex => (15, 0),
        Class::Number => (14, 0),
        Class::Comment => (12, 0),
        Class::DocComment => (12, DOC),
        Class::FunctionDecl => (8, DECL),
        Class::FunctionCall => (8, 0),
        Class::MethodCall => (9, 0),
        Class::Type if BUILTIN_TYPES.contains(&word) => (1, LIB),
        Class::Type => (1, 0),
        Class::TypeDecl => (2, DECL),
        Class::Builtin if BUILTIN_NAMESPACES.contains(&word) => (0, LIB),
        Class::Builtin => (8, LIB),
        Class::Parameter => (5, DECL),
        Class::Variable => (6, 0),
        Class::Property => (7, 0),
        Class::SelfRef => (11, 0),
        Class::Attribute => (17, 0),
        Class::Macro => (10, 0),
        Class::Operator => (16, 0),
        Class::Punctuation => return None,
    })
}

fn decl_token(d: &Decl, at_decl: bool) -> (u32, u32) {
    let decl = if at_decl { DECL } else { 0 };
    match d.kind {
        DeclKind::Parameter => (5, decl),
        DeclKind::Local | DeclKind::Binder | DeclKind::Global => (6, decl | if d.mutable { 0 } else { RO }),
        DeclKind::Field => (7, decl | if d.mutable { 0 } else { RO }),
        DeclKind::Function | DeclKind::External => (8, decl),
        DeclKind::Method | DeclKind::Operator => (9, decl),
        DeclKind::Class | DeclKind::Object | DeclKind::Dsl => (2, decl),
        DeclKind::Struct => (3, decl),
        DeclKind::TypeParameter => (4, decl),
    }
}

pub fn semantic_tokens(src: &Source) -> Vec<SemanticToken> {
    let text = &src.text;
    let toks = &src.analysis.tokens;
    let mut items: Vec<(std::ops::Range<usize>, u32, u32)> = Vec::new();
    let mut classified = HashSet::new();
    for (r, c) in highlight(text, toks) {
        classified.insert(r.start);
        if let Some((ty, m)) = class_token(c, &text[r.clone()]) {
            items.push((r, ty, m));
        }
    }
    // Plain identifiers: colour by what they resolve to.
    let decl_at: HashMap<usize, &Decl> = src.analysis.decls.iter().map(|d| (d.name_range.start, d)).collect();
    for (i, t) in toks.iter().enumerate() {
        if t.kind != TokKind::Ident || classified.contains(&t.range.start) {
            continue;
        }
        if let Some(d) = decl_at.get(&t.range.start) {
            let (ty, m) = decl_token(d, true);
            items.push((t.range.clone(), ty, m));
            continue;
        }
        let name = t.text(text);
        let member = src.is_member_access(i);
        if member {
            continue;
        }
        if let Some(d) = src.analysis.resolve(name, t.range.start).into_iter().next() {
            let (ty, m) = decl_token(d, false);
            items.push((t.range.clone(), ty, m));
        }
    }
    items.sort_by_key(|(r, _, _)| r.start);

    let mut out = Vec::with_capacity(items.len());
    let (mut last_line, mut last_col) = (0u32, 0u32);
    for (r, ty, m) in items {
        // Split multi-line tokens (block comments, strings) per line.
        let mut start = r.start;
        while start < r.end {
            let line_end = text[start..r.end].find('\n').map_or(r.end, |p| start + p);
            let seg_end = if line_end > start && text.as_bytes()[line_end - 1] == b'\r' { line_end - 1 } else { line_end };
            if seg_end > start {
                let p = src.index.position(text, start);
                let len: u32 = text[start..seg_end].chars().map(|c| c.len_utf16() as u32).sum();
                let delta_line = p.line - last_line;
                let delta_start = if delta_line == 0 { p.character - last_col } else { p.character };
                out.push(SemanticToken { delta_line, delta_start, length: len, token_type: ty, token_modifiers_bitset: m });
                last_line = p.line;
                last_col = p.character;
            }
            start = line_end + 1;
        }
    }
    out
}

// --------------------------------------------------------------- hover

fn md_code(sig: &str) -> String {
    format!("```tessera\n{sig}\n```")
}

fn describe(t: Target, cur: &Source) -> String {
    let d = t.decl;
    let mut s = md_code(&d.signature);
    let mut meta = d.kind.label().to_string();
    if let Some(p) = d.parent {
        let parent = &t.src.analysis.decls[p];
        meta.push_str(&format!(" in `{}`", parent.name));
    }
    if !std::ptr::eq(t.src, cur) {
        meta.push_str(&format!(" — `{}`", t.src.file_name()));
    }
    s.push_str(&format!("\n\n*{meta}*"));
    if let Some(doc) = &d.doc {
        s.push_str("\n\n---\n\n");
        s.push_str(doc);
    }
    s
}

pub fn hover(cur: &Source, imports: &[Arc<Source>], pos: Position) -> Option<Hover> {
    let offset = cur.offset(pos);
    let t = cur.analysis.token_at(offset)?;
    let word = t.text(&cur.text);
    let range = Some(cur.range(t.range.clone()));
    let value = match t.kind {
        TokKind::Keyword | TokKind::Bool => keyword_doc(word).map(|d| format!("**{word}** — {d}"))?,
        TokKind::Ident => {
            let i = cur.analysis.tokens.iter().position(|x| std::ptr::eq(x, t))?;
            let member = cur.is_member_access(i);
            let targets = lookup(cur, imports, word, t.range.start, member);
            if !targets.is_empty() {
                let mut parts: Vec<String> = targets.iter().take(4).map(|x| describe(*x, cur)).collect();
                if targets.len() > 4 {
                    parts.push(format!("*… and {} more*", targets.len() - 4));
                }
                parts.join("\n\n---\n\n")
            } else if let Some(d) = type_doc(word) {
                format!("{}\n\n{d}", md_code(&format!("type {word}")))
            } else if BUILTIN_FUNCTIONS.contains(&word) && !member {
                format!("{}\n\n*built-in function*", md_code(&format!("{word}(…)")))
            } else if BUILTIN_METHODS.contains(&word) && member {
                format!("{}\n\n*built-in method*", md_code(&format!(".{word}(…)")))
            } else if BUILTIN_NAMESPACES.contains(&word) {
                format!("**{word}** — built-in runtime object")
            } else if CONTRACT_WORDS.contains(&word) || word == "result" {
                format!("**{word}** — {}", keyword_doc(word)?)
            } else {
                return None;
            }
        }
        _ => return None,
    };
    Some(Hover { contents: HoverContents::Markup(MarkupContent { kind: MarkupKind::Markdown, value }), range })
}

// ------------------------------------------------------------- navigation

/// Definition of the identifier at `pos`, or the file an `import` names.
pub fn definition(cur: &Source, imports: &[Arc<Source>], pos: Position, resolve_import: &dyn Fn(&str) -> Option<PathBuf>) -> Vec<Location> {
    let offset = cur.offset(pos);
    let Some(t) = cur.analysis.token_at(offset) else { return Vec::new() };
    if t.kind == TokKind::Str {
        // `import "lib/util";`
        let line = cur.index.position(&cur.text, t.range.start).line as usize;
        let lr = cur.index.line_range(&cur.text, line);
        if let Some(target) = crate::imports::parse_import_target(&cur.text[lr]) {
            if let Some(p) = resolve_import(&target) {
                if let Ok(u) = Url::from_file_path(&p) {
                    return vec![Location::new(u, Range::default())];
                }
            }
        }
        return Vec::new();
    }
    let Some((i, t)) = cur.ident_at(offset) else { return Vec::new() };
    let member = cur.is_member_access(i);
    lookup(cur, imports, t.text(&cur.text), t.range.start, member)
        .into_iter()
        .map(|x| Location::new(x.src.uri.clone(), x.src.range(x.decl.name_range.clone())))
        .collect()
}

fn same_decl(a: Target, b: Target) -> bool {
    std::ptr::eq(a.src, b.src) && a.decl.name_range == b.decl.name_range
}

/// Occurrences of the symbol at `pos` in `docs` (the current document first).
pub fn references(cur: &Source, imports: &[Arc<Source>], others: &[Arc<Source>], pos: Position, include_decl: bool) -> Vec<Location> {
    let offset = cur.offset(pos);
    let Some((i, t)) = cur.ident_at(offset) else { return Vec::new() };
    let name = t.text(&cur.text).to_string();
    let member = cur.is_member_access(i);
    let targets = lookup(cur, imports, &name, t.range.start, member);
    let Some(&target) = targets.first() else { return Vec::new() };

    let mut out = Vec::new();
    let mut scan = |src: &Source, imps: &[Arc<Source>]| {
        for (j, tok) in src.analysis.tokens.iter().enumerate() {
            if tok.kind != TokKind::Ident || tok.text(&src.text) != name {
                continue;
            }
            // `x.name` can only reach a member; a bare name may still reach one inside its class.
            let m = src.is_member_access(j);
            if m && !is_member_kind(target.decl.kind) {
                continue;
            }
            let hits = lookup(src, imps, &name, tok.range.start, m);
            let is_ref = hits.iter().any(|h| {
                same_decl(*h, target)
                    || (h.src.uri == target.src.uri && h.decl.name_range == target.decl.name_range)
            });
            if !is_ref {
                continue;
            }
            let is_decl = src.uri == target.src.uri && tok.range == target.decl.name_range;
            if is_decl && !include_decl {
                continue;
            }
            out.push(Location::new(src.uri.clone(), src.range(tok.range.clone())));
        }
    };
    scan(cur, imports);
    let local_only = !(target.decl.scope.start == 0 && target.decl.scope.end >= target.src.text.len());
    if !local_only {
        for o in others {
            if o.uri == cur.uri {
                continue;
            }
            // Others see the target through their own imports; approximate with the defining file.
            let defining: Vec<Arc<Source>> = imports.iter().filter(|s| s.uri == target.src.uri).cloned().collect();
            scan(o, &defining);
        }
    }
    out
}

pub fn document_highlights(cur: &Source, imports: &[Arc<Source>], pos: Position) -> Vec<DocumentHighlight> {
    references(cur, imports, &[], pos, true)
        .into_iter()
        .filter(|l| l.uri == cur.uri)
        .map(|l| DocumentHighlight { range: l.range, kind: Some(DocumentHighlightKind::TEXT) })
        .collect()
}

/// Rename is offered for names declared in the current file whose uses can
/// all be found syntactically (not fields / methods, which `x.name` reaches
/// without type information).
pub fn prepare_rename(cur: &Source, imports: &[Arc<Source>], pos: Position) -> Result<Range, String> {
    let offset = cur.offset(pos);
    let (i, t) = cur.ident_at(offset).ok_or("not an identifier")?;
    let member = cur.is_member_access(i);
    let targets = lookup(cur, imports, t.text(&cur.text), t.range.start, member);
    let target = targets.first().ok_or("no declaration found for this name")?;
    if !std::ptr::eq(target.src, cur) {
        return Err(format!("`{}` is declared in {}; rename it there", target.decl.name, target.src.file_name()));
    }
    if is_member_kind(target.decl.kind) {
        return Err("fields and methods can't be renamed safely without type information".into());
    }
    Ok(cur.range(t.range.clone()))
}

pub fn rename(cur: &Source, imports: &[Arc<Source>], others: &[Arc<Source>], pos: Position, new_name: &str) -> Result<WorkspaceEdit, String> {
    prepare_rename(cur, imports, pos)?;
    let valid = new_name.chars().next().is_some_and(|c| c == '_' || c.is_alphabetic())
        && new_name.chars().all(|c| c == '_' || c.is_alphanumeric())
        && !KEYWORDS.contains(&new_name);
    if !valid {
        return Err(format!("`{new_name}` is not a valid identifier"));
    }
    let mut changes: HashMap<Url, Vec<TextEdit>> = HashMap::new();
    for l in references(cur, imports, others, pos, true) {
        changes.entry(l.uri).or_default().push(TextEdit::new(l.range, new_name.to_string()));
    }
    Ok(WorkspaceEdit { changes: Some(changes), ..Default::default() })
}

// ----------------------------------------------------------- completion

fn completion_kind(d: &Decl) -> CompletionItemKind {
    match d.kind {
        DeclKind::Function | DeclKind::External => CompletionItemKind::FUNCTION,
        DeclKind::Method | DeclKind::Operator => CompletionItemKind::METHOD,
        DeclKind::Class | DeclKind::Object | DeclKind::Dsl => CompletionItemKind::CLASS,
        DeclKind::Struct => CompletionItemKind::STRUCT,
        DeclKind::Field => CompletionItemKind::FIELD,
        DeclKind::Global if !d.mutable => CompletionItemKind::CONSTANT,
        DeclKind::TypeParameter => CompletionItemKind::TYPE_PARAMETER,
        _ => CompletionItemKind::VARIABLE,
    }
}

const SNIPPETS: &[(&str, &str, &str)] = &[
    ("def", "def ${1:name}(${2}): ${3:Int} {\n\t$0\n}", "function"),
    ("main", "def main(): Int {\n\t$0\n\treturn 0;\n}", "main function"),
    ("class", "class ${1:Name}(${2}) {\n\t$0\n}", "class"),
    ("struct", "struct ${1:Name}(${2});", "struct"),
    ("if", "if (${1:cond}) {\n\t$0\n}", "if"),
    ("ifelse", "if (${1:cond}) {\n\t$2\n} else {\n\t$0\n}", "if / else"),
    ("while", "while (${1:cond}) {\n\t$0\n}", "while loop"),
    ("for", "for (${1:i} <- ${2:0} until ${3:n}) {\n\t$0\n}", "range loop"),
    ("match", "${1:value} match {\n\tcase ${2:pattern} => ${3:result};\n\tcase _ => ${0:default};\n}", "match"),
    ("map", "map (${1:x} <- ${2:xs}) { $0 }", "map comprehension"),
    ("filter", "filter (${1:x} <- ${2:xs}) { $0 }", "filter comprehension"),
    ("reduce", "reduce (${1:acc}, ${2:x} <- ${3:xs}, ${4:0}) { $0 }", "reduce"),
    ("pmap", "pmap (${1|cpu,simd,openmp,gpu|}, ${2:x} <- ${3:xs}) { $0 }", "parallel map"),
    ("import", "import \"${1:path}\";", "import"),
    ("tensor", "new Tensor<${1:Double}, ${2:2}, ${3:2}>(${4:0.0})", "new tensor"),
    ("dict", "{${1:\"key\"}: ${2:value}}", "dictionary literal"),
    ("set", "{${1:a}, ${2:b}}", "set literal"),
    ("contract", "def ${1:name}(${2:x}: ${3:Int}): ${4:Int}\n\trequires ${5:x >= 0}\n\tensures ${6:result >= 0}\n{\n\t$0\n}", "function with contract"),
    ("invariant", "while (${1:i} < ${2:n})\n\tinvariant ${3:0 <= i and i <= n}\n\tdecreases ${4:n - i}\n{\n\t$0\n}", "loop with invariant"),
    ("template", "template<${1:Int} ${2:N}>\nstruct ${3:Name}(${4:v: Long}) {\n\tdef operator +(o: ${3:Name}<${2:N}>): ${3:Name}<${2:N}> = $0\n}", "template struct with operator"),
    ("fordict", "for (${1:key}, ${2:value} <- ${3:dict}) {\n\t$0\n}", "dictionary loop"),
];

pub fn completion(cur: &Source, imports: &[Arc<Source>], pos: Position, snippets: bool) -> Vec<CompletionItem> {
    let offset = cur.offset(pos);
    let toks = &cur.analysis.tokens;
    // No completion inside comments and strings.
    if let Some(t) = toks.iter().find(|t| t.range.start < offset && offset < t.range.end) {
        if t.is_trivia() || matches!(t.kind, TokKind::Str | TokKind::Regex | TokKind::Char) {
            return Vec::new();
        }
    }
    if let Some(t) = toks.iter().find(|t| t.range.end == offset && t.kind == TokKind::LineComment) {
        let _ = t;
        return Vec::new();
    }
    // Is the word being typed preceded by `.`?
    let bytes = cur.text.as_bytes();
    let mut w = offset;
    while w > 0 {
        let c = cur.text[..w].chars().next_back().unwrap();
        if c == '_' || c.is_alphanumeric() {
            w -= c.len_utf8();
        } else {
            break;
        }
    }
    let mut p = w;
    while p > 0 && matches!(bytes[p - 1], b' ' | b'\t') {
        p -= 1;
    }
    let member = p > 0 && bytes[p - 1] == b'.';

    let mut items: Vec<CompletionItem> = Vec::new();
    let mut seen = HashSet::new();
    let add_decl =|items: &mut Vec<CompletionItem>, seen: &mut HashSet<String>, src: &Source, d: &Decl, sort: &str| {
        if !seen.insert(d.name.clone()) || d.kind == DeclKind::Operator {
            return;
        }
        let in_other = !std::ptr::eq(src, cur);
        items.push(CompletionItem {
            label: d.name.clone(),
            kind: Some(completion_kind(d)),
            detail: Some(d.signature.clone()),
            label_details: in_other.then(|| CompletionItemLabelDetails { detail: None, description: Some(src.file_name()) }),
            documentation: d.doc.clone().map(|doc| Documentation::MarkupContent(MarkupContent { kind: MarkupKind::Markdown, value: doc })),
            sort_text: Some(format!("{sort}{}", d.name)),
            ..Default::default()
        });
    };

    if member {
        for src in std::iter::once(cur).chain(imports.iter().map(|s| s.as_ref())) {
            for d in &src.analysis.decls {
                if matches!(d.kind, DeclKind::Field | DeclKind::Method) {
                    add_decl(&mut items, &mut seen, src, d, "1");
                }
            }
        }
        for m in BUILTIN_METHODS {
            if seen.insert(m.to_string()) {
                items.push(CompletionItem {
                    label: m.to_string(),
                    kind: Some(CompletionItemKind::METHOD),
                    detail: Some("built-in method".into()),
                    sort_text: Some(format!("5{m}")),
                    ..Default::default()
                });
            }
        }
        return items;
    }

    // Names in scope, innermost first.
    let mut visible: Vec<&Decl> = cur
        .analysis
        .decls
        .iter()
        .filter(|d| d.scope.start <= offset && offset <= d.scope.end && !(d.name_range.start <= offset && offset <= d.name_range.end))
        .filter(|d| !matches!(d.kind, DeclKind::Local | DeclKind::Binder) || d.name_range.end < offset)
        .collect();
    visible.sort_by_key(|d| d.scope.end - d.scope.start);
    for d in visible {
        let sort = match d.kind {
            DeclKind::Local | DeclKind::Parameter | DeclKind::Binder | DeclKind::TypeParameter => "0",
            DeclKind::Field | DeclKind::Method => "1",
            _ => "2",
        };
        add_decl(&mut items, &mut seen, cur, d, sort);
    }
    for src in imports {
        for d in src.analysis.decls.iter().filter(|d| is_top_level_export(d)) {
            add_decl(&mut items, &mut seen, src, d, "3");
        }
    }
    for t in BUILTIN_TYPES {
        if seen.insert(t.to_string()) {
            items.push(CompletionItem {
                label: t.to_string(),
                kind: Some(CompletionItemKind::CLASS),
                detail: type_doc(t).map(str::to_string),
                sort_text: Some(format!("4{t}")),
                ..Default::default()
            });
        }
    }
    for f in BUILTIN_FUNCTIONS {
        if seen.insert(f.to_string()) {
            items.push(CompletionItem {
                label: f.to_string(),
                kind: Some(CompletionItemKind::FUNCTION),
                detail: Some("built-in function".into()),
                sort_text: Some(format!("5{f}")),
                ..Default::default()
            });
        }
    }
    for n in BUILTIN_NAMESPACES {
        if seen.insert(n.to_string()) {
            items.push(CompletionItem { label: n.to_string(), kind: Some(CompletionItemKind::MODULE), sort_text: Some(format!("5{n}")), ..Default::default() });
        }
    }
    for k in KEYWORDS.iter().chain(CONTRACT_WORDS) {
        if seen.insert(k.to_string()) {
            items.push(CompletionItem {
                label: k.to_string(),
                kind: Some(CompletionItemKind::KEYWORD),
                documentation: keyword_doc(k).map(|d| Documentation::MarkupContent(MarkupContent { kind: MarkupKind::Markdown, value: d.into() })),
                sort_text: Some(format!("6{k}")),
                ..Default::default()
            });
        }
    }
    if snippets {
        for (label, body, desc) in SNIPPETS {
            items.push(CompletionItem {
                label: label.to_string(),
                kind: Some(CompletionItemKind::SNIPPET),
                detail: Some(desc.to_string()),
                insert_text: Some(body.to_string()),
                insert_text_format: Some(InsertTextFormat::SNIPPET),
                sort_text: Some(format!("7{label}")),
                ..Default::default()
            });
        }
    }
    items
}

// --------------------------------------------------------- signature help

pub fn signature_help(cur: &Source, imports: &[Arc<Source>], pos: Position) -> Option<SignatureHelp> {
    let offset = cur.offset(pos);
    let toks: Vec<(usize, &Tok)> = cur.analysis.tokens.iter().enumerate().filter(|(_, t)| !t.is_trivia() && t.range.end <= offset).collect();
    let mut depth = 0i32;
    let mut commas = 0u32;
    let mut open = None;
    for k in (0..toks.len()).rev() {
        let w = toks[k].1.text(&cur.text);
        match w {
            ")" | "]" | "}" => depth += 1,
            "[" | "[@" | "{" => {
                if depth == 0 {
                    return None;
                }
                depth -= 1;
            }
            "(" => {
                if depth == 0 {
                    open = Some(k);
                    break;
                }
                depth -= 1;
            }
            "," if depth == 0 => commas += 1,
            ";" if depth == 0 => return None,
            _ => {}
        }
    }
    let open = open?;
    if open == 0 {
        return None;
    }
    let (ci, callee) = toks[open - 1];
    if callee.kind != TokKind::Ident {
        return None;
    }
    let name = callee.text(&cur.text);
    let member = cur.is_member_access(ci);
    let targets: Vec<Target> = lookup(cur, imports, name, callee.range.start, member)
        .into_iter()
        .filter(|t| t.decl.kind.is_callable() || matches!(t.decl.kind, DeclKind::Class | DeclKind::Struct))
        .collect();
    if targets.is_empty() {
        return None;
    }
    let signatures: Vec<SignatureInformation> = targets
        .iter()
        .map(|t| {
            let label = t.decl.signature.clone();
            let idx = t.src.analysis.decls.iter().position(|d| std::ptr::eq(d, t.decl)).unwrap_or(usize::MAX);
            let want = if t.decl.kind.is_callable() { DeclKind::Parameter } else { DeclKind::Field };
            let params: Vec<ParameterInformation> = t
                .src
                .analysis
                .decls
                .iter()
                .filter(|d| d.parent == Some(idx) && d.kind == want)
                .map(|d| d.signature.trim_start_matches("val ").to_string())
                // Constructor fields only (body `val`s aren't in the header).
                .filter(|p| label.contains(p.as_str()))
                .map(|p| ParameterInformation { label: ParameterLabel::Simple(p), documentation: None })
                .collect();
            SignatureInformation {
                label,
                documentation: t.decl.doc.clone().map(|d| Documentation::MarkupContent(MarkupContent { kind: MarkupKind::Markdown, value: d })),
                parameters: Some(params),
                active_parameter: None,
            }
        })
        .collect();
    // Prefer an overload with enough parameters.
    let active = signatures.iter().position(|s| s.parameters.as_ref().map_or(0, |p| p.len()) as u32 > commas).unwrap_or(0);
    Some(SignatureHelp { signatures, active_signature: Some(active as u32), active_parameter: Some(commas) })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn src(text: &str) -> Source {
        Source::new(Url::parse("file:///t/main.csec").unwrap(), text.to_string())
    }

    fn pos_of(s: &Source, needle: &str, nth: usize) -> Position {
        let off = s.text.match_indices(needle).nth(nth).unwrap().0;
        s.index.position(&s.text, off)
    }

    const PROG: &str = "// Adds.\ndef add(a: Int, b: Int): Int {\n    val s = a + b;\n    return s;\n}\nclass P(x: Double) {\n    def get(): Double = x;\n}\ndef main(): Int {\n    val p = new P(1.0);\n    val r = add(1, 2);\n    return r;\n}\n";

    #[test]
    fn definition_and_references() {
        let s = src(PROG);
        let none = |_: &str| None;
        let d = definition(&s, &[], pos_of(&s, "add(1", 0), &none);
        assert_eq!(d.len(), 1);
        assert_eq!(d[0].range.start, Position::new(1, 4));
        let refs = references(&s, &[], &[], pos_of(&s, "s;", 0), true);
        assert_eq!(refs.len(), 2);
        // `x` in the method body is the constructor field.
        let d = definition(&s, &[], pos_of(&s, "x;", 0), &none);
        assert_eq!(d[0].range.start, Position::new(5, 8));
    }

    #[test]
    fn hover_completion_signature() {
        let s = src(PROG);
        let h = hover(&s, &[], pos_of(&s, "add(1", 0)).unwrap();
        let HoverContents::Markup(m) = h.contents else { panic!() };
        assert!(m.value.contains("def add(a: Int, b: Int): Int") && m.value.contains("Adds."), "{}", m.value);

        let c = completion(&s, &[], pos_of(&s, "return r", 0), true);
        let labels: Vec<_> = c.iter().map(|i| i.label.as_str()).collect();
        assert!(labels.contains(&"p") && labels.contains(&"r") && labels.contains(&"add") && labels.contains(&"while"));
        assert!(!labels.contains(&"s"), "`s` is local to add");

        let mut at = pos_of(&s, "add(1, 2)", 0);
        at.character += 7; // after `add(1, `
        let sh = signature_help(&s, &[], at).unwrap();
        assert_eq!(sh.active_parameter, Some(1));
        assert_eq!(sh.signatures[0].parameters.as_ref().unwrap().len(), 2);
    }

    #[test]
    fn rename_locals_only() {
        let s = src(PROG);
        let e = rename(&s, &[], &[], pos_of(&s, "s = a", 0), "sum").unwrap();
        let edits = &e.changes.unwrap()[&s.uri];
        assert_eq!(edits.len(), 2);
        assert!(prepare_rename(&s, &[], pos_of(&s, "get()", 0)).is_err());
    }

    #[test]
    fn semantic_tokens_are_ordered() {
        let s = src("/* a\n b */ def f(x: Int): Int { return x; }");
        let toks = semantic_tokens(&s);
        assert!(toks.len() > 5);
        assert_eq!((toks[0].delta_line, toks[0].length), (0, 4));
        assert_eq!((toks[1].delta_line, toks[1].length), (1, 5));
    }
}
