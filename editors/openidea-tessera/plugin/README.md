# Tessera for OpenIDEA

Support for Tessera (`.csec`) files in OpenIDEA.

- **Highlighting, structure view, folding** — from a WebAssembly
  `syntax-provider` built on the shared `tessera-syntax` lexer, which mirrors
  the `tess` lexer (keywords, operators, literals, `${name}` interpolation,
  `r"..."` regex literals, contextual DSL words such as `atom`/`steps` inside
  `molecule { ... }`, and parallel policies in `pmap`/`preduce`).
- **Language server** — `tessera-lsp`, started by name when it is on `PATH`
  (grant `process:tessera-lsp` when installing): `tess` parse and type errors
  on open/save, lexical problems while typing, completion, hover, go to
  definition (also into imported files), references, rename, signature help,
  document/workspace symbols and semantic highlighting.
- A **New › Tessera File** template, a scratch template and a `.csec` file icon.

Build everything with `editors/openidea-tessera/scripts/build.ps1`; see the
README there.
