//! Runs `tess` on a saved file and turns its output into diagnostics.
//!
//! `tess` stops at the first parse error and reports type errors without a
//! position, so: parse/lex errors are mapped exactly (through the import line
//! map), and type errors are anchored on the declaration of the first name
//! quoted in the message.

use crate::imports::LineMap;
use crate::text::LineIndex;
use lsp_types::{Diagnostic, DiagnosticSeverity, NumberOrString};
use serde::Deserialize;
use std::collections::HashMap;
use std::io::Read;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::time::{Duration, Instant};
use tessera_syntax::{Analysis, DeclKind, TokKind};

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase", default)]
pub struct Settings {
    /// Path of `tess`; found automatically when empty.
    pub compiler_path: Option<String>,
    /// Extra arguments, e.g. `["--no-strict-ownership"]`.
    pub compiler_args: Vec<String>,
    /// `"full"` (parse + type check, default) or `"syntax"` (`--syntax-only`).
    pub check_mode: String,
    pub check_on_save: bool,
    pub check_on_open: bool,
    pub check_timeout_ms: u64,
}

impl Default for Settings {
    fn default() -> Self {
        Settings {
            compiler_path: None,
            compiler_args: Vec::new(),
            check_mode: "full".into(),
            check_on_save: true,
            check_on_open: true,
            check_timeout_ms: 30_000,
        }
    }
}

fn exe(name: &str) -> String {
    if cfg!(windows) { format!("{name}.exe") } else { name.to_string() }
}

/// Finds `tess`: settings, `TESSERA_COMPILER` / `CSEC_COMPILER`, the
/// workspace's build outputs, then `PATH`.
pub fn find_compiler(settings: &Settings, roots: &[PathBuf]) -> Option<PathBuf> {
    if let Some(p) = settings.compiler_path.as_deref().filter(|p| !p.is_empty()) {
        let p = PathBuf::from(p);
        return p.is_file().then_some(p);
    }
    for var in ["TESSERA_COMPILER", "CSEC_COMPILER"] {
        if let Some(p) = std::env::var_os(var).map(PathBuf::from).filter(|p| p.is_file()) {
            return Some(p);
        }
    }
    let name = exe("tess");
    for root in roots {
        for dir in ["x64/Release", "x64/Debug", "build", "build/Release", "build/Debug", "out/build", "bin"] {
            let p = dir.split('/').fold(root.clone(), |acc, part| acc.join(part)).join(&name);
            if p.is_file() {
                return Some(p);
            }
        }
    }
    std::env::var_os("PATH")
        .into_iter()
        .flat_map(|paths| std::env::split_paths(&paths).collect::<Vec<_>>())
        .map(|d| d.join(&name))
        .find(|p| p.is_file())
}

pub struct Output {
    pub code: Option<i32>,
    pub text: String,
    pub timed_out: bool,
}

pub fn run(compiler: &Path, settings: &Settings, file: &Path, cwd: &Path) -> std::io::Result<Output> {
    static COUNTER: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
    let n = COUNTER.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
    let tmp = std::env::temp_dir().join(format!("tessera-lsp-{}-{n}.ll", std::process::id()));

    let mut cmd = Command::new(compiler);
    if settings.check_mode == "syntax" {
        cmd.arg("--syntax-only");
    } else {
        cmd.arg("--emit-ir").arg("-o").arg(&tmp);
    }
    cmd.args(&settings.compiler_args)
        .arg(file)
        .current_dir(cwd)
        .stdin(Stdio::null())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        const CREATE_NO_WINDOW: u32 = 0x0800_0000;
        cmd.creation_flags(CREATE_NO_WINDOW);
    }
    let mut child = cmd.spawn()?;
    let mut so = child.stdout.take().unwrap();
    let mut se = child.stderr.take().unwrap();
    let t1 = std::thread::spawn(move || {
        let mut s = Vec::new();
        so.read_to_end(&mut s).ok();
        s
    });
    let t2 = std::thread::spawn(move || {
        let mut s = Vec::new();
        se.read_to_end(&mut s).ok();
        s
    });
    let deadline = Instant::now() + Duration::from_millis(settings.check_timeout_ms.max(1000));
    let mut timed_out = false;
    let status = loop {
        if let Some(st) = child.try_wait()? {
            break Some(st);
        }
        if Instant::now() >= deadline {
            child.kill().ok();
            child.wait().ok();
            timed_out = true;
            break None;
        }
        std::thread::sleep(Duration::from_millis(25));
    };
    let mut text = String::from_utf8_lossy(&t1.join().unwrap_or_default()).into_owned();
    text.push('\n');
    text.push_str(&String::from_utf8_lossy(&t2.join().unwrap_or_default()));
    std::fs::remove_file(&tmp).ok();
    Ok(Output { code: status.and_then(|s| s.code()), text, timed_out })
}

/// Diagnostics grouped by file.
pub type FileDiagnostics = HashMap<PathBuf, Vec<Diagnostic>>;

struct FileText {
    text: String,
    index: LineIndex,
    analysis: Analysis,
}

/// Loaded on demand while mapping positions.
struct Files<'a> {
    cache: HashMap<PathBuf, Option<FileText>>,
    read: &'a dyn Fn(&Path) -> Option<String>,
}

impl Files<'_> {
    fn get(&mut self, p: &Path) -> Option<&FileText> {
        let read = self.read;
        self.cache
            .entry(p.to_path_buf())
            .or_insert_with(|| {
                read(p).map(|text| {
                    let index = LineIndex::new(&text);
                    let analysis = Analysis::new(&text);
                    FileText { text, index, analysis }
                })
            })
            .as_ref()
    }
}

/// `"... at line L, column C"` → (message, L, C).
fn split_location(msg: &str) -> Option<(&str, u32, u32)> {
    let at = msg.rfind(" at line ")?;
    let rest = &msg[at + " at line ".len()..];
    let (l, c) = rest.split_once(", column ")?;
    let line = l.trim().parse().ok()?;
    let col = c.trim().trim_end_matches('.').parse().ok()?;
    Some((&msg[..at], line, col))
}

fn quoted_names(msg: &str) -> Vec<&str> {
    let mut out = Vec::new();
    let mut rest = msg;
    while let Some(s) = rest.find('\'') {
        let after = &rest[s + 1..];
        let Some(e) = after.find('\'') else { break };
        let name = &after[..e];
        if !name.is_empty()
            && name.chars().next().is_some_and(|c| c == '_' || c.is_alphabetic())
            && name.chars().all(|c| c == '_' || c.is_alphanumeric())
        {
            out.push(name);
        }
        rest = &after[e + 1..];
    }
    out
}

fn diag(range: lsp_types::Range, severity: DiagnosticSeverity, code: &str, message: String) -> Diagnostic {
    Diagnostic {
        range,
        severity: Some(severity),
        code: Some(NumberOrString::String(code.into())),
        source: Some("tess".into()),
        message,
        ..Default::default()
    }
}

/// Turns compiler output into diagnostics for `main` and the files it imports.
pub fn parse_output(
    out: &Output,
    main: &Path,
    map: &LineMap,
    read: &dyn Fn(&Path) -> Option<String>,
    compiler_name: &str,
) -> FileDiagnostics {
    let mut files = Files { cache: HashMap::new(), read };
    let mut result: FileDiagnostics = HashMap::new();
    let mut seen = std::collections::HashSet::new();
    let zero = lsp_types::Range::default();
    let main = main.to_path_buf();

    if out.timed_out {
        result.entry(main.clone()).or_default().push(diag(
            zero,
            DiagnosticSeverity::INFORMATION,
            "timeout",
            format!("{compiler_name} did not finish in time; no compiler diagnostics."),
        ));
        return result;
    }

    // The first line of `main` (fallback anchor).
    let first_line = |files: &mut Files, p: &Path| -> lsp_types::Range {
        files
            .get(p)
            .map(|f| {
                let r = f.index.line_range(&f.text, 0);
                f.index.range(&f.text, r)
            })
            .unwrap_or_default()
    };

    for raw in out.text.lines() {
        let line = raw.trim();
        if line.is_empty() {
            continue;
        }
        let line = line.strip_prefix("Parsing failed: ").unwrap_or(line);
        if !seen.insert(line.to_string()) {
            continue;
        }

        if let Some(rest) = line.strip_prefix("Parser Error: ").or_else(|| line.strip_prefix("Lexing failed: ")) {
            let lexing = line.starts_with("Lexing");
            let Some((msg, l, c)) = split_location(rest) else {
                let r = first_line(&mut files, &main);
                result.entry(main.clone()).or_default().push(diag(r, DiagnosticSeverity::ERROR, "parse", rest.to_string()));
                continue;
            };
            let (file, line0) = map.locate(l).map(|(f, l)| (f.to_path_buf(), l)).unwrap_or((main.clone(), l.saturating_sub(1)));
            let range = files
                .get(&file)
                .map(|f| {
                    let lr = f.index.line_range(&f.text, line0 as usize);
                    // tess columns are 1-based bytes. Lexing: token start. Parsing: one past the token end.
                    let at = (lr.start + (c.saturating_sub(1)) as usize).min(lr.end.max(lr.start));
                    let r = if lexing {
                        let end = f.text[at..].chars().next().map_or(at, |ch| at + ch.len_utf8());
                        at..end
                    } else if let Some(t) = f.analysis.tokens.iter().find(|t| t.range.end == at && !t.is_trivia()) {
                        t.range.clone()
                    } else if at > lr.start {
                        let prev = f.text[..at].chars().next_back().map_or(1, char::len_utf8);
                        at - prev..at
                    } else {
                        at..at
                    };
                    f.index.range(&f.text, r)
                })
                .unwrap_or_default();
            result
                .entry(file)
                .or_default()
                .push(diag(range, DiagnosticSeverity::ERROR, if lexing { "lex" } else { "parse" }, msg.to_string()));
            continue;
        }

        if let Some(msg) = line.strip_prefix("Type error: ") {
            let (file, range) = anchor_type_error(msg, &main, map, &mut files).unwrap_or_else(|| (main.clone(), first_line(&mut files, &main)));
            result.entry(file).or_default().push(diag(range, DiagnosticSeverity::ERROR, "type", msg.to_string()));
            continue;
        }

        // `Error (file:line): message` (default/named arguments, enums, interfaces, closures) and
        // `Error (line N): message` (a line of the import-expanded program).
        if let Some(rest) = line.strip_prefix("Error (") {
            if let Some(close) = rest.find("): ") {
                let place = &rest[..close];
                let msg = &rest[close + 3..];
                let located = if let Some(n) = place.strip_prefix("line ") {
                    n.trim().parse::<u32>().ok().map(|l| {
                        map.locate(l).map(|(f, l)| (f.to_path_buf(), l)).unwrap_or((main.clone(), l.saturating_sub(1)))
                    })
                } else {
                    place.rfind(':').and_then(|colon| {
                        let file = PathBuf::from(&place[..colon]);
                        place[colon + 1..].trim().parse::<u32>().ok().map(|l| (file, l.saturating_sub(1)))
                    })
                };
                if let Some((file, line0)) = located {
                    let range = files
                        .get(&file)
                        .map(|f| {
                            let lr = f.index.line_range(&f.text, line0 as usize);
                            f.index.range(&f.text, lr)
                        })
                        .unwrap_or_default();
                    result.entry(file).or_default().push(diag(range, DiagnosticSeverity::ERROR, "semantic", msg.to_string()));
                    continue;
                }
            }
        }
        if let Some(msg) = line.strip_prefix("Error: ") {
            let (file, range) = anchor_type_error(msg, &main, map, &mut files).unwrap_or_else(|| (main.clone(), first_line(&mut files, &main)));
            result.entry(file).or_default().push(diag(range, DiagnosticSeverity::ERROR, "semantic", msg.to_string()));
            continue;
        }

        if let Some(path) = line.strip_prefix("Unable to read import: ").or_else(|| line.strip_prefix("Import cycle detected at ")) {
            let stem = Path::new(path.trim()).file_stem().map(|s| s.to_string_lossy().into_owned()).unwrap_or_default();
            let range = files
                .get(&main)
                .and_then(|f| {
                    (0..f.index.line_count()).find_map(|i| {
                        let lr = f.index.line_range(&f.text, i);
                        let l = &f.text[lr.clone()];
                        (l.trim_start().starts_with("import") && l.contains(&stem)).then(|| f.index.range(&f.text, lr))
                    })
                })
                .unwrap_or_else(|| first_line(&mut files, &main));
            result.entry(main.clone()).or_default().push(diag(range, DiagnosticSeverity::ERROR, "import", line.to_string()));
            continue;
        }

        if line.starts_with("Generated LLVM IR is invalid") {
            let r = first_line(&mut files, &main);
            result.entry(main.clone()).or_default().push(diag(
                r,
                DiagnosticSeverity::WARNING,
                "codegen",
                format!("{line} (this is a compiler code-generation problem, not necessarily an error in this file)"),
            ));
            continue;
        }
        if line.starts_with("Unable to read input file") {
            let r = first_line(&mut files, &main);
            result.entry(main.clone()).or_default().push(diag(r, DiagnosticSeverity::ERROR, "io", line.to_string()));
        }
    }

    let failed = out.code.is_some_and(|c| c != 0) || out.code.is_none();
    if failed && result.values().all(|v| v.is_empty()) {
        let tail: Vec<&str> = out
            .text
            .lines()
            .map(str::trim)
            .filter(|l| !l.is_empty() && !l.starts_with("get current directory") && !l.starts_with("Parsing completed"))
            .collect();
        let tail = tail[tail.len().saturating_sub(6)..].join("\n");
        let why = match out.code {
            Some(c) if (c as u32) >= 0xC000_0000 => format!("{compiler_name} crashed (exit 0x{:08X})", c as u32),
            Some(c) => format!("{compiler_name} failed (exit {c})"),
            None => format!("{compiler_name} was terminated"),
        };
        let msg = if tail.is_empty() { why } else { format!("{why}:\n{tail}") };
        let r = first_line(&mut files, &main);
        result.entry(main).or_default().push(diag(r, DiagnosticSeverity::WARNING, "compiler", msg));
    }
    result
}

/// Type errors carry no position: anchor on the declaration of a quoted name
/// (`Variable 'x' ...`, `Function 'f' ...`, `use of moved value 'v'`).
fn anchor_type_error(msg: &str, main: &Path, map: &LineMap, files: &mut Files) -> Option<(PathBuf, lsp_types::Range)> {
    let names = quoted_names(msg);
    let mut order: Vec<PathBuf> = vec![main.to_path_buf()];
    order.extend(map.files.iter().filter(|f| f.as_path() != main).cloned());
    let wants_fn = msg.starts_with("Function ") || msg.contains("argument") || msg.contains("ownership transfer to");
    for name in names {
        for file in &order {
            let Some(f) = files.get(file) else { continue };
            let mut decls: Vec<_> = f.analysis.decls.iter().filter(|d| d.name == name).collect();
            if decls.is_empty() {
                continue;
            }
            decls.sort_by_key(|d| {
                let callable = d.kind.is_callable();
                (callable != wants_fn, matches!(d.kind, DeclKind::Field | DeclKind::Parameter))
            });
            let d = decls[0];
            // For moves/borrows, point at the last use instead: that's where the error usually is.
            let r = if msg.contains("moved") || msg.contains("borrow") {
                f.analysis
                    .tokens
                    .iter()
                    .rev()
                    .find(|t| t.kind == TokKind::Ident && t.text(&f.text) == name && t.range.start >= d.name_range.start && t.range.end <= d.scope.end.min(f.text.len()))
                    .map(|t| t.range.clone())
                    .unwrap_or(d.name_range.clone())
            } else {
                d.name_range.clone()
            };
            return Some((file.clone(), f.index.range(&f.text, r)));
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn locations_and_names() {
        assert_eq!(
            split_location("Expected ')', but found 'return' at line 4, column 9"),
            Some(("Expected ')', but found 'return'", 4, 9))
        );
        assert_eq!(quoted_names("Variable 'x' declared as 'Int' but initialized with 'String'"), vec!["x", "Int", "String"]);
    }

    #[test]
    fn maps_located_errors() {
        let dir = std::env::temp_dir().join("tessera-lsp-located");
        std::fs::create_dir_all(&dir).unwrap();
        let main = dir.join("m.csec");
        std::fs::write(&main, "def f(a: Int, b: Int = 1): Int = a + b
def main(): Int { return f(b = 2); }
").unwrap();
        let out = Output {
            text: format!("Error ({}:2): call to 'f': f is missing a value for parameter 'a'
", main.display()),
            code: Some(1),
            timed_out: false,
        };
        let map = LineMap::default();
        let read = |p: &Path| std::fs::read_to_string(p).ok();
        let d = parse_output(&out, &main, &map, &read, "tess");
        let list = d.get(&main).expect("diagnostics for the main file");
        assert_eq!(list.len(), 1);
        assert_eq!(list[0].range.start.line, 1);
        assert!(list[0].message.contains("missing a value"));
    }

    #[test]
    fn maps_parser_errors() {
        let dir = std::env::temp_dir().join(format!("tessera-lsp-compiler-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        let main = dir.join("bad.csec");
        let src = "def main(): Int {\n  val x: Int = 1\n  val y = (x + ;\n  return 0;\n}\n";
        std::fs::write(&main, src).unwrap();
        let main = crate::imports::canonical(&main);
        let read = |p: &Path| std::fs::read_to_string(p).ok();
        let map = LineMap::build(&main, &dir, &read);
        let out = Output {
            code: Some(1),
            text: "get current directory: x\nParser Error: Expected ')', but found 'return' at line 4, column 9\nParsing failed: Parser Error: Expected ')', but found 'return' at line 4, column 9\n".into(),
            timed_out: false,
        };
        let d = parse_output(&out, &main, &map, &read, "tess");
        let v = &d[&main];
        assert_eq!(v.len(), 1);
        assert_eq!(v[0].range, lsp_types::Range::new(lsp_types::Position::new(3, 2), lsp_types::Position::new(3, 8)));

        let out = Output {
            code: Some(1),
            text: "Type error: Variable 'y' declared as 'Int' but initialized with 'String'\nType checking found 1 error(s). Aborting code generation.\n".into(),
            timed_out: false,
        };
        let d = parse_output(&out, &main, &map, &read, "tess");
        assert_eq!(d[&main][0].range.start, lsp_types::Position::new(2, 6));
        std::fs::remove_dir_all(&dir).ok();
    }
}
