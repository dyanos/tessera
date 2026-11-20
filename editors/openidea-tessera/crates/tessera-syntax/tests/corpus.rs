//! Runs every analysis over the repository's Tessera sources: nothing may
//! panic, the lexer must accept what `tess` accepts, and highlight ranges
//! must be sorted and disjoint.

use std::path::{Path, PathBuf};
use tessera_syntax::*;

fn collect(dir: &Path, out: &mut Vec<PathBuf>) {
    let Ok(rd) = std::fs::read_dir(dir) else { return };
    for e in rd.flatten() {
        let p = e.path();
        if p.is_dir() {
            collect(&p, out);
        } else if p.extension().is_some_and(|x| x == "csec" || x == "t") {
            out.push(p);
        }
    }
}

#[test]
fn repository_sources() {
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../..");
    let mut files = Vec::new();
    for d in ["tests/positive", "examples", "selfhost/src"] {
        collect(&root.join(d), &mut files);
    }
    if files.is_empty() {
        eprintln!("no corpus found under {}", root.display());
        return;
    }
    let mut decl_count = 0;
    for f in &files {
        let Ok(src) = std::fs::read_to_string(f) else { continue };
        let a = Analysis::new(&src);
        let unknown: Vec<_> = a.tokens.iter().filter(|t| t.kind == TokKind::Unknown).map(|t| t.text(&src)).collect();
        assert!(unknown.is_empty(), "{}: unknown tokens {:?}", f.display(), unknown);
        let hl = highlight(&src, &a.tokens);
        for w in hl.windows(2) {
            assert!(w[0].0.end <= w[1].0.start, "{}: overlap {:?}", f.display(), w);
        }
        for d in &a.decls {
            assert!(d.name_range.end <= src.len() && d.range.start <= d.name_range.start, "{}: {:?}", f.display(), d);
            assert!(d.parent.map_or(true, |p| p < a.decls.len()));
        }
        decl_count += a.decls.len();
        let _ = folds(&src, &a.tokens);
        let _ = lex_problems(&src, &a.tokens);
    }
    eprintln!("{} files, {} declarations", files.len(), decl_count);
}
