//! `import "x";` resolution and the line map of the compiler's import
//! expansion. Mirrors `parseImportTarget`, `resolveImportPath` and
//! `expandImports` in `src/main.cpp`: imports are inlined textually, so the
//! line numbers `tess` reports refer to the expanded text.

use std::path::{Path, PathBuf};

/// Strips `\\?\` from Windows canonical paths so they compare and print like
/// the paths editors use.
pub fn simplify(p: PathBuf) -> PathBuf {
    #[cfg(windows)]
    {
        let s = p.to_string_lossy();
        if let Some(rest) = s.strip_prefix(r"\\?\UNC\") {
            return PathBuf::from(format!(r"\\{rest}"));
        }
        if let Some(rest) = s.strip_prefix(r"\\?\") {
            return PathBuf::from(rest);
        }
    }
    p
}

pub fn canonical(p: &Path) -> PathBuf {
    match std::fs::canonicalize(p) {
        Ok(c) => simplify(c),
        Err(_) => std::path::absolute(p).unwrap_or_else(|_| p.to_path_buf()),
    }
}

fn starts_with_import_keyword(t: &str) -> bool {
    if !t.starts_with("import") {
        return false;
    }
    match t.as_bytes().get(6) {
        None => true,
        Some(b) => matches!(b, b' ' | b'\t' | b'"' | b'\''),
    }
}

/// The target of an `import` line, as `tess` reads it (one import per line).
pub fn parse_import_target(line: &str) -> Option<String> {
    let t = line.trim_matches(|c| matches!(c, ' ' | '\t' | '\r' | '\n'));
    if !starts_with_import_keyword(t) {
        return None;
    }
    let mut rest = t[6..].trim_matches(|c| matches!(c, ' ' | '\t' | '\r' | '\n'));
    if let Some(r) = rest.strip_suffix(';') {
        rest = r.trim_matches(|c| matches!(c, ' ' | '\t' | '\r' | '\n'));
    }
    if rest.is_empty() {
        return None;
    }
    let quoted = (rest.starts_with('"') && rest.ends_with('"')) || (rest.starts_with('\'') && rest.ends_with('\''));
    if quoted {
        if rest.len() <= 2 {
            return None;
        }
        return Some(rest[1..rest.len() - 1].to_string());
    }
    Some(rest.to_string())
}

/// Resolves an import target; `None` when no candidate exists (the compiler
/// then leaves the line in place).
pub fn resolve_import(including_file: &Path, target: &str, cwd: &Path) -> Option<PathBuf> {
    let with_ext = |p: PathBuf| {
        if p.extension().is_none() { vec![p.with_extension("t"), p.with_extension("csec")] } else { vec![p] }
    };
    let requested = with_ext(PathBuf::from(target));
    if requested[0].is_absolute() {
        return requested.iter().find(|p| p.is_file()).map(|p| canonical(p));
    }
    let base = including_file.parent().unwrap_or(Path::new("."));
    let mut candidates: Vec<PathBuf> = [base, cwd].iter().flat_map(|r| requested.iter().map(|p| r.join(p))).collect();
    if target.contains('.') && !target.contains('/') && !target.contains('\\') {
        let dotted = with_ext(PathBuf::from(target.replace('.', std::path::MAIN_SEPARATOR_STR)));
        candidates.extend([base, cwd].iter().flat_map(|r| dotted.iter().map(|p| r.join(p))));
    }
    candidates.into_iter().find(|c| c.is_file()).map(|c| canonical(&c))
}

/// Where each line of the expanded program came from.
#[derive(Default, Debug)]
pub struct LineMap {
    pub files: Vec<PathBuf>,
    /// Per expanded line (0-based): (index into `files`, 0-based line).
    pub lines: Vec<Option<(usize, u32)>>,
}

impl LineMap {
    /// Builds the map for compiling `main` with working directory `cwd`.
    /// `read` returns a file's text (open editor buffers or disk).
    pub fn build(main: &Path, cwd: &Path, read: &dyn Fn(&Path) -> Option<String>) -> LineMap {
        let mut b = Builder { map: LineMap::default(), cur: None, stack: Vec::new(), included: Vec::new() };
        b.map.lines.push(None);
        b.expand(&canonical(main), cwd, read);
        b.map
    }

    /// Maps a 1-based expanded line to (file, 0-based line).
    pub fn locate(&self, line1: u32) -> Option<(&Path, u32)> {
        let (f, l) = (*self.lines.get(line1.checked_sub(1)? as usize)?)?;
        Some((self.files[f].as_path(), l))
    }
}

struct Builder {
    map: LineMap,
    cur: Option<(usize, u32)>,
    stack: Vec<PathBuf>,
    included: Vec<PathBuf>,
}

impl Builder {
    fn set_origin(&mut self, origin: (usize, u32)) {
        let last = self.map.lines.last_mut().unwrap();
        if last.is_none() {
            *last = Some(origin);
        }
        self.cur = Some(origin);
    }

    fn newline(&mut self) {
        self.map.lines.push(None);
    }

    fn expand(&mut self, path: &Path, cwd: &Path, read: &dyn Fn(&Path) -> Option<String>) {
        if self.stack.iter().any(|p| p == path) || self.included.iter().any(|p| p == path) {
            return;
        }
        let Some(src) = read(path) else { return };
        let fi = self.map.files.len();
        self.map.files.push(path.to_path_buf());
        self.stack.push(path.to_path_buf());
        self.included.push(path.to_path_buf());

        let pieces: Vec<&str> = src.split('\n').collect();
        let n = pieces.len();
        for (i, line) in pieces.iter().enumerate() {
            let has_newline = i + 1 < n;
            let origin = (fi, i as u32);
            let resolved = parse_import_target(line).and_then(|t| resolve_import(path, &t, cwd));
            match resolved {
                Some(target) => {
                    self.expand(&canonical(&target), cwd, read);
                    // The compiler appends "\n" after the expansion.
                    self.set_origin(origin);
                    self.newline();
                }
                None => {
                    if !line.is_empty() {
                        self.set_origin(origin);
                    } else {
                        let last = self.map.lines.last_mut().unwrap();
                        if last.is_none() {
                            *last = Some(origin);
                        }
                    }
                    if has_newline {
                        self.newline();
                    }
                }
            }
        }
        self.stack.pop();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn import_targets() {
        assert_eq!(parse_import_target("import \"a/b\";"), Some("a/b".into()));
        assert_eq!(parse_import_target("  import 'x'  "), Some("x".into()));
        assert_eq!(parse_import_target("import lib.util;"), Some("lib.util".into()));
        assert_eq!(parse_import_target("imports"), None);
        assert_eq!(parse_import_target("import \"\";"), None);
    }

    #[test]
    fn line_map_follows_expansion() {
        let dir = std::env::temp_dir().join(format!("tessera-lsp-imports-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        std::fs::write(dir.join("lib.csec"), "def one(): Int = 1;\ndef two(): Int = 2;\n").unwrap();
        std::fs::write(dir.join("main.csec"), "import \"lib\";\ndef main(): Int {\n    return one();\n}\n").unwrap();
        let map = LineMap::build(&dir.join("main.csec"), &dir, &|p| std::fs::read_to_string(p).ok());
        // Expanded: "def one..\ndef two..\n" + "\n" + "def main(): Int {\n..."
        let (f, l) = map.locate(1).unwrap();
        assert!(f.ends_with("lib.csec") && l == 0);
        let (f, l) = map.locate(2).unwrap();
        assert!(f.ends_with("lib.csec") && l == 1);
        let (f, l) = map.locate(4).unwrap();
        assert!(f.ends_with("main.csec") && l == 1, "{f:?} {l}");
        let (f, l) = map.locate(5).unwrap();
        assert!(f.ends_with("main.csec") && l == 2);
        std::fs::remove_dir_all(&dir).ok();
    }
}
