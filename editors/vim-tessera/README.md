# Tessera for Vim / Neovim

Filetype detection, syntax highlighting, indentation, and buffer settings for Tessera (`.csec`) files.

It highlights keywords, declarations (`def` names, `def operator +`, `class` / `struct` / `object`
names), types, numbers (hex, binary, octal, exponents), strings with `${...}` interpolation and
escapes, raw strings `r"..."`, characters, `//` `/* */` comments and `///` `/** */` doc comments,
`[@Attributes]`, `$...$` LaTeX math, and these context-dependent words:

- contract clauses `requires` / `ensures` / `invariant` / `decreases` between a signature or loop
  header and its body, with `result` highlighted only inside them (`val result = 1` stays plain);
- parallel policies `cpu` / `simd` / `openmp` / `gpu` inside `pmap(...)` / `preduce(...)`;
- DSL words (`atom`, `bond`, `steps`, ...) inside `molecule` / `cfd` / `ode` / `protein` blocks;
- infix words (`a inner b`, `xs union ys`, `p implies q`).

Contract lines indent one level under the declaration and the body `{` returns to its level:

```tessera
def div(a: Int, b: Int): Int
    requires b != 0
    ensures result * b + a % b == a
{
    return a / b;
}
```

`gf` on `import std.math.algebra;` opens `lib/std/math/algebra.csec` (from the repository root).

## Install

With a plugin manager, point it at this directory, e.g. [vim-plug](https://github.com/junegunn/vim-plug):

```vim
Plug '~/path/to/tessera/editors/vim-tessera'
```

Or with Vim's native packages:

```sh
mkdir -p ~/.vim/pack/tessera/start
ln -s "$PWD/editors/vim-tessera" ~/.vim/pack/tessera/start/vim-tessera
# Neovim: ~/.local/share/nvim/site/pack/tessera/start
# Windows: mklink /J %USERPROFILE%\vimfiles\pack\tessera\start\vim-tessera editors\vim-tessera
```

Keep the word lists in sync with `editors/vscode-tessera/syntaxes/tessera.tmLanguage.json` and
`editors/openidea-tessera/crates/tessera-syntax/src/{lexer,highlight}.rs`.
