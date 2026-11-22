//! A lossless lexer for Tessera that mirrors `src/lexer.cpp` of the `tess`
//! compiler: same keywords, same maximal-munch operator set, same literal
//! rules. Unlike the compiler it keeps comments, records byte ranges, and never
//! fails: input the compiler rejects becomes [`TokKind::Unknown`] (or a token
//! flagged as unterminated) so editors can still highlight around it.

use std::ops::Range;

/// Reserved words (`Lexer::initializeKeywords`). `true`/`false` lex as
/// [`TokKind::Bool`], everything else here as [`TokKind::Keyword`].
pub const KEYWORDS: &[&str] = &[
    "case", "class", "def", "else", "extends", "false", "for", "if", "import", "match", "new", "null",
    "object", "override", "operator", "return", "super", "this", "true", "val", "var", "while", "to",
    "until", "and", "or", "xor", "map", "pmap", "reduce", "preduce", "filter", "external", "inline",
    "constexpr", "template", "typename", "mut", "box", "unsafe", "unatomic", "struct", "async",
    "await", "go", "build", "break", "continue", "enum", "molecule", "cfd", "ode", "protein",
];

/// Operators (`Lexer::initializeOperators`), matched longest first (max 3).
pub const OPERATORS: &[&str] = &[
    "=", "==", "!=", "+", "-", "*", "/", "%", "$", "$$", "\\", "<", ">", "<=", ">=", "<<", ">>",
    "<<|", ">>|", "!", "&", "|", "^", "~", ":", ".", ",", ";", "(", ")", "{", "}", "[", "]", "[@",
    "[%", "%]", "<-", "->", "++", "--", "+=", "-=", "*=", "/=", "%=", "**", "_", "=>", "<:", "<%", ">:", "#",
    "@", "'", "?",
];

pub fn is_keyword(word: &str) -> bool {
    KEYWORDS.contains(&word)
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum TokKind {
    Ident,
    Keyword,
    /// `true` / `false`.
    Bool,
    /// Decimal, hex (`0x`), binary (`0b`) or octal (`0o`) integer.
    Int,
    /// `1.5`, `1e9`, `2.5e-3`.
    Float,
    /// `'a'`, `'\n'`.
    Char,
    /// `"..."` or `u"..."`; may contain escapes and `${name}` interpolation.
    Str,
    /// `r"..."`: a regex literal (no escapes).
    Regex,
    LineComment,
    BlockComment,
    /// `///` or `/** ... */`.
    DocComment,
    Op,
    /// A character the compiler rejects (`?`, backtick, control characters).
    Unknown,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Tok {
    pub kind: TokKind,
    pub range: Range<usize>,
    /// False for a string / block comment that runs to the end of the file.
    pub terminated: bool,
}

impl Tok {
    pub fn text<'a>(&self, src: &'a str) -> &'a str {
        &src[self.range.clone()]
    }

    pub fn is_trivia(&self) -> bool {
        matches!(self.kind, TokKind::LineComment | TokKind::BlockComment | TokKind::DocComment)
    }
}

fn is_ident_start(c: char) -> bool {
    c == '_' || c.is_ascii_alphabetic() || !c.is_ascii()
}

fn is_ident_continue(c: char) -> bool {
    c == '_' || c.is_ascii_alphanumeric() || !c.is_ascii()
}

/// Tokenizes the whole source. Whitespace is dropped; everything else
/// (including comments) is kept, in order, without gaps between tokens other
/// than whitespace.
pub fn tokenize(src: &str) -> Vec<Tok> {
    let bytes = src.as_bytes();
    let mut toks = Vec::new();
    let mut pos = 0usize;
    let len = bytes.len();

    let push = |toks: &mut Vec<Tok>, kind, start, end, terminated| {
        toks.push(Tok { kind, range: start..end, terminated });
    };

    if src.starts_with("#!") {
        pos = src.find('\n').unwrap_or(len);
        push(&mut toks, TokKind::LineComment, 0, pos, true);
    }

    while pos < len {
        let b = bytes[pos];
        let start = pos;

        // Comments.
        if b == b'/' && pos + 1 < len && bytes[pos + 1] == b'/' {
            while pos < len && bytes[pos] != b'\n' {
                pos += 1;
            }
            let doc = src[start..pos].starts_with("///") && !src[start..pos].starts_with("////");
            push(&mut toks, if doc { TokKind::DocComment } else { TokKind::LineComment }, start, pos, true);
            continue;
        }
        if b == b'/' && pos + 1 < len && bytes[pos + 1] == b'*' {
            pos += 2;
            let mut terminated = false;
            while pos < len {
                if bytes[pos] == b'*' && pos + 1 < len && bytes[pos + 1] == b'/' {
                    pos += 2;
                    terminated = true;
                    break;
                }
                pos += 1;
            }
            let text = &src[start..pos];
            let doc = text.starts_with("/**") && !text.starts_with("/**/") && text.len() > 4;
            push(&mut toks, if doc { TokKind::DocComment } else { TokKind::BlockComment }, start, pos, terminated);
            continue;
        }

        // Whitespace (`std::isspace`).
        if matches!(b, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c) {
            pos += 1;
            continue;
        }

        // Prefixed strings: r"..." (regex, raw) and u"..." (unicode string).
        if matches!(b, b'r' | b'R' | b'u' | b'U') && pos + 1 < len && bytes[pos + 1] == b'"' {
            let raw = matches!(b, b'r' | b'R');
            pos += 2;
            let mut terminated = false;
            while pos < len {
                if bytes[pos] == b'"' {
                    pos += 1;
                    terminated = true;
                    break;
                }
                if !raw && bytes[pos] == b'\\' {
                    pos += 2;
                } else {
                    pos += 1;
                }
            }
            let pos2 = pos.min(len);
            pos = pos2;
            push(&mut toks, if raw { TokKind::Regex } else { TokKind::Str }, start, pos, terminated);
            continue;
        }

        // Plain strings. Like the compiler, a string may span lines.
        if b == b'"' {
            pos += 1;
            let mut terminated = false;
            while pos < len {
                if bytes[pos] == b'"' {
                    pos += 1;
                    terminated = true;
                    break;
                }
                if bytes[pos] == b'\\' {
                    pos += 2;
                } else {
                    pos += 1;
                }
            }
            pos = pos.min(len);
            push(&mut toks, TokKind::Str, start, pos, terminated);
            continue;
        }

        // Char literal: 'x' or '\x'; otherwise `'` is an operator.
        if b == b'\'' {
            let mut p = pos + 1;
            if p < len && bytes[p] == b'\\' {
                p += 2;
            } else if p < len {
                // One UTF-8 code point.
                p += src[p..].chars().next().map_or(1, char::len_utf8);
            }
            if p < len && bytes[p] == b'\'' {
                pos = p + 1;
                push(&mut toks, TokKind::Char, start, pos, true);
                continue;
            }
        }

        // Numbers.
        if b.is_ascii_digit() {
            while pos < len && bytes[pos].is_ascii_digit() {
                pos += 1;
            }
            let single_zero = pos - start == 1 && bytes[start] == b'0';
            let peek = bytes.get(pos).copied().unwrap_or(0);
            let kind = if single_zero && matches!(peek, b'x' | b'X') {
                pos += 1;
                while pos < len && bytes[pos].is_ascii_hexdigit() {
                    pos += 1;
                }
                TokKind::Int
            } else if single_zero && matches!(peek, b'b' | b'B') {
                pos += 1;
                while pos < len && matches!(bytes[pos], b'0' | b'1') {
                    pos += 1;
                }
                TokKind::Int
            } else if single_zero && matches!(peek, b'o' | b'O') {
                pos += 1;
                while pos < len && (b'0'..=b'7').contains(&bytes[pos]) {
                    pos += 1;
                }
                TokKind::Int
            } else if matches!(peek, b'e' | b'E') {
                let saved = pos;
                let mut p = pos + 1;
                if p < len && matches!(bytes[p], b'+' | b'-') {
                    p += 1;
                }
                let digits = p;
                while p < len && bytes[p].is_ascii_digit() {
                    p += 1;
                }
                if p > digits {
                    pos = p;
                    TokKind::Float
                } else {
                    pos = saved;
                    TokKind::Int
                }
            } else if peek == b'.' {
                pos += 1;
                while pos < len && bytes[pos].is_ascii_digit() {
                    pos += 1;
                }
                if pos < len && matches!(bytes[pos], b'e' | b'E') {
                    pos += 1;
                    if pos < len && matches!(bytes[pos], b'+' | b'-') {
                        pos += 1;
                    }
                    while pos < len && bytes[pos].is_ascii_digit() {
                        pos += 1;
                    }
                }
                TokKind::Float
            } else {
                TokKind::Int
            };
            push(&mut toks, kind, start, pos, true);
            continue;
        }

        // Identifiers and keywords (ASCII plus any non-ASCII code point).
        let ch = src[pos..].chars().next().unwrap();
        if is_ident_start(ch) {
            pos += ch.len_utf8();
            while pos < len {
                let c = src[pos..].chars().next().unwrap();
                if !is_ident_continue(c) {
                    break;
                }
                pos += c.len_utf8();
            }
            let word = &src[start..pos];
            let kind = if word == "true" || word == "false" {
                TokKind::Bool
            } else if is_keyword(word) {
                TokKind::Keyword
            } else {
                TokKind::Ident
            };
            push(&mut toks, kind, start, pos, true);
            continue;
        }

        // Operators, longest match first.
        let mut matched = 0;
        for n in (1..=3).rev() {
            if pos + n <= len && src.is_char_boundary(pos + n) && OPERATORS.contains(&&src[pos..pos + n]) {
                matched = n;
                break;
            }
        }
        if matched > 0 {
            pos += matched;
            push(&mut toks, TokKind::Op, start, pos, true);
            continue;
        }

        pos += ch.len_utf8();
        push(&mut toks, TokKind::Unknown, start, pos, true);
    }
    toks
}

/// Pieces of a string literal for highlighting.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum StrPart {
    Text,
    Escape,
    /// `${` or `}` of an interpolation.
    InterpDelim,
    /// The interpolated identifier.
    InterpName,
}

/// Splits a `Str` token into text, escapes and `${name}` interpolations.
/// Interpolation follows the compiler: only `${identifier}` is recognised.
pub fn string_parts(src: &str, range: Range<usize>) -> Vec<(Range<usize>, StrPart)> {
    let bytes = src.as_bytes();
    let mut out = Vec::new();
    let mut text_start = range.start;
    let mut i = range.start;
    let end = range.end;
    let flush = |out: &mut Vec<(Range<usize>, StrPart)>, from: usize, to: usize| {
        if to > from {
            out.push((from..to, StrPart::Text));
        }
    };
    while i < end {
        let b = bytes[i];
        if b == b'\\' && i + 1 < end {
            flush(&mut out, text_start, i);
            let ch_len = src[i + 1..].chars().next().map_or(1, char::len_utf8);
            let stop = (i + 1 + ch_len).min(end);
            out.push((i..stop, StrPart::Escape));
            i = stop;
            text_start = i;
            continue;
        }
        if b == b'$' && i + 2 < end && bytes[i + 1] == b'{' {
            let name_start = i + 2;
            let mut j = name_start;
            if j < end && (bytes[j] == b'_' || bytes[j].is_ascii_alphabetic()) {
                j += 1;
                while j < end && (bytes[j] == b'_' || bytes[j].is_ascii_alphanumeric()) {
                    j += 1;
                }
                if j < end && bytes[j] == b'}' {
                    flush(&mut out, text_start, i);
                    out.push((i..name_start, StrPart::InterpDelim));
                    out.push((name_start..j, StrPart::InterpName));
                    out.push((j..j + 1, StrPart::InterpDelim));
                    i = j + 1;
                    text_start = i;
                    continue;
                }
            }
        }
        i += 1;
    }
    flush(&mut out, text_start, end);
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    fn kinds(src: &str) -> Vec<(TokKind, &str)> {
        tokenize(src).iter().map(|t| (t.kind, t.text(src))).collect()
    }

    #[test]
    fn basic_tokens() {
        assert_eq!(
            kinds("def f(x: Int): Int = x >>| g; // hi"),
            vec![
                (TokKind::Keyword, "def"),
                (TokKind::Ident, "f"),
                (TokKind::Op, "("),
                (TokKind::Ident, "x"),
                (TokKind::Op, ":"),
                (TokKind::Ident, "Int"),
                (TokKind::Op, ")"),
                (TokKind::Op, ":"),
                (TokKind::Ident, "Int"),
                (TokKind::Op, "="),
                (TokKind::Ident, "x"),
                (TokKind::Op, ">>|"),
                (TokKind::Ident, "g"),
                (TokKind::Op, ";"),
                (TokKind::LineComment, "// hi"),
            ]
        );
    }

    #[test]
    fn numbers_follow_the_compiler() {
        assert_eq!(
            kinds("0x1F 0b101 0o17 12 1.5 1e9 2.5e-3 3e"),
            vec![
                (TokKind::Int, "0x1F"),
                (TokKind::Int, "0b101"),
                (TokKind::Int, "0o17"),
                (TokKind::Int, "12"),
                (TokKind::Float, "1.5"),
                (TokKind::Float, "1e9"),
                (TokKind::Float, "2.5e-3"),
                (TokKind::Int, "3"),
                (TokKind::Ident, "e"),
            ]
        );
    }

    #[test]
    fn strings_chars_and_regex() {
        assert_eq!(
            kinds(r#"r"a\d+" u"x" "a\"b" 'c' '\n' x'"#),
            vec![
                (TokKind::Regex, r#"r"a\d+""#),
                (TokKind::Str, r#"u"x""#),
                (TokKind::Str, r#""a\"b""#),
                (TokKind::Char, "'c'"),
                (TokKind::Char, r"'\n'"),
                (TokKind::Ident, "x"),
                (TokKind::Op, "'"),
            ]
        );
        let t = tokenize("\"open");
        assert!(!t[0].terminated);
    }

    #[test]
    fn unicode_identifiers_and_unknowns() {
        assert_eq!(
            kinds("val 이름 = π ? `"),
            vec![
                (TokKind::Keyword, "val"),
                (TokKind::Ident, "이름"),
                (TokKind::Op, "="),
                (TokKind::Ident, "π"),
                (TokKind::Op, "?"),
                (TokKind::Unknown, "`"),
            ]
        );
    }

    #[test]
    fn interpolation_parts() {
        let src = r#""a\n${name} ${1}""#;
        let parts = string_parts(src, 0..src.len());
        let got: Vec<_> = parts.iter().map(|(r, k)| (&src[r.clone()], *k)).collect();
        assert_eq!(
            got,
            vec![
                ("\"a", StrPart::Text),
                ("\\n", StrPart::Escape),
                ("${", StrPart::InterpDelim),
                ("name", StrPart::InterpName),
                ("}", StrPart::InterpDelim),
                (" ${1}\"", StrPart::Text),
            ]
        );
    }
}
