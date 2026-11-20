# Tessera editor support: OpenIDEA plugin + `tessera-lsp`

```
editors/openidea-tessera/
├─ crates/tessera-syntax/   shared lexer, highlighting, declarations/outline, folding, lexical diagnostics
├─ crates/tessera-lsp/      the language server (LSP over stdio)
├─ plugin/                  the OpenIDEA plugin package (language definition + wasm syntax provider)
└─ scripts/                 build.ps1 (build, check, pack, install), smoke_lsp.py (end-to-end test)
```

`tessera-syntax` mirrors `src/lexer.cpp`: the same reserved words, the same
maximal-munch operator set, the same literal rules. Keep it in sync when the
lexical grammar changes (`docs/grammar.md` §1); `cargo test` lexes every
`.csec` file in `tests/positive`, `examples` and `selfhost/src` and fails on any
character `tess` would accept but it doesn't.

## Build and install

Needs Rust (stable) and, for the plugin, the `wasm32-wasip2` target (the
script adds it).

```powershell
powershell -File editors/openidea-tessera/scripts/build.ps1 -Test -Install
```

This runs the tests, builds `target/release/tessera-lsp.exe`, builds the
plugin component into `plugin/dist/plugin.wasm`, validates it with
`openidea-plugin check` when that tool is installed, packs
`tessera-lang.tessera-0.1.0.oiplugin`, and with `-Install` puts `tessera-lsp`
in `~/.cargo/bin`.

1. Make sure `tessera-lsp` is on `PATH` for OpenIDEA (`~/.cargo/bin`, or copy
   the exe anywhere on `PATH`), then restart the IDE.
2. In OpenIDEA: **Settings › Plugins › ⚙ › Install Plugin from Package File…**
   and pick the `.oiplugin`. Grant `process:tessera-lsp` on the install page
   (it is optional: without it you still get highlighting, structure view and
   folding).

For plugin development use `openidea-plugin link editors/openidea-tessera/plugin`
after building, then Settings › Plugins › *Reload* after each rebuild. The tool
installs with
`cargo install --locked --git https://github.com/l0ng-ai/openidea --tag v0.1.38 ide-host --bin openidea-plugin`.

## What you get

| Feature | Where it comes from |
| --- | --- |
| Highlighting (keywords, types, `${name}` interpolation, `r"..."` regexes, DSL words inside `molecule`/`cfd`/`ode`/`protein`, `pmap`/`preduce` policies, word operators such as `inner`, LaTeX `\commands`) | plugin (wasm) |
| Structure view, folding (bodies, comment runs, import blocks) | plugin (wasm) |
| `.csec` icon, *New › Tessera File*, scratch template, comment toggling | plugin |
| Errors as you type: unknown characters, unterminated strings/comments, unbalanced brackets, `&&`/`||` | `tessera-lsp` |
| `tess` parse and type errors on open/save (mapped through `import` expansion) | `tessera-lsp` |
| Completion (scope-aware locals, parameters, imported declarations, members after `.`, keywords, built-in types/functions, snippets) | `tessera-lsp` |
| Hover (signature + doc comment), go to definition (also into imported files and on `import "…"`), references, highlights, rename (locals/globals; not fields/methods), signature help, workspace symbols, semantic tokens | `tessera-lsp` |

`tess` stops at the first parse error and reports type errors without a
location; type errors are anchored on the declaration (or, for move/borrow
errors, the last use) of the first name quoted in the message.

## Language server settings

`plugin/languages/tessera.toml` passes these as `initializationOptions` (all
optional):

| Key | Default | |
| --- | --- | --- |
| `compilerPath` | auto | `tess` to run. Auto: `$TESSERA_COMPILER`, `<root>/x64/{Release,Debug}`, `<root>/build[/Release,/Debug]`, then `PATH`. |
| `compilerArgs` | `[]` | extra flags, e.g. `["--no-strict-ownership"]` for the self-host sources |
| `checkMode` | `"full"` | `"full"` = `--emit-ir` (parse + type check), `"syntax"` = `--syntax-only` |
| `checkOnOpen` / `checkOnSave` | `true` | |
| `checkTimeoutMs` | `30000` | |

The server is editor-neutral: any LSP client (VS Code, Neovim, Helix, …) can
start `tessera-lsp` for `.csec` files.
