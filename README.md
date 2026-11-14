# Tessera

Tessera is an experimental, tensor-first language for numerical computing, data
analysis, and scientific modeling. The compiler targets math-heavy programs that
should be easy to write and still have a clear path to optimized CPU, SIMD,
OpenMP, and GPU execution. The name comes from *tesseract* (a four-dimensional
tensor), reflecting the tensor-native core.

This repository contains the current C++17 compiler prototype, its native runtime,
and language regression fixtures. The compiler (`csec++`) parses `.csec` source
files, builds LLVM IR, and can emit a native executable (the default), write
IR/object files, or run through the configured LLVM execution backend.

For the language surface see [docs/syntax.md](docs/syntax.md); for the normative
grammar see [docs/grammar.md](docs/grammar.md).

## What Is Implemented

- Lexer/parser, AST, semantic checks, and LLVM IR generation for the current
  Tessera syntax.
- **First-class tensors**: `Tensor<Elem, D1, D2, …>` shapes, `new` construction,
  `t[i, j]` element access/assignment (any-depth member-access l-values too),
  component-wise arithmetic with scalar broadcast, `@` matmul, `inner`/`outer`/
  `tensor` products, named `x`/`y`/`z`/`w` components, and Fortran-style sections.
  See [examples/feedforward_nn.csec](examples/feedforward_nn.csec).
- Arbitrary-precision `Nat`, scalar math, set helpers, and transformer-oriented
  tensor test fixtures.
- **Strict-by-default ownership**: Copy vs owned (affine) types, explicit `<-`
  move and `&`/`&mut` borrows, use-after-move / borrow / alias checking, `clone`,
  `free`, `box T`, and `Shared`/`Weak` reference counting. Scope-exit
  destruction currently drops `box`, tensors, and `Shared`/`Weak` automatically;
  heap classes and arrays/vectors are released with explicit `free` until
  ownership-aware drop glue lands. `--no-strict-ownership` opts out.
- **Multiple return values** with tuple return types and destructuring
  (`a, b = f()`, `_` to ignore), and value `struct`s.
- **Concurrency**: Erlang-style microthreads (`spawn`/`send`/`receive`, mailboxes,
  monitors, share-nothing captures), `go f(x)` and `async f(x)` / `await` with
  `Future<T>`, OS threads with mutexes/atomics/channels, and inter-task
  communication through pipes, shared memory (also across OS processes), and
  named mailboxes (mail senders/receivers). `async def` keeps the
  no-borrow-across-await rule.
- **Standard library** in `lib/std`, found by `import` (`std.microthread`,
  `std.thread`, `std.process`, `std.pipe`, `std.shared`, `std.mail`, `std.io`,
  `std.dataframe` (Polars-style DataFrame/LazyFrame/expressions over Parquet and CSV),
  `std.math.natural`/`real`/`complex`, `std.symbolic`, `std.json`, `std.net`,
  `std.http` (HTTP(S) client, streaming, Server-Sent Events, HTML/URL helpers for
  crawlers), `std.openai` (OpenAI-compatible chat/streaming/tool calls/embeddings)). A library module enables
  compiler intrinsics with `[@Intrinsic] external def ...`.
- **Math**: arbitrary-precision `Nat` with division/powers/gcd/primality,
  built-in `Complex`/`Rational`/`Quaternion` value types, and a symbolic algebra
  engine (simplify, expand, differentiate, integrate, solve, LaTeX output).
- `text[i]` byte indexing of strings (a single load, vectorisable) and
  memory-mapped / chunked binary file input (`std.io`). See
  [examples/parallel_wc.csec](examples/parallel_wc.csec), a parallel `wc -l`
  that runs at about 92% of a C++ mmap+SIMD reference.
- Web access: see [examples/web_fetch.csec](examples/web_fetch.csec),
  [examples/web_crawler.csec](examples/web_crawler.csec) (concurrent same-site crawler),
  [examples/openai_chat.csec](examples/openai_chat.csec), and
  [examples/openai_web_agent.csec](examples/openai_web_agent.csec) (an LLM that browses
  through tool calls; works with OpenAI, Ollama, LM Studio, vLLM, ...).
- Native runtime support for printing, input, math functions, sockets, POSIX-like
  file/process helpers, parallel helpers, and dynamic library interop.
- `[@DllImport("library", "symbol")] external def ...` for native symbol imports.
- `loadLibrary`, `getSymbol`, `closeLibrary`, `callNative*`, and
  `callNativeDouble*` for runtime dynamic library calls.
- `--emit-exe` (default), `--emit-ir`, `--emit-obj`, `--run`/`--run-ir` output
  modes, plus `--static` to link the runtime into a standalone executable.

## Current Focus

- Tensor-first syntax and type forms such as `Tensor<Double, 2, 3>`.
- Math-native operators and LaTeX-inspired equation syntax.
- Collection transforms: `map`, `pmap`, `filter`, and `reduce`.
- Policy-based reduction syntax with `preduce(cpu|simd, acc, x <- xs, init)`.
- Modeling sugar for ODE, molecular dynamics, CFD, and MCMC-style examples.
- Strict-by-default ownership with explicit `<-` move / `&` borrow and
  `unsafe`/`unatomic` escape hatches.

## Requirements

Common requirements:

- CMake 3.16 or newer.
- A C++17 compiler.
- LLVM development files and CMake package files. LLVM 18 through 23 are
  supported. `src/llvm_compat.h` picks the right API for each release, and the
  code builds without deprecation warnings against LLVM 23.
- LLVM command line tools, especially `llc`, for `--emit-obj` and `--emit-exe`.
- A native C++ linker driver, usually `c++`, `clang++`, or `g++`.

Optional requirements:

- Ninja, for faster CMake builds.
- OpenMP runtime/development package, used when available by the native parallel
  runtime.
- libedit/readline-compatible development package when required by the installed
  LLVM package. Some LLVM builds link `LLVMSupport` against editline.

### Linux Packages

Ubuntu/Debian example:

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build llvm-dev clang lld
sudo apt install libomp-dev libedit-dev
```

Fedora example:

```sh
sudo dnf install gcc-c++ cmake ninja-build llvm-devel clang lld
sudo dnf install libomp-devel libedit-devel
```

Arch Linux example:

```sh
sudo pacman -S base-devel cmake ninja llvm clang lld openmp libedit
```

Package names vary by distribution. The important pieces are the LLVM CMake
package (`LLVMConfig.cmake`), LLVM libraries/headers, and the `llc` executable.

### macOS Packages

Homebrew example:

```sh
brew install cmake ninja llvm libomp libedit
```

Homebrew LLVM is not always on the default compiler/linker search path. Use
`LLVM_DIR`, `LLVM_BIN`, and `PATH` as shown below if CMake or `--emit-exe` cannot
find LLVM.

## Build

Recommended CMake build:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

If LLVM was installed in a non-default prefix, point CMake at LLVM explicitly:

```sh
cmake -S . -B build -G Ninja -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm
cmake --build build
```

Common macOS/Homebrew command:

```sh
cmake -S . -B build -G Ninja -DLLVM_DIR="$(brew --prefix llvm)/lib/cmake/llvm"
cmake --build build
```

On Windows, a prebuilt LLVM that uses the DLL C runtime (`/MD`, e.g. conda-forge
`llvmdev`) needs a Release build with `-DCSEC_RELEASE_DLL_CRT=ON`:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCSEC_RELEASE_DLL_CRT=ON ^
      -DLLVM_DIR=%CONDA_PREFIX%\Library\lib\cmake\llvm
```

Microthreads (`std.microthread`, `go`/`async`) work on Windows (fibers), Linux,
and macOS. On x86-64 and arm64 (including Apple Silicon) they use a small
assembly context switch rather than the deprecated `ucontext` API.

### Linux and macOS with conda-forge LLVM

This is the setup the continuous-integration jobs (`.github/workflows/ci.yml`, Ubuntu x86-64 and
macOS arm64) use, and it needs no root access:

```sh
conda create -n tessera -c conda-forge "llvmdev=18.*" zlib zstd libxml2 cmake ninja
conda activate tessera
sh scripts/fetch_z3.sh                  # optional: Z3 for `csec++ --verify` and std.logic
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
      -DLLVM_DIR="$CONDA_PREFIX/lib/cmake/llvm" -DCMAKE_PREFIX_PATH="$CONDA_PREFIX"
cmake --build build
sh tests/run_suite.sh Debug/csec++      # fixtures, --verify diagnostics, self-host
```

Packaged LLVMs on Linux and macOS (conda-forge, Homebrew, apt) are built to be used through their
shared `libLLVM`; CMake detects that (`LLVM_LINK_LLVM_DYLIB`) and links it instead of the static
component libraries (`-DCSEC_LLVM_SHARED=ON/OFF` overrides). It also compiles csec++ with `NDEBUG`
when LLVM was built without assertions, because some LLVM classes change layout with `NDEBUG`.

### Z3 (`csec++ --verify`, `std.logic`)

Z3 is optional. By default csec++ links the shared library (`libz3.dll`, `libz3.so`, `libz3.dylib`):
`scripts/fetch_z3.ps1` (Windows, SHA-256 checked) or `scripts/fetch_z3.sh` (Linux, macOS) downloads
the official release into `deps/z3`, and CMake copies `libz3.dll` next to `csec++.exe`. To link Z3
statically instead, so that neither csec++ nor the runtime needs the shared library, build the static
library from source and select it:

```sh
sh scripts/build_z3_static.sh                       # Linux, macOS -> deps/z3-static
powershell -File scripts\build_z3_static.ps1 -Config Debug   # Windows, from a VS developer prompt
cmake -S . -B build -G Ninja -DCSEC_Z3_LINK=STATIC ...
```

On Windows the static library must use the same C runtime as csec++: `-Config Debug` for a Debug
build, `-Config Release` (add `-DllCrt` with `-DCSEC_RELEASE_DLL_CRT=ON`) for a Release build.

The build produces:

- `csec++`: compiler executable.
- `csec_native_runtime`: shared native runtime library copied next to the
  compiler executable.

### Editor support

- VS Code: `editors/vscode-tessera` (TextMate grammar).
- OpenIDEA: `editors/openidea-tessera` (plugin plus the `tessera-lsp` language server: hover,
  completion, snippets, go-to-definition, outline, diagnostics).
- Vim / Neovim: `editors/vim-tessera` (syntax, indentation, filetype settings).

## Compiler Usage

From the build output directory:

```sh
./csec++ --syntax-only ../tests/positive/00_smoke/001_minimal_main.csec
./csec++ --emit-ir ../tests/native/003_posix_dynamic_symbol_default.csec -o out.ll
./csec++ --emit-obj ../tests/native/003_posix_dynamic_symbol_default.csec -o out.o
./csec++ --emit-exe ../tests/native/003_posix_dynamic_symbol_default.csec -o out
./out
```

### Optimization

Pass `-O0`, `-O1`, `-O2`, `-O3` (or `-O` for `-O2`) to control optimization of the
emitted output. `-O1`/`-O2`/`-O3` run LLVM's per-module optimization pipeline
(mem2reg, inlining, GVN, DCE, loop optimizations, ...); `-O0` emits unoptimized IR;
with no flag the compiler only promotes stack slots to registers (mem2reg). The
level applies to `--emit-ir`/`--emit-obj`/`--emit-exe` (and code run from that
output, e.g. `--run-ir`); `--run` always uses the interpreter-safe mem2reg pipeline.

```sh
./csec++ --emit-exe program.csec -O2 -o program   # optimized native executable
./csec++ --emit-ir  program.csec -O3 -o program.ll # inspect optimized IR
./csec++ --emit-exe program.csec -O3 -march=native -o program  # use this CPU's vector ISA
```

Modules are generated for the host target triple and data layout. The default CPU
is the portable baseline (SSE2 on x86-64); `-march=native` targets the build
machine's CPU (e.g. AVX2/AVX-512).

### Standard Library Path

`import std.io` resolves to `lib/std/io.csec`. The compiler looks in the
importing file's directory and the working directory, then in the
standard-library roots: directories given with `--lib-path <dir>`, entries of
`CSEC_LIB_PATH`, and `lib/` next to the runtime library (or up to two parent
directories, so a build in `x64/Debug` finds the repository `lib/`). See
`LANGUAGE_SPEC.md`.

`--emit-obj` and `--emit-exe` call `llc`. The compiler searches for `llc` in:

1. `LLVM_BIN`
2. `../llvm-project/build/bin`
3. `PATH`

Examples:

```sh
export LLVM_BIN="$(brew --prefix llvm)/bin"
export PATH="$LLVM_BIN:$PATH"
```

Standard runtime APIs are provided through a C#-style native system assembly.
The build emits `System.Native.dll` on Windows, `libSystem.Native.dylib` on
macOS, or `System.Native.so` on Linux. Programs that use built-in runtime APIs
automatically link `System.Native`; `[@DllImport("System.Native", "...")]` also
resolves to the platform library extension.

`--emit-exe` compiles the program and links any required `System.*` libraries.
Override the native linker driver with `CXX` when needed:

```sh
export CXX=clang++
./csec++ --emit-exe program.csec -o program
```

Additional native libraries and search paths can be passed through:

```sh
./csec++ --emit-exe program.csec -o program \
  --link-path /usr/local/lib \
  --link-lib m
```

For dynamic library interop:

```ts
[@DllImport("libm.so", "cos")]
external def nativeCos(value: Double): Double;

def main(): Int {
    val imported: Double = nativeCos(0.0);

    val handle: Long = loadLibrary("libm.so.6");
    val symbol: Long = getSymbol(handle, "cos");
    val value: Double = callNativeDouble1(symbol, 0.0);
    closeLibrary(handle);
    return 0;
}
```

On macOS, use `.dylib` names such as `libm.dylib`. On Linux, use `.so` names such
as `libm.so.6`. Passing `0` to `getSymbol` searches the default process symbol
scope on POSIX platforms.

`nativeCos` is the Tessera name used at the call site. The actual C symbol
is the second `DllImport` argument, `"cos"`. Avoid declaring the Tessera
function itself as `cos`, because `cos(...)` is already a built-in scalar math
helper and will normally resolve through the native runtime math path.

## Parallel API Surface

`pmap` accepts an optional backend policy:

```ts
val ys = pmap(openmp, x <- xs) {
    x * x;
};
```

Supported policy names are `cpu`, `simd`, `openmp`, and `gpu`. `openmp` lowers
capture-free `pmap` bodies to an outlined callback and executes it through the
native parallel runtime. `simd` and `gpu` are accepted as policy names, but full
SIMD/GPU lowering remains backend work.

Runtime parallel configuration helpers:

```ts
val n = parallelThreads();
setParallelThreads(8);
val hasOpenMp = parallelBackendAvailable("openmp");
val gpuReady = parallelBackendImplemented("gpu");
```

`parallelBackendAvailable` answers whether a backend is plausible on the current
platform. `parallelBackendImplemented` answers whether this compiler/runtime path
is wired today. `gpu` is reserved syntax and currently fails during semantic
analysis/code generation instead of silently falling back to CPU execution.

Reduction can name its accumulator explicitly through `preduce`:

```ts
val total = preduce(simd, acc, x <- xs, 0) {
    acc + x;
};
```

`preduce(cpu, ...)` lowers to the normal deterministic reduction loop.
`preduce(simd, ...)` emits vectorization metadata. `preduce(openmp, ...)` and
`preduce(gpu, ...)` are reserved until real parallel reduction lowering exists.

## Design Direction

The intended high-level numerical syntax includes:

- Pipeline-friendly collection expressions.
- Typed tensors with dtype and shape information.
- Broadcasting, slicing, reductions, scans, and stencils.
- Dataframe/table operations for CSV and tabular data.
- Explicit execution policies for CPU, SIMD, OpenMP, and GPU backends.
- Backend lowering to optimized libraries such as BLAS/LAPACK and GPU math
  libraries where possible.

See `docs/memory-model-design.md` for the implemented ownership, borrow, move,
and destruction model (milestones M0–M8), and `docs/syntax.md` for the language
surface. `docs/memory-management.md` is the earlier design proposal, kept for
historical reference. Runnable ownership examples live in
`examples/strict_ownership_demo.csec` and `examples/strict_ownership_errors.csec`.
