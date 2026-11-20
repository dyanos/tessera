//! `tessera-lsp` ??a Language Server Protocol server for Tessera (`.t`, `.csec`).
//!
//! Speaks LSP over stdio. Lexical problems are reported as you type;
//! `tess` runs in the background on open and save for parse and type
//! errors. Run `tessera-lsp --version` to check the install.

mod compiler;
mod features;
mod imports;
mod text;

use compiler::Settings;
use crossbeam_channel::Sender;
use features::Source;
use lsp_server::{Connection, ErrorCode, Message, Notification, Request, Response};
use lsp_types::notification::Notification as _;
use lsp_types::request::Request as _;
use lsp_types::*;
use serde_json::Value;
use std::collections::{HashMap, HashSet};
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use std::time::{Instant, SystemTime};

/// Diagnostics per file, merged from the lexer (open documents) and from
/// compiler runs (keyed by the file whose check produced them).
#[derive(Default)]
struct DiagStore {
    uris: HashMap<String, Url>,
    versions: HashMap<String, i32>,
    lex: HashMap<String, Vec<Diagnostic>>,
    compiler: HashMap<String, HashMap<String, Vec<Diagnostic>>>,
}

impl DiagStore {
    fn publish(&self, key: &str, sender: &Sender<Message>) {
        let Some(uri) = self.uris.get(key) else { return };
        let mut diagnostics: Vec<Diagnostic> = self.lex.get(key).cloned().unwrap_or_default();
        if let Some(by_owner) = self.compiler.get(key) {
            let mut seen = HashSet::new();
            for d in by_owner.values().flatten() {
                if seen.insert((d.range.start, d.message.clone())) {
                    diagnostics.push(d.clone());
                }
            }
        }
        let params = PublishDiagnosticsParams { uri: uri.clone(), diagnostics, version: self.versions.get(key).copied() };
        send_notification::<notification::PublishDiagnostics>(sender, params);
    }

    /// Replaces the results of `owner`'s check; returns the files to republish.
    fn set_compiler(&mut self, owner: &str, results: HashMap<String, (Url, Vec<Diagnostic>)>) -> Vec<String> {
        let mut touched: HashSet<String> = HashSet::new();
        for (file, by_owner) in self.compiler.iter_mut() {
            if by_owner.remove(owner).is_some() {
                touched.insert(file.clone());
            }
        }
        for (file, (uri, diags)) in results {
            self.uris.entry(file.clone()).or_insert(uri);
            self.compiler.entry(file.clone()).or_default().insert(owner.to_string(), diags);
            touched.insert(file);
        }
        touched.into_iter().collect()
    }
}

/// Normalised key for a path (case-insensitive on Windows).
fn path_key(p: &Path) -> String {
    let s = imports::canonical(p).to_string_lossy().into_owned();
    if cfg!(windows) { s.to_lowercase() } else { s }
}

fn uri_key(uri: &Url) -> String {
    uri.to_file_path().map(|p| path_key(&p)).unwrap_or_else(|_| uri.to_string())
}

fn send_notification<N: notification::Notification>(sender: &Sender<Message>, params: N::Params) {
    let not = Notification::new(N::METHOD.to_string(), params);
    sender.send(Message::Notification(not)).ok();
}

fn log(sender: &Sender<Message>, typ: MessageType, message: impl Into<String>) {
    send_notification::<notification::LogMessage>(sender, LogMessageParams { typ, message: message.into() });
}

struct Server {
    sender: Sender<Message>,
    docs: HashMap<Url, (i32, Arc<Source>)>,
    roots: Vec<PathBuf>,
    settings: Settings,
    compiler: Option<PathBuf>,
    warned_no_compiler: bool,
    snippets: bool,
    diags: Arc<Mutex<DiagStore>>,
    generations: Arc<Mutex<HashMap<String, u64>>>,
    /// Imported files read from disk: path ??(mtime, source).
    disk_cache: HashMap<PathBuf, (Option<SystemTime>, Arc<Source>)>,
    /// Workspace files for workspace/symbol, refreshed at most every 30 s.
    workspace_index: Option<(Instant, Vec<Arc<Source>>)>,
}

impl Server {
    fn root_for(&self, path: &Path) -> PathBuf {
        self.roots
            .iter()
            .filter(|r| path.starts_with(r))
            .max_by_key(|r| r.as_os_str().len())
            .cloned()
            .or_else(|| path.parent().map(Path::to_path_buf))
            .unwrap_or_else(|| PathBuf::from("."))
    }

    fn find_compiler(&mut self) {
        self.compiler = compiler::find_compiler(&self.settings, &self.roots);
        match &self.compiler {
            Some(c) => log(&self.sender, MessageType::INFO, format!("tessera-lsp: using compiler {}", c.display())),
            None => log(
                &self.sender,
                MessageType::INFO,
                "tessera-lsp: tess not found (set initializationOptions.compilerPath or TESSERA_COMPILER); only lexical diagnostics are reported",
            ),
        }
    }

    /// Source for a file: the open buffer if any, else disk (cached by mtime).
    fn source_for_path(&mut self, path: &Path) -> Option<Arc<Source>> {
        let key = path_key(path);
        if let Some((_, s)) = self.docs.values().find(|(_, s)| s.path.as_deref().is_some_and(|p| path_key(p) == key)) {
            return Some(s.clone());
        }
        let mtime = std::fs::metadata(path).and_then(|m| m.modified()).ok();
        if let Some((t, s)) = self.disk_cache.get(path) {
            if *t == mtime {
                return Some(s.clone());
            }
        }
        let text = std::fs::read_to_string(path).ok()?;
        let uri = Url::from_file_path(path).ok()?;
        let s = Arc::new(Source::new(uri, text));
        self.disk_cache.insert(path.to_path_buf(), (mtime, s.clone()));
        Some(s)
    }

    /// Files `src` imports, transitively.
    fn imports_of(&mut self, src: &Source) -> Vec<Arc<Source>> {
        let Some(path) = src.path.clone() else { return Vec::new() };
        let cwd = self.root_for(&path);
        let mut out = Vec::new();
        let mut seen: HashSet<String> = HashSet::from([path_key(&path)]);
        let mut queue: Vec<(PathBuf, String)> = vec![(path, src.text.clone())];
        while let Some((file, text)) = queue.pop() {
            for line in text.lines() {
                let Some(target) = imports::parse_import_target(line) else { continue };
                let Some(p) = imports::resolve_import(&file, &target, &cwd) else { continue };
                if !seen.insert(path_key(&p)) || out.len() > 200 {
                    continue;
                }
                if let Some(s) = self.source_for_path(&p) {
                    queue.push((p, s.text.clone()));
                    out.push(s);
                }
            }
        }
        out
    }

    fn doc(&self, uri: &Url) -> Option<Arc<Source>> {
        self.docs.get(uri).map(|(_, s)| s.clone())
    }

    fn publish_lex(&self, uri: &Url) {
        let Some((version, src)) = self.docs.get(uri) else { return };
        let diags: Vec<Diagnostic> = tessera_syntax::lex_problems(&src.text, &src.analysis.tokens)
            .into_iter()
            .map(|p| Diagnostic {
                range: src.range(p.range),
                severity: Some(match p.severity {
                    tessera_syntax::Severity::Error => DiagnosticSeverity::ERROR,
                    tessera_syntax::Severity::Warning => DiagnosticSeverity::WARNING,
                }),
                code: Some(NumberOrString::String(p.code.into())),
                source: Some("tessera".into()),
                message: p.message,
                ..Default::default()
            })
            .collect();
        let key = uri_key(uri);
        let mut store = self.diags.lock().unwrap();
        store.uris.insert(key.clone(), uri.clone());
        store.versions.insert(key.clone(), *version);
        store.lex.insert(key.clone(), diags);
        store.publish(&key, &self.sender);
    }

    /// Runs `tess` on the saved file in the background.
    fn check(&mut self, uri: &Url) {
        let Ok(path) = uri.to_file_path() else { return };
        let Some(compiler) = self.compiler.clone() else {
            if !self.warned_no_compiler {
                self.warned_no_compiler = true;
                log(&self.sender, MessageType::WARNING, "tessera-lsp: no tess compiler found; skipping compiler diagnostics");
            }
            return;
        };
        let owner = uri_key(uri);
        let generation = {
            let mut g = self.generations.lock().unwrap();
            let e = g.entry(owner.clone()).or_insert(0);
            *e += 1;
            *e
        };
        let cwd = self.root_for(&path);
        let settings = self.settings.clone();
        let sender = self.sender.clone();
        let diags = self.diags.clone();
        let generations = self.generations.clone();
        let main_uri = uri.clone();
        std::thread::spawn(move || {
            let main = imports::canonical(&path);
            let read = |p: &Path| std::fs::read_to_string(p).ok();
            let map = imports::LineMap::build(&main, &cwd, &read);
            let name = compiler.file_name().map(|n| n.to_string_lossy().into_owned()).unwrap_or_else(|| "tess".into());
            let by_file = match compiler::run(&compiler, &settings, &main, &cwd) {
                Ok(out) => compiler::parse_output(&out, &main, &map, &read, &name),
                Err(e) => {
                    log(&sender, MessageType::ERROR, format!("tessera-lsp: failed to run {}: {e}", compiler.display()));
                    return;
                }
            };
            if generations.lock().unwrap().get(&owner) != Some(&generation) {
                return; // a newer check is running
            }
            let main_key = path_key(&main);
            let results: HashMap<String, (Url, Vec<Diagnostic>)> = by_file
                .into_iter()
                .filter_map(|(p, d)| {
                    let key = path_key(&p);
                    let uri = if key == main_key { main_uri.clone() } else { Url::from_file_path(&p).ok()? };
                    Some((key, (uri, d)))
                })
                .collect();
            let mut store = diags.lock().unwrap();
            for key in store.set_compiler(&owner, results) {
                store.publish(&key, &sender);
            }
        });
    }

    fn workspace_sources(&mut self) -> Vec<Arc<Source>> {
        if let Some((t, v)) = &self.workspace_index {
            if t.elapsed().as_secs() < 30 {
                let mut v = v.clone();
                for (_, s) in self.docs.values() {
                    v.retain(|x| x.uri != s.uri);
                    v.push(s.clone());
                }
                return v;
            }
        }
        const SKIP: &[&str] = &["target", "build", "out", "node_modules", "vcpkg_installed", "x64", "Debug", "Release"];
        let mut files = Vec::new();
        let mut stack: Vec<PathBuf> = self.roots.clone();
        while let Some(dir) = stack.pop() {
            let Ok(rd) = std::fs::read_dir(&dir) else { continue };
            for e in rd.flatten() {
                let p = e.path();
                let name = e.file_name().to_string_lossy().into_owned();
                if p.is_dir() {
                    if !name.starts_with('.') && !name.starts_with('_') && !SKIP.contains(&name.as_str()) && !name.starts_with("build") {
                        stack.push(p);
                    }
                } else if (name.ends_with(".t") || name.ends_with(".csec")) && files.len() < 5000 {
                    files.push(p);
                }
            }
        }
        let mut v: Vec<Arc<Source>> = files.iter().filter_map(|p| self.source_for_path(p)).collect();
        self.workspace_index = Some((Instant::now(), v.clone()));
        for (_, s) in self.docs.values() {
            v.retain(|x| x.uri != s.uri);
            v.push(s.clone());
        }
        v
    }

    fn other_open_docs(&self, uri: &Url) -> Vec<Arc<Source>> {
        self.docs.iter().filter(|(u, _)| *u != uri).map(|(_, (_, s))| s.clone()).collect()
    }

    // ------------------------------------------------------------ dispatch

    fn on_notification(&mut self, not: Notification) {
        match not.method.as_str() {
            notification::DidOpenTextDocument::METHOD => {
                let Ok(p) = serde_json::from_value::<DidOpenTextDocumentParams>(not.params) else { return };
                let doc = p.text_document;
                self.docs.insert(doc.uri.clone(), (doc.version, Arc::new(Source::new(doc.uri.clone(), doc.text))));
                self.publish_lex(&doc.uri);
                if self.settings.check_on_open {
                    self.check(&doc.uri);
                }
            }
            notification::DidChangeTextDocument::METHOD => {
                let Ok(p) = serde_json::from_value::<DidChangeTextDocumentParams>(not.params) else { return };
                let uri = p.text_document.uri;
                let Some(change) = p.content_changes.into_iter().last() else { return };
                // Full sync (we advertise TextDocumentSyncKind::FULL).
                self.docs.insert(uri.clone(), (p.text_document.version, Arc::new(Source::new(uri.clone(), change.text))));
                self.publish_lex(&uri);
            }
            notification::DidSaveTextDocument::METHOD => {
                let Ok(p) = serde_json::from_value::<DidSaveTextDocumentParams>(not.params) else { return };
                self.disk_cache.clear();
                if self.settings.check_on_save {
                    self.check(&p.text_document.uri);
                }
            }
            notification::DidCloseTextDocument::METHOD => {
                let Ok(p) = serde_json::from_value::<DidCloseTextDocumentParams>(not.params) else { return };
                let uri = p.text_document.uri;
                self.docs.remove(&uri);
                let key = uri_key(&uri);
                let mut store = self.diags.lock().unwrap();
                store.lex.remove(&key);
                let touched = store.set_compiler(&key, HashMap::new());
                store.compiler.remove(&key);
                store.versions.remove(&key);
                send_notification::<notification::PublishDiagnostics>(
                    &self.sender,
                    PublishDiagnosticsParams { uri: uri.clone(), diagnostics: Vec::new(), version: None },
                );
                for k in touched {
                    if k != key {
                        store.publish(&k, &self.sender);
                    }
                }
            }
            notification::DidChangeConfiguration::METHOD => {
                let Ok(p) = serde_json::from_value::<DidChangeConfigurationParams>(not.params) else { return };
                let v = p.settings.get("tessera").cloned().unwrap_or(p.settings);
                if let Ok(s) = serde_json::from_value::<Settings>(v) {
                    self.settings = s;
                    self.find_compiler();
                }
            }
            _ => {}
        }
    }

    fn on_request(&mut self, req: Request) -> Response {
        let id = req.id.clone();
        let method = req.method.clone();
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| self.handle(req)));
        match result {
            Ok(Ok(v)) => Response::new_ok(id, v),
            Ok(Err((code, msg))) => Response::new_err(id, code as i32, msg),
            Err(_) => Response::new_err(id, ErrorCode::InternalError as i32, format!("tessera-lsp: internal error in {method}")),
        }
    }

    fn handle(&mut self, req: Request) -> Result<Value, (ErrorCode, String)> {
        fn params<P: serde::de::DeserializeOwned>(v: Value) -> Result<P, (ErrorCode, String)> {
            serde_json::from_value(v).map_err(|e| (ErrorCode::InvalidParams, e.to_string()))
        }
        let ok = |v: Value| Ok(v);
        fn json<T: serde::Serialize>(v: T) -> Value {
            serde_json::to_value(v).unwrap_or(Value::Null)
        }
        macro_rules! doc_or_null {
            ($uri:expr) => {
                match self.doc($uri) {
                    Some(d) => d,
                    None => return ok(Value::Null),
                }
            };
        }
        match req.method.as_str() {
            request::DocumentSymbolRequest::METHOD => {
                let p: DocumentSymbolParams = params(req.params)?;
                let d = doc_or_null!(&p.text_document.uri);
                ok(json(DocumentSymbolResponse::Nested(features::document_symbols(&d))))
            }
            request::FoldingRangeRequest::METHOD => {
                let p: FoldingRangeParams = params(req.params)?;
                let d = doc_or_null!(&p.text_document.uri);
                ok(json(features::folding_ranges(&d)))
            }
            request::SemanticTokensFullRequest::METHOD => {
                let p: SemanticTokensParams = params(req.params)?;
                let d = doc_or_null!(&p.text_document.uri);
                ok(json(SemanticTokensResult::Tokens(SemanticTokens { result_id: None, data: features::semantic_tokens(&d) })))
            }
            request::HoverRequest::METHOD => {
                let p: HoverParams = params(req.params)?;
                let tp = p.text_document_position_params;
                let d = doc_or_null!(&tp.text_document.uri);
                let imps = self.imports_of(&d);
                ok(json(features::hover(&d, &imps, tp.position)))
            }
            request::GotoDefinition::METHOD | request::GotoDeclaration::METHOD => {
                let p: GotoDefinitionParams = params(req.params)?;
                let tp = p.text_document_position_params;
                let d = doc_or_null!(&tp.text_document.uri);
                let imps = self.imports_of(&d);
                let path = d.path.clone();
                let cwd = path.as_deref().map(|p| self.root_for(p)).unwrap_or_default();
                let resolve = |t: &str| path.as_deref().and_then(|p| imports::resolve_import(p, t, &cwd));
                ok(json(GotoDefinitionResponse::Array(features::definition(&d, &imps, tp.position, &resolve))))
            }
            request::References::METHOD => {
                let p: ReferenceParams = params(req.params)?;
                let tp = p.text_document_position;
                let d = doc_or_null!(&tp.text_document.uri);
                let imps = self.imports_of(&d);
                let others = self.other_open_docs(&d.uri);
                ok(json(features::references(&d, &imps, &others, tp.position, p.context.include_declaration)))
            }
            request::DocumentHighlightRequest::METHOD => {
                let p: DocumentHighlightParams = params(req.params)?;
                let tp = p.text_document_position_params;
                let d = doc_or_null!(&tp.text_document.uri);
                let imps = self.imports_of(&d);
                ok(json(features::document_highlights(&d, &imps, tp.position)))
            }
            request::Completion::METHOD => {
                let p: CompletionParams = params(req.params)?;
                let tp = p.text_document_position;
                let d = doc_or_null!(&tp.text_document.uri);
                let imps = self.imports_of(&d);
                ok(json(CompletionResponse::Array(features::completion(&d, &imps, tp.position, self.snippets))))
            }
            request::SignatureHelpRequest::METHOD => {
                let p: SignatureHelpParams = params(req.params)?;
                let tp = p.text_document_position_params;
                let d = doc_or_null!(&tp.text_document.uri);
                let imps = self.imports_of(&d);
                ok(json(features::signature_help(&d, &imps, tp.position)))
            }
            request::PrepareRenameRequest::METHOD => {
                let p: TextDocumentPositionParams = params(req.params)?;
                let d = doc_or_null!(&p.text_document.uri);
                let imps = self.imports_of(&d);
                match features::prepare_rename(&d, &imps, p.position) {
                    Ok(r) => ok(json(PrepareRenameResponse::Range(r))),
                    Err(e) => Err((ErrorCode::RequestFailed, e)),
                }
            }
            request::Rename::METHOD => {
                let p: RenameParams = params(req.params)?;
                let tp = p.text_document_position;
                let d = doc_or_null!(&tp.text_document.uri);
                let imps = self.imports_of(&d);
                let others = self.other_open_docs(&d.uri);
                features::rename(&d, &imps, &others, tp.position, &p.new_name).map(json).map_err(|e| (ErrorCode::RequestFailed, e))
            }
            request::WorkspaceSymbolRequest::METHOD => {
                let p: WorkspaceSymbolParams = params(req.params)?;
                let sources = self.workspace_sources();
                ok(json(WorkspaceSymbolResponse::Flat(features::workspace_symbols(&sources, &p.query))))
            }
            _ => Err((ErrorCode::MethodNotFound, format!("unhandled method {}", req.method))),
        }
    }
}

fn capabilities() -> ServerCapabilities {
    ServerCapabilities {
        position_encoding: Some(PositionEncodingKind::UTF16),
        text_document_sync: Some(TextDocumentSyncCapability::Options(TextDocumentSyncOptions {
            open_close: Some(true),
            change: Some(TextDocumentSyncKind::FULL),
            save: Some(TextDocumentSyncSaveOptions::SaveOptions(SaveOptions { include_text: Some(false) })),
            ..Default::default()
        })),
        hover_provider: Some(HoverProviderCapability::Simple(true)),
        completion_provider: Some(CompletionOptions { trigger_characters: Some(vec![".".into()]), ..Default::default() }),
        signature_help_provider: Some(SignatureHelpOptions {
            trigger_characters: Some(vec!["(".into(), ",".into()]),
            retrigger_characters: None,
            work_done_progress_options: Default::default(),
        }),
        definition_provider: Some(OneOf::Left(true)),
        declaration_provider: Some(DeclarationCapability::Simple(true)),
        references_provider: Some(OneOf::Left(true)),
        document_highlight_provider: Some(OneOf::Left(true)),
        document_symbol_provider: Some(OneOf::Left(true)),
        workspace_symbol_provider: Some(OneOf::Left(true)),
        folding_range_provider: Some(FoldingRangeProviderCapability::Simple(true)),
        rename_provider: Some(OneOf::Right(RenameOptions { prepare_provider: Some(true), work_done_progress_options: Default::default() })),
        semantic_tokens_provider: Some(SemanticTokensServerCapabilities::SemanticTokensOptions(SemanticTokensOptions {
            legend: features::legend(),
            full: Some(SemanticTokensFullOptions::Bool(true)),
            range: None,
            work_done_progress_options: Default::default(),
        })),
        ..Default::default()
    }
}

#[allow(deprecated)]
fn main() {
    let arg = std::env::args().nth(1);
    match arg.as_deref() {
        Some("--version" | "-V") => {
            println!("tessera-lsp {}", env!("CARGO_PKG_VERSION"));
            return;
        }
        Some("--help" | "-h") => {
            println!("tessera-lsp {}\nLanguage server for Tessera (.t, .csec). Speaks LSP over stdio.\n\nCompiler lookup: initializationOptions.compilerPath, $TESSERA_COMPILER, <root>/x64/{{Release,Debug}}/tess, <root>/build/tess, PATH.", env!("CARGO_PKG_VERSION"));
            return;
        }
        _ => {}
    }

    let (connection, io_threads) = Connection::stdio();
    let (id, init) = match connection.initialize_start() {
        Ok(x) => x,
        Err(e) => {
            eprintln!("tessera-lsp: initialize failed: {e}");
            return;
        }
    };
    let init: InitializeParams = serde_json::from_value(init).unwrap_or_default();
    let result = InitializeResult {
        capabilities: capabilities(),
        server_info: Some(ServerInfo { name: "tessera-lsp".into(), version: Some(env!("CARGO_PKG_VERSION").into()) }),
    };
    if connection.initialize_finish(id, serde_json::to_value(result).unwrap()).is_err() {
        return;
    }

    let mut roots: Vec<PathBuf> = init
        .workspace_folders
        .unwrap_or_default()
        .iter()
        .filter_map(|f| f.uri.to_file_path().ok())
        .collect();
    if roots.is_empty() {
        if let Some(p) = init.root_uri.and_then(|u| u.to_file_path().ok()) {
            roots.push(p);
        } else if let Some(p) = init.root_path {
            roots.push(PathBuf::from(p));
        }
    }
    let roots = roots.into_iter().map(|r| imports::canonical(&r)).collect();
    let settings = init
        .initialization_options
        .map(|v| v.get("tessera").cloned().unwrap_or(v))
        .and_then(|v| serde_json::from_value::<Settings>(v).ok())
        .unwrap_or_default();
    let snippets = init
        .capabilities
        .text_document
        .as_ref()
        .and_then(|t| t.completion.as_ref())
        .and_then(|c| c.completion_item.as_ref())
        .and_then(|c| c.snippet_support)
        .unwrap_or(false);

    let mut server = Server {
        sender: connection.sender.clone(),
        docs: HashMap::new(),
        roots,
        settings,
        compiler: None,
        warned_no_compiler: false,
        snippets,
        diags: Arc::default(),
        generations: Arc::default(),
        disk_cache: HashMap::new(),
        workspace_index: None,
    };
    server.find_compiler();

    for msg in &connection.receiver {
        match msg {
            Message::Request(req) => {
                if connection.handle_shutdown(&req).unwrap_or(true) {
                    break;
                }
                let resp = server.on_request(req);
                connection.sender.send(Message::Response(resp)).ok();
            }
            Message::Notification(not) => server.on_notification(not),
            Message::Response(_) => {}
        }
    }
    drop(server);
    drop(connection);
    io_threads.join().ok();
}

