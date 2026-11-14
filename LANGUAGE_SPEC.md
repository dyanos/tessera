# Tessera Language Specification — Concurrency and Standard Library

This document records accepted syntax and semantics that both the native compiler
(`src/`) and the self-hosted compiler (`selfhost/`) implement. The broader surface
syntax is described in `docs/syntax.md` and `docs/grammar.md`.

## 1. Library imports and intrinsics

### 1.1 Import resolution

```
import a.b.c
import "relative/path.csec"
```

`import` textually includes a source file once (cycles and repeats are ignored). A
target is resolved in this order; the first existing file wins:

1. relative to the importing file, then the working directory (`a.b.c` as given, then
   as `a/b/c.csec`);
2. the **standard-library roots**, as `<root>/a/b/c.csec`:
   1. directories passed with `--lib-path <dir>` (native compiler);
   2. entries of the `CSEC_LIB_PATH` environment variable (`;`-separated on Windows,
      `:` elsewhere);
   3. `lib/` next to the runtime library (`System.Native.dll`), and `lib/` in up to two
      parent directories of it (so a build in `x64/Debug` finds the repository `lib/`).

An import that resolves to no file is left in place and ignored, as before.

**Aliases.** `import a.b.c = name;` (also `import "path.csec" = name;`) imports the module as
usual and also gives it a local name:

```
import std.dataframe = df;

def main(): Int {
    val t = df.readParquet("sales.parquet");   // same as readParquet(...)
    return 0;
}
```

- In the importing file, `name.f` refers to `f` when `f` is a top-level function (`def` or
  `external def`, intrinsics included) of the aliased module. The module's names also stay
  available unqualified, since imports are textual.
- `name.x`, where `x` is not a function of the module, is left alone. So a variable that shares
  the alias name keeps its members (`val df = "abc"; df.length`).
- Aliases are per file. They are not applied inside comments or string literals, and they do
  not reach the files that the module itself imports.
- It is an error to alias a module that cannot be found, to use one alias for two modules, or
  to use an alias that is not an identifier.

Both compilers share one implementation of this lookup (`csecFindLibraryModule` in
`src/native_runtime/runtime_services.inc`).

### 1.2 Intrinsics

The compiler has built-in lowerings for some operations that cannot be written in the
language itself (they take closures, or need code generation). They are **off by
default**. A library module enables one by declaring it with the `Intrinsic` attribute:

```
[@Intrinsic] external def spawn(body: () => Int): Int;
```

The declaration emits no code; it only makes the name available, with the compiler's
built-in meaning, to the program that imports the module. So the compiler author
decides, module by module, which intrinsics an `import` exposes. Calling an intrinsic
name that no imported module declared is an ordinary unresolved call (an error).

| Intrinsic | Declared by |
| --- | --- |
| `spawn self send receive receiveTimeout yield isAlive monitor mailboxSize processCount joinProcesses` | `std.microthread` |
| `threadStart` | `std.thread` |

### 1.3 Standard library modules (`lib/std`)

| Module | Contents |
| --- | --- |
| `std.microthread` | Erlang-style processes (§2) |
| `std.thread` | OS threads, mutexes, atomics, channels |
| `std.dataframe` | Polars-style DataFrame / LazyFrame / expressions; Parquet and CSV (§8) |
| `std.process` | running OS processes (`runCommand`, `commandOutput`, `startProcess`, `waitProcess`, ...) |
| `std.math.natural` | `Nat` division/modulo/power/gcd/lcm/isqrt, primality, factorial, binomial, Fibonacci |
| `std.math.real` | constants and real-number helpers |
| `std.math.complex` | complex numbers: `imaginaryUnit()`, `conj`, `cabs`, `carg`, `cexp`, `clog`, `csqrt`, `cpow`, ... |
| `std.math.algebra` | rings and fields with operator overloading: Z/nZ, GF(p), GF(p)[x], GF(p^k) (§11) |
| `std.logic` | formulas, validity / counterexamples with Z3, and an LCF-style proof kernel (§13) |
| `std.symbolic` | symbolic algebra: parse, simplify, expand, diff, integrate, subst, solve, evaluate, print |
| `std.pipe` | byte/line pipes between threads and microthreads; child processes with piped stdin/stdout (§6) |
| `std.shared` | shared memory: private (threads/microthreads) or named (across OS processes) (§6) |
| `std.mail` | named mailboxes with mail senders and receivers, across threads, microthreads, and processes (§6) |
| `std.io` | chunked binary file reads, memory-mapped files, `parseInt`/`parseLong` (§7) |
| `std.json` | JSON documents: parse, path queries, build, print (§9) |
| `std.net` | plain TCP sockets (§9) |
| `std.http` | HTTP(S) client, streamed bodies and Server-Sent Events, URL and HTML helpers (§9) |
| `std.openai` | client for OpenAI-compatible APIs: chat, streaming, tool calls, embeddings, models (§9) |

## 2. Microthreads (`import std.microthread`)

A *process* is a lightweight user-mode thread with its own stack and a private FIFO
mailbox of `Int` messages. Processes share nothing.

```
val pid = spawn([=]() -> { val n = receive(); send(parent, n * 2); return 0; });
```

- `spawn(f)`: `f` must be a zero-parameter lambda **literal** capturing by value (`[=]`
  or an explicit capture list), or the name of a zero-parameter function. `[&]`
  lambdas, lambdas with parameters, and closure-valued variables are compile errors.
  The lambda's captured values are copied into the new process at spawn time.
- Scheduling is cooperative and M:1 per OS thread. Processes run in FIFO order and
  switch only at `receive`/`receiveTimeout` with an empty mailbox, `await` of an
  unfinished future, `yield`, `joinProcesses`, and process exit.
- `main` is pid 1. When `main` returns the program ends; `joinProcesses()` first runs
  the other processes until none can make progress.
- If every process is blocked in `receive`/`await` and no timeout is pending, the
  runtime prints a deadlock diagnostic and exits with status 70.
- Besides `Int` messages each process has a FIFO mailbox of strings:
  `sendText(pid, text)`, `receiveText()`, `receiveTextTimeout(ms, fallback)`,
  `textMailboxSize()`. The text is copied to the receiver.

## 3. Asynchronous execution: `go`, `async`, `await`

```
PrefixExpression ::= 'go' CallExpression
                   | 'async' CallExpression
                   | 'await' PrefixExpression
```

`go` is a reserved keyword. `async` keeps its meaning as a declaration modifier when it
is directly followed by `def` (or `unsafe def`); anywhere else it starts an `async`
expression.

- `go f(a, b)` evaluates the arguments in the caller, starts the call in a new
  microthread, discards its result, and yields the new process id (`Int`).
- `async f(a, b)` does the same but yields a `Future<T>`, where `T` is the call's type.
- `await e`:
  - if `e : Future<T>`, blocks the current microthread until the task has finished and
    yields its `T` result (awaiting a future again returns the same value);
  - otherwise `e` must appear inside an `async def`, and the value is yielded unchanged
    (an `async def` still runs synchronously when called directly).
- The operand of `go`/`async` must be a call of a named function. Arguments may not be
  borrows (`&x`, `&mut x`): the task can outlive them.
- `await` keeps the ownership rule that no borrow may be live across it.
- Supported result types for `Future<T>`: integers, `Boolean`, `Double`, `Float`,
  `String`, and other pointer-represented values.
- `go`/`async`/`await` need no import; they use the same runtime scheduler as
  `std.microthread`.

## 4. Built-in number aggregates

`Complex { re: Double, im: Double }`, `Rational { numerator: Long, denominator: Long }`
and `Quaternion { w, x, y, z: Double }` are built-in **value** types (copied like
`struct`s; no `<-` needed):

- construction: `new Complex(re, im)`, `new Rational(n, d)`, `new Quaternion(w, x, y, z)`;
- fields are read with `.re`, `.im`, etc. on any expression (`conj(z).im`);
- `+ - * /` between two values of the same type (no `/` for `Quaternion`).

## 6. Communication between threads, microthreads, and processes

| Mechanism | Module | Threads | Microthreads | OS processes |
| --- | --- | --- | --- | --- |
| Int / text messages | `std.microthread` | — (one scheduler per thread) | yes | — |
| Channels (Long) | `std.thread` | yes | yes | — |
| Pipes | `std.pipe` | yes | yes | child stdin/stdout (`pipeOpenProcess`) |
| Shared memory | `std.shared` | yes | yes | named regions (`sharedOpen`) |
| Mail | `std.mail` | yes | yes | yes (named mailboxes) |

**Cooperative blocking.** Every blocking operation of `std.pipe`, `std.shared`
(`sharedLock`), `std.mail`, and `std.thread` (`mutexLock`, `channelReceive`,
`channelReceiveTimeout`, `threadJoin`) yields to the other runnable microthreads of its
OS thread while it waits, and sleeps when there are none. So a microthread waiting
on a pipe or for mail never stalls the rest of its thread, and microthread stages can be
chained into a pipeline with pipes.

**Pipes.** `pipeNew()` creates an in-process byte pipe. `pipeWrite`/`pipeWriteLine`
append; `pipeRead(max)`/`pipeReadLine()` wait for data; `pipeClose` ends the input, after
which readers drain what remains and then see end of input (`pipeAtEnd`).
`pipeOpenProcess(command)` starts a shell command whose stdin and stdout are connected to
the caller: `processWrite[Line]`, `processCloseInput`, `processReadLine`,
`processReadAll`, `processFinish` (waits; returns the exit code).

**Shared memory.** `sharedNew(bytes)` is private to the program; `sharedOpen(name,
bytes)` names a region shared by every process of the user session that opens the name.
The first opener creates it zero-filled. Memory is addressed as `Long`/`Double` slots
(`index * 8` bytes), bytes, or NUL-terminated strings at a byte offset. Out-of-range
accesses read 0 and are ignored. `sharedAddLong` and `sharedCompareExchange` are atomic.
`sharedLock`/`sharedUnlock` is a region-wide lock that also works across processes.

**Mail.** A mailbox is identified by name. `mailSender(mailbox, senderName)` and
`mailReceiver(mailbox)` open endpoints, from any thread, microthread, or process.
`mailSend[Tagged]` queues a string (with an optional `Long` tag). `mailReceive`,
`mailReceiveTimeout`, and `mailPoll` take the oldest message. `mailLastSender` and
`mailLastTag` describe the message last received through that endpoint. Messages are
copied and delivered FIFO. A mailbox holds about 1 MiB of queued mail, and a send to a
full mailbox returns `false`.

**Threads, mutexes, and channels.** `threadStart(body)` runs `body` on a new OS thread and
`threadJoin` returns its result. A thread that is never joined is abandoned when `main`
returns, and the program still exits normally. Mutexes are not recursive. Microthreads may
contend for one, and `mutexUnlock` on a mutex that is not locked returns `false`.
Channels are unbounded FIFO queues of `Long`. `channelSend` returns `false` once the channel is
closed. `channelReceive(ch, fallback)` returns `fallback` when the channel is closed and empty,
and `channelReceiveTimeout(ch, ms, fallback)` also returns it after `ms` milliseconds with no
value. `channelClosed` reports whether the channel is closed, and `channelFree` closes it and
releases the handle.

## 7. String bytes and file input

**`text[i]`.** Indexing a `String` with one integer (`Int` or `Long`) yields byte `i` as a `Char`.
It compiles to a single load, with no call and no scan, so byte loops optimise like C loops (and
are vectorised at `-O2`/`-O3`). Valid indices are `0 <= i <= text.length`, where index
`text.length` is the terminating NUL. As with C strings, other indices are out of bounds and are not
checked. Slices of strings are not supported. (`text.charAt(i)` remains available; it is bounds
safe but scans from the start, i.e. O(i).)

**`std.io`.**
- `openRead(path)`, `seekTo(file, offset)`, `readChunk(file, maxBytes)`, and `lastReadCount(file)`
  read a file in binary mode in chunks. The byte count is reported separately, so data containing
  NUL bytes is exact. Each chunk is a fresh allocation; free it with `releaseChunk`.
- `mapFile(path)` maps a whole file read-only. `mappedBytes(m)` is a view for `view[i]`,
  `0 <= i < mappedSize(m)` (use a `Long` index beyond 2 GiB). It is not NUL-terminated and stays
  valid until `unmapFile(m)`. One mapping can be scanned by several threads.
- `fileSize(path)`, `parseInt(text, fallback)`, `parseLong(text, fallback)`.

**Target.** The native compiler gives every module the host's target triple and data layout before
code generation. The default CPU is the portable baseline (SSE2 on x86-64); `-march=native` targets
the build machine's CPU (e.g. AVX2/AVX-512).

`examples/parallel_wc.csec` is a parallel `wc -l` built from these pieces.

## 8. Data frames and Parquet (`import std.dataframe`)

`std.dataframe` is modelled on [Polars](https://pola.rs): eager `DataFrame`s, lazy `LazyFrame`
queries, and composable expressions. Columns have the dtypes `i64`, `f64`, `str`, and `bool`, and
any cell may be null.

```csec
import std.dataframe = pl;

def main(): Int {
    val sales = pl.readParquet("sales.parquet");
    val top = sales
        .filter((pl.col("amount") > 100) & pl.col("region").isNotNull())
        .withColumns((pl.col("qty") * pl.col("price")).alias("total"))
        .groupBy("region")
        .agg(pl.col("total").sum(), pl.col("amount").mean().alias("avg"), pl.len())
        .sort("total", true);
    println(top.toString());

    // Lazy: optimized, then run by collect().
    val q = pl.scanParquet("sales.parquet")
        .filter(pl.col("region") == "east")
        .select(pl.col("amount"), pl.col("qty"))
        .head(10);
    println(q.explain());
    val firstTen = q.collect();
    return 0;
}
```

**Expressions** (`Expr`). They are built with `col(name)`, `all()`, `lit(value)`, `len()`, and
`when(cond).then(a).otherwise(b)`.
- Arithmetic is `+ - * / %`. `/` always gives `f64`, `%` follows the divisor's sign, and `+` on two
  strings concatenates them.
- Comparisons are `== != < <= > >=`. A comparison operand may be another expression or a
  `Long`, `Double`, `String`, or `Boolean`.
- Logic is `&`, `|`, `.and()`, `.or()`, `.not()`, with Kleene nulls (false & null = false, true | null = true).
- Null handling is `isNull`, `isNotNull`, and `fillNull`.
- Math functions are `abs`, `sqrt`, `exp`, `log`, `floor`, `ceil`, `round(n)`, and `neg`.
- Other functions are `cast("i64" | "f64" | "str" | "bool")`, `isBetween(lo, hi)`, and `alias(name)`.
- String functions go through `.str()`: `contains`, `startsWith`, and `endsWith` (all literal
  matches), `lenChars`, `lenBytes`, `toUppercase`, `toLowercase`, `stripChars`, `replace`, and
  `replaceAll`.
- Aggregations are `sum`, `mean`, `min`, `max`, `count` (non-null cells), `std`, `var` (ddof 1),
  `median`, `first`, `last`, `nUnique`, and `nullCount`. In a select an aggregation is a scalar that
  broadcasts; inside `agg()` it is computed per group. `expr.over("k")` computes it per group of `k`
  and keeps one value per row (a window function).
- An output column is named after the expression's leftmost input column (`literal` for a literal,
  `len` for `len()`), or after its `alias`.

**DataFrame.**
- Shape: `height`, `width`, `shape`, `columnName(i)`, `dtype(name)`, and `schema`.
- Columns: `select(...)` (expressions, or `"a,b"`), `withColumns(...)`, `withColumn`, `drop`,
  `rename("old=new,...")`, and `withRowIndex`.
- Rows: `filter`, `head`, `tail`, `slice`, `limit`, `unique([subset])`, and `dropNulls([subset])`.
- `sort(by[, descending[, nullsLast]])` sorts by `"a,-b"` or by an expression. The sort is stable,
  and nulls come first by default, as in Polars.
- `groupBy(keys).agg(...)` (or `.len()`) keeps the groups in order of first appearance.
- `join(other, on[, how[, suffix]])` takes `on` as `"k"`, `"a,b"`, or `"l=r"`. `how` is `inner`,
  `left`, `full`, `semi`, `anti`, or `cross`.
- Summaries: `vstack`, `describe`, `nullCount`, `sum`, `mean`, `min`, `max`, `std`, and `median`.
- Cells: `getLong`, `getDouble`, `getString`, `isNull`, and `itemLong`/`itemDouble`/`itemString`.
- Output: `toString([maxRows])` gives a Polars-style table (UTF-8 box drawing, or ASCII through
  `setTableFormat("ascii")`; on a Windows console that is not in UTF-8 mode the default is ASCII).
  `writeParquet`, `writeCsv`, and `toCsv` write the frame out.
- `lazy()` turns the frame into a `LazyFrame`.

**LazyFrame.**
- Building: `scanParquet(path)`, `scanCsv(path)`, or `df.lazy()`, then the same operations as a
  DataFrame. `collect()` runs the query, `explain()` shows the optimized plan (`explain(false)`
  the plan as written), and `columns()` lists the output columns.
- Predicate pushdown: filters move below sorts, `withColumns` (when they do not read the new
  columns), group keys, and the left side of joins, down into the scan. At the scan, Parquet
  row-group min/max statistics skip row groups the filter cannot match.
- Projection pushdown: a scan reads only the columns the query uses, and unused `withColumns`
  outputs are dropped.
- Limit pushdown: `head(n)` over a scan stops reading row groups once it has `n` rows.
- `lastScanInfo()` reports what the last scan read (`row_groups=1/3 columns=2/6`).

**Input and output.** `readParquet(path[, "a,b"])`, `readCsv(path)`, `readCsvString(text)`,
`scanParquet`, `scanCsv`, and `concat(a, b)`.
- Parquet: the reader handles flat schemas, data pages v1 and v2, and the PLAIN, dictionary, RLE,
  DELTA_*, and BYTE_STREAM_SPLIT encodings.
- Compression: UNCOMPRESSED, SNAPPY, GZIP, and LZ4_RAW are built in. ZSTD, which Polars writes by
  default, needs a runtime built with libzstd (`dfZstdSupported()`; CMake `CSEC_WITH_ZSTD`, AUTO
  by default).
- Types: booleans become `bool`; integers, DATE (days), TIMESTAMP (the file's unit), and INT96
  (nanoseconds) become `i64`; floats and DECIMAL become `f64`; byte arrays become `str`. Nested and
  repeated columns are rejected; read the flat ones by name.
- Parallelism: columns are decoded in parallel, and `take`-style operations (filter, sort, join)
  gather columns in parallel.
- `writeParquet` writes uncompressed PLAIN files.

**Errors.** Like a Polars exception, a failing operation prints `DataFrame error: ...` and exits
with status 1. After `setRaiseErrors(false)` it instead returns an invalid frame (`isValid()` is
false) and `dfLastError()` explains why.

**Values, not owners.** `DataFrame`, `LazyFrame`, `Expr`, and the other API types are value structs
around runtime handles, so they copy freely without `<-`. A method takes up to six expressions per
call; chain `withColumns`/`select` calls for more.

**Procedural API.** The earlier handle-based functions remain for existing code: `dfFilter`,
`dfSort`, `dfGroupBy("k", "col:fn,...")`, `dfJoin`, `dfCompute`, `dfGetLong`, and the rest. Their
input and output functions are `dfReadParquet`, `dfReadParquetColumns`, `dfWriteParquet`,
`dfReadCsv`, `dfFromCsv`, `dfWriteCsv`, and `dfToCsv`. Before the Polars-style API these were named
`readParquet`, `readCsv`, ...; those names now return `DataFrame`s.

## 9. JSON, HTTP, and OpenAI-compatible clients

**`std.json`.** `parseJson(text)`, `jsonObject()`, `jsonArray()` return a `Json` handle.
Values are addressed by path: `""` is the root, `a.b` a member, `a[0]` an element, `a[-1]`
the last element, `a["x.y"]` a member with dots in its name (`jsonKey(name)` builds that
step for any string), and `a[]` (setters only) appends. Setters (`set`, `setNull`,
`setObject`, `setArray`, `setJson`, `push`) create missing containers and return the
document, so they chain. `get(path)` is a view that shares the subtree, `copy(path)` an
independent copy. Numbers keep their source text, so 64-bit integers round-trip exactly.
A failed parse gives a `Json` with `isValid()` false; `jsonLastError()` explains it.

**`std.net`.** Blocking TCP sockets for text data: `tcpListen`, `tcpAccept`, `tcpConnect`,
`tcpSend`, `tcpReceive`, `tcpClose` (handles are -1 on failure; run servers on a `std.thread`).

**`std.http`.** `httpRequest(method, url)` builds a request (`header`, `bearer`, `body`,
`json`, `timeout`); `send()` waits for the whole response, `open()` returns once the headers
are in so the body can be read with `nextLine()/line()` or as Server-Sent Events with
`nextEvent()/eventData()`. Shortcuts: `httpGet`, `httpPost`, `httpPostJson`, `fetchText`,
`fetchJson`, `fetchPageText`, `download`. A transport failure gives `status() == 0` and an
`error()` message; `ok()` means a 2xx status. HTTPS uses WinHTTP on Windows and libcurl
(loaded at run time) elsewhere; redirects and gzip/deflate are handled. Requests run on a
worker OS thread and the caller waits cooperatively, so microthreads can have many requests
in flight at once. Crawler helpers: `htmlTitle`, `htmlToText`, `htmlLinks(html, base)`,
`urlResolve`, `urlEncode`, `urlDecode`, `urlHost`, `sameHost`.

**`std.openai`.** `openai(baseUrl, apiKey, model)` or `openaiFromEnv()` (`OPENAI_BASE_URL`,
`OPENAI_API_KEY`, `OPENAI_MODEL`) give a client for any server implementing the OpenAI REST
API (OpenAI, Azure via `header`, Ollama, LM Studio, vLLM, llama.cpp, OpenRouter, ...).
`newChat().system(..).user(..).temperature(..)` builds a conversation; `ai.complete(chat)`
returns a `ChatResult` (`ok`, `error`, `status`, `text`, `finishReason`, token usage, tool
calls), `ai.stream(chat)` a `ChatStream` read with `next()/delta()`. Tool calling:
`chat.tool(name, description, jsonSchema)`; when `result.toolCallCount() > 0`, add
`chat.assistant(result)`, then `chat.toolResult(result.toolCallId(i), output)` per call, and
complete again. Also `ask`, `embeddings`, `models`, and raw `postJson`/`getJson`.

Examples: `examples/web_fetch.csec`, `examples/web_crawler.csec`,
`examples/openai_chat.csec`, `examples/openai_web_agent.csec`.

## 10. Dictionaries and declaration type inference

### 10.1 Declaration type inference

In `val name = expr` and `var name = expr` (also `constexpr val`), the annotation may be
omitted when the type of `expr` is known; the variable or constant then has that type. This
applies to locals, module-level globals, and class fields (`var total = 0`). Literals give
`Int` (or `Long` when the value does not fit 32 bits), `Float` for a plain decimal literal,
`String`, `Char`, and `Bool`; calls give the callee's declared return type; operators give
their result type (a comparison or `and`/`or` is always `Bool`, also when the operands are a
mix of `Int` and `Long`; a bitwise operator on mixed integer widths gives the wider type); a
dictionary literal gives its `Dict<K, V>` (§10.2), and `d[k]` gives `V`. An explicit annotation
always wins and the initializer is converted to it (`val wide: Long = 1`). An annotation is
required when the initializer has no type to infer, for example an empty `{}`.

### 10.2 Dictionary literals and `Dict<K, V>`

```
dict-lit = '{' [ expr ':' expr { ',' expr ':' expr } [ ',' ] ] '}' ;
type     = ... | 'Dict' '<' type ',' type '>' ;
```

```csec
val ages = {"alice": 30, "bob": 25};                  // Dict<String, Int>
val scores = {"math": [90, 85, 77], "art": [60, 70]}; // Dict<String, Vector[Int]>
val points = {"p": {"x": 1.5, "y": 2.0}};             // Dict<String, Dict<String, Float>>
var counts: Dict<String, Long> = {};
```

- **Parsing.** A `{` in expression position is a dictionary literal when its first top-level item
  is `key: value`, or when it is an empty `{}` used as a value (after `=`, `(`, `,`, `:`, `[`,
  `<-`, `return`); otherwise it is a block. `>>` closing nested type arguments is split into two
  `>`.
- **Typing.** `K` is the common type of the keys and `V` the common type of the values. Integer
  types widen to the wider one, integers mixed with `Float`/`Double` give the floating-point type,
  list literals give `Vector[T]` (whatever their lengths), and nested dictionaries unify
  entry-wise. Other types, class types included, must match exactly (there is no implicit upcast),
  so `{"a": 1, "b": "x"}` is a type error. When the literal initializes a declaration annotated `Dict<K, V>` (or is assigned to a
  `Dict` variable or dictionary element), that type is used and each entry must convert to it.
  `K` must be `Int`, `Long`, `Short`, `Byte`, `Char`, `Bool`, or `String`; `V` may be any type
  that is a scalar or a reference (not a value struct or tuple).
- **Semantics.** A dictionary is a reference value backed by the runtime (`dict_runtime.inc`):
  assignment and argument passing share it, `copy()` makes an independent shallow copy. Entries
  keep insertion order. String keys are compared by content (the dictionary keeps its own copy of
  the key text). List literal values are copied to the heap, so they outlive the function that
  built the dictionary.
- **Operations.** `d[k]` reads (a missing key prints `KeyError: <key>` and exits with status 1);
  `d[k] = v` inserts or replaces; `d[k] op= v` is `d[k] = d[k] op v`; `k in d` / `k notin d`.
  Methods: `size()`/`length()`/`count()` (`Long`), `isEmpty()`, `contains(k)`/`has(k)`/
  `containsKey(k)`, `get(k)`, `get(k, default)`, `remove(k)` (`Bool`: whether it was present),
  `keyAt(i)`, `valueAt(i)`, `copy()`, `clear()`, `toString()`. `print`/`println` and string
  concatenation render a dictionary like Python: `{'a': 1, 'b': [2, 3]}`.
- **Iteration.** `for (k <- d) { ... }` visits the keys in insertion order; `for (k, v <- d)` also
  binds each value. `d.keys()` returns the keys as a `Set<K>` (§10.3).

### 10.3 Set literals and `Set<T>`

```
set-lit = '{' expr ',' [ expr { ',' expr } [ ',' ] ] '}' ;   (* at least one ',' *)
type    = ... | 'Set' '<' type '>' ;
```

```csec
val primes = {2, 3, 5, 7};          // Set<Int>
val one = {42,};                    // a one-element set needs the trailing comma
var seen: Set<String> = {};         // `{}` is an empty set only when the type says Set
```

- **Parsing.** A `{` in expression position is a set literal when its top-level items are separated
  by commas and none of `:`, `;`, `=`, `<-`, `->` appears at the top level before the closing `}`.
  `{x}` without a comma stays a block expression (whose value is `x`), and a block that begins with
  a destructuring assignment (`{ q, r = f(); ... }`) stays a block. An empty `{}` is a dictionary
  unless it initializes or is assigned to something typed `Set<T>`.
- **Typing.** `T` is the common type of the elements (same rules as dictionary keys, §10.2), and
  must be `Int`, `Long`, `Short`, `Byte`, `Char`, `Bool`, or `String`.
- **Semantics.** A set is a shared reference value like a dictionary, keeps insertion order, and
  compares String elements by content. It prints like Python: `{1, 2, 3}`, and `set()` when empty.
- **Operators.** `a | b` / `a union b`, `a & b` / `a intersect b`, `a - b` / `a diff b` /
  `a without b`, `a ^ b` / `a symdiff b` produce a new set; `a == b`, `a != b`, `a <= b` /
  `a subset b`, `a >= b` / `a superset b`, `a < b` / `a properSubset b`, `a > b` /
  `a properSuperset b` are `Bool`; `x in s`, `x notin s`. Both sides must have the same element
  type. (On `Long` bitsets the keyword operators keep their existing meaning.)
- **Methods.** `add(x)` and `remove(x)`/`discard(x)` return whether the set changed;
  `contains(x)`/`has(x)`, `size()`/`length()`/`count()`, `isEmpty()`, `clear()`, `copy()`,
  `elementAt(i)`, `union(s)`, `intersection(s)`, `difference(s)`, `symmetricDifference(s)`,
  `isSubset(s)`, `isSuperset(s)`, `isDisjoint(s)`, `toString()`.
- **Iteration.** `for (x <- s) { ... }` visits the elements in insertion order.

## 11. Rings and fields (`import std.math.algebra`)

Algebraic structures are ordinary values with operator overloading (struct `operator` methods,
including the unary `def operator -(): T`). A structure is created at run time with its parameters
checked; its elements remember the structure, and combining elements of different structures stops
the program with `AlgebraError: ...` (exit status 1).

| Structure | Created by | Element | Operators |
| --- | --- | --- | --- |
| ring Z/nZ (n >= 1) | `integersMod(n)` | `Zmod` | `+ - *`, `/` (by a unit), unary `-`, `** e` (e < 0 uses the inverse), `== !=`; also with a `Long` |
| prime field GF(p) | `primeField(p)` -- `p` must be prime | `Fp` | as `Zmod`, `/` by any nonzero element |
| polynomial ring GF(p)[x] | `F.x()`, `F.constant(c)`, `F.monomial(c, k)` | `PolyFp` | `+ - *`, `/` and `%` (Euclidean), unary `-`, `** e`, `== !=`; with `Long` and `Fp` |
| finite field GF(p^k) | `galoisField(p, k)` or `galoisField(m)` for an irreducible `m` | `GFq` | as `Fp` |

```csec
import std.math.algebra

val F = primeField(7);                 // primeField(8): AlgebraError, use galoisField(2, 3)
val a = F.of(3);
val c = a * F.of(5) + 1;               // 2 (mod 7)
val x = F.x();
val f = x ** 2 + x * 3 + 1;            // irreducible over GF(7)
val K = galoisField(2, 8);             // modulus x^8 + x^4 + x^3 + x + 1
val g = K.generator();
val h = (g ** 7 + g + 1).inverse();
```

- **Arithmetic.** Residues are kept in `[0, n)`; products use 128-bit intermediates, so every
  modulus below 2^63 is exact. Primality is a deterministic Miller-Rabin test for 64-bit integers.
- **Z/nZ** (`IntegersMod`): `of`, `zero`, `one`, `modulus`, `order`, `unitCount` (Euler's phi),
  `isField`. Elements: `value`, `modulus`, `isZero`, `isUnit`, `inverse`, `order`, `toString`
  (`"5 (mod 12)"`).
- **GF(p)** (`PrimeField`): `of`, `zero`, `one`, `characteristic`, `order`, `generator` (a
  primitive root), `x`, `constant`, `monomial`. Elements add `legendre`, `isSquare`, and `sqrt`
  (Tonelli-Shanks; the smaller root).
- **GF(p)[x]** (`PolyFp`): `degree` (-1 for zero), `coeff(i)`, `leading`, `eval(v)`, `derivative`,
  `monic`, `gcd` (monic), `isIrreducible` (Rabin's test), `toString` (`"x^2 + 3x + 1"`).
  Polynomials are runtime handles that are not freed.
- **GF(p^k)** (`GaloisField`): `characteristic`, `degree`, `order`, `modulus`, `zero`, `one`,
  `of(c)`, `of(f)` (the class of a polynomial), `generator` (the class `a` of x),
  `primitiveElement`, `element(packed)`. `galoisField(p, k)` uses the smallest monic irreducible
  polynomial of degree k (ordered by base-p digits). An element is packed in one `Long` (its
  coefficients as base-p digits), so p^(k+1) must stay below 2^62. Elements: `inverse`, `order`,
  `poly`, `packed`, `toString` (in the generator `a`, e.g. `"a^7 + a + 1"`).
- The structures above are values: `GF(7)` and `GF(11)` elements share the type `Fp` and are told
  apart at run time.
- **Type-level moduli.** `Mod<N>` (Z/NZ) and `GF<P>` (the prime field) are template structs whose
  modulus is part of the type: `new GF<7>(3) * new GF<5>(2)` is a compile-time type error
  (`no 'operator*' of GF<7> accepts an operand of type GF<5>`). They have the same operators as
  `Zmod` / `Fp` (and `inverse`, `order`, `sqrt`, `toString`, ...). `GF<P>` checks that `P` is prime
  on use (once per prime, then cached), so `GF<8>` stops with an AlgebraError. Moduli up to 2^63
  are exact: `new GF<9223372036854775783>(...)`.

Template classes and structs (`template<Int N> struct S(...) { def operator +(o: S<N>): S<N> ... }`)
may refer to their own instance type; every instance (`S<7>`, `S<11>`) is a distinct class, and an
operator whose overloads accept none of the right operand's types is a type error. Instances used
while the program is type-checked are registered then and generated with the rest of the code.

## 12. Contracts and static verification (`csec++ --verify`)

Functions and loops may carry specifications that `csec++ --verify` proves with the Z3 SMT solver
(an external library; see "Building with Z3" below). Specifications are never compiled: `requires`,
`ensures`, `invariant`, and `decreases` only affect `--verify`. `assert` is both proved by `--verify`
and checked at run time.

```
function   = ... 'def' fn-name '(' [ params ] ')' [ ':' type ] { contract } fn-body ;
contract   = 'requires' expr | 'ensures' expr ;
while-stmt = 'while' '(' expr ')' { loop-spec } block ;
for-stmt   = 'for' '(' identifier '<-' iterable ')' { loop-spec } block ;
loop-spec  = 'invariant' expr | 'decreases' expr ;
assert     = 'assert' '(' expr [ ',' expr ] ')' ;
```

`requires`, `ensures`, `invariant`, `decreases`, and `result` are contextual words: they keep
working as ordinary identifiers elsewhere. A clause expression ends where the body starts (`{`, or
`=` for an expression body), so `def twice(x: Int): Int requires x < 1000 = x * 2` is valid.

```csec
def safeDiv(a: Int, b: Int): Int
    requires b != 0 and !(a == -2147483648 and b == -1)
    ensures result * b + a % b == a
{
    return a / b;
}

def sumTo(n: Int): Int
    requires 0 <= n and n <= 10000
    ensures result == n * (n + 1) / 2
{
    var s = 0;
    var i = 0;
    while (i < n)
        invariant 0 <= i and i <= n
        invariant s == i * (i + 1) / 2
        decreases n - i
    {
        i = i + 1;
        s = s + i;
    }
    return s;
}
```

### 12.1 What is checked

For every function defined in the input file (imported modules contribute only their contracts):

- `requires` clauses must be satisfiable together (otherwise the function can never be called);
  they are assumed in the body.
- every `return` (and the end of a `Unit` function) satisfies each `ensures`; `result` is the
  returned value;
- every `assert(c)` holds where it is reached; afterwards `c` is assumed;
- every call to a function with contracts satisfies the callee's `requires`; the result is known
  only through the callee's `ensures` (calls are verified modularly, so recursion needs no
  special handling); a callee without contracts returns an unknown value;
- integer division and remainder: the divisor is not zero, and `MIN / -1` does not occur;
- shifts: the amount is in `[0, width)`;
- integer arithmetic does not overflow its type (`Int` is 32-bit, `Long` 64-bit, `Short` 16-bit,
  `Byte`/`Char` 8-bit), and values are not narrowed to a type that cannot hold them;
- loops: each `invariant` holds on entry and is preserved by an iteration that starts from an
  arbitrary state satisfying the invariants and the loop condition; after the loop the invariants
  hold and the condition is false. A `decreases` measure is non-negative at the start of an
  iteration and strictly smaller at its end, so the loop terminates. A range loop
  `for (i <- a to b)` also has the implicit invariant `i >= a`; a loop without invariants is still
  checked, but nothing is known afterwards about the variables it assigns (reported as a note);
- `if` conditions that are always true or always false at that point are reported as warnings.

A failed check is reported with the file and line and, when the solver found one, a
counterexample: the parameters' values and the local variables at that point:

```
02_contract_errors.csec:12: error: 'a + b' may overflow Int
    counterexample: a = 2147483647, b = 1
```

`csec++ --verify` exits with status 1 when there is an error, 0 otherwise (warnings and notes do not
fail). No code is generated in this mode.

### 12.2 Integer semantics

- **Default:** integers are mathematical integers constrained to their type's range, and every
  operation must provably stay in range (an overflow is an error). As long as no overflow is
  reported, this agrees with the compiled program. Contract expressions (`requires`, `ensures`,
  `invariant`, `decreases`) are evaluated with mathematical integers without overflow checks,
  so a specification such as `s == i * (i + 1) / 2` means the mathematical value.
- **`--verify-wrap`:** integers are bit-vectors of their width and wrap around exactly as the
  compiled program does (for code that relies on wrap-around, such as hashing); overflow is not an
  error. `--verify-overflow` additionally reports where it may happen. Non-linear bit-vector
  arithmetic (`*`, `/`, `%` on unknowns) can be much harder for the solver in this mode.
- `--verify-timeout <ms>` sets the per-check solver time limit (default 10000). A check the solver
  cannot decide in time is reported as an error with "could not decide"; nothing is assumed proven.

### 12.3 Modelled fragment

Modelled: `Int`, `Long`, `Short`, `Byte`, `Char`, `Bool` locals and parameters; literals;
arithmetic, comparison, logical (`and`, `or`, `!`, `implies`, `iff`, `xor`), and bitwise operators;
`val`/`var`, assignment (`=`, `op=`, `++`, `--`), `if`/`else` (also as an expression), `while`,
range `for`, `return`, `assert`, calls to top-level functions. Everything else (other types,
methods, fields, collections, globals) is an unknown value: the verifier may then report a check
it cannot prove, but it never proves a false property. Such places are listed as notes. Methods of
classes and structs are not verified yet.

### 12.4 Building with Z3

Z3 is an external C++ library. `scripts/fetch_z3.ps1` (Windows) or `scripts/fetch_z3.sh` (macOS,
Linux) downloads the official release into `deps/z3` (the Windows script checks its SHA-256); CMake
then finds it (`CSEC_WITH_Z3=AUTO`, or set `Z3_ROOT`; a system install is found too) and copies
`libz3.dll` next to `csec++.exe`. Without Z3, `csec++` builds normally and `--verify` reports that it
is unavailable.

`-DCSEC_Z3_LINK=STATIC` links a static Z3 into csec++ and into the System.Native runtime instead,
so neither needs the shared library at run time (the default, `SHARED`, links `libz3`). The official
releases ship no static library; `scripts/build_z3_static.ps1` / `scripts/build_z3_static.sh` build
one from the same Z3 version into `deps/z3-static` (on Windows matching csec++'s C runtime:
`-Config Debug` or `-Config Release [-DllCrt]`).

The self-hosted compiler accepts all of this syntax: it drops the specification clauses and
compiles `assert` as a run-time check (§5).

## 13. Logic and proofs (`import std.logic`)

`std.logic` makes formulas values, decides them with the Z3 solver, and checks proofs in a small
LCF-style kernel.

- **Formulas and terms.** `prop(name)` is a proposition, `intVar(name)` an integer variable,
  `intVal(n)` an integer constant, `trueFormula()` / `falseFormula()`. Terms combine with `+ - *` (also
  with a `Long`) and unary `-`; comparisons `== != < <= > >=` build formulas; formulas combine with
  `&`, `|`, `!`, and the methods `implies` and `iff`; `forAll(x, f)` and `exists(x, f)` quantify an
  integer variable. Integer terms are mathematical integers (no overflow). Formulas are hash-consed:
  `f.sameAs(g)` is syntactic equality; `f.substitute(x, t)` substitutes without capture.
- **Decision.** `isValid(f)`, `isSatisfiable(f)`, `counterexample(f)` (an assignment making `f`
  false, `""` if valid), `model(f)` (one making it true), `equivalent(f, g)`. An undecided query
  counts as "not proved". Z3 is loaded when first needed: `libz3` next to the program, on the
  library path, or named by `CSEC_Z3_LIBRARY`; `z3Available()` tells whether it was found.
- **Theorems.** A `Thm` is `hypotheses |- conclusion` and can only be produced by the kernel's rules:
  `assume(f)`, `trueIntro()`, `impIntro(a, t)` (discharges `a`), `impElim`/`modusPonens`,
  `andIntro`, `andElim1`, `andElim2`, `orIntro1(t, b)`, `orIntro2(a, t)`, `orElim(tOr, fromA, fromB)`,
  `notIntro(a, t)`, `notElim(tA, tNotA)`, `exFalso(t, f)`, `byContradiction(a, t)` (classical),
  `iffIntro`, `iffElim1`, `iffElim2`, `allIntro(x, t)` (x not free in the hypotheses),
  `allElim(t, witness)`, `existsIntro(f, witness, t)`, `refl(t)`, `rewrite(eq, t)`, and
  `decide(f)`, which adds `|- f` when Z3 proves `f` valid and marks the theorem (and every theorem
  derived from it) as relying on that oracle (`usesOracle()`, printed as `[by Z3]`). A rule applied
  to theorems of the wrong shape stops with `LogicError: ...`. `t.isProofOf(f)` is true for a closed
  theorem whose conclusion is exactly `f`.

```csec
val p = prop("p");
val q = prop("q");
val h = assume(p & q);
val swap = impIntro(p & q, andIntro(andElim2(h), andElim1(h)));
swap.toString();                          // "|- p & q -> q & p"
isValid(forAll(intVar("x"), intVar("x") * intVar("x") >= intVal(0)));   // true
```

The kernel is independent of `csec++ --verify` (§12): contracts are proved by the verifier, while
`std.logic` reasons about formulas a program builds. Like other struct-based modules, it is
native-only (§5).

## 5. Self-hosted compiler support

The self-hosted compiler (`selfhost/`, whose emitter lives in `src/native_runtime/*.inc`) implements:

- the standard-library search path (§1.1) — it shares the runtime's import resolver;
- `[@...]` attributes as their own top-level declarations, and `[@Intrinsic]` declarations,
  which emit no `declare` and enable the intrinsic lowering (§1.2);
- all `std.microthread` intrinsics and `threadStart`;
- `go`, `async`, and `await` (§3), including `async def` as a declaration modifier, with
  futures of `Int`, `Long`, `Boolean`, `Double`, and pointer-represented results;
- `text[i]` on String locals and parameters, and escaped `Char` literals (`'\n'`, `'\t'`, ...);
- the contract syntax of §12: `requires`/`ensures`/`invariant`/`decreases` clauses are accepted and
  dropped (verification is `csec++ --verify`), and `assert(c[, message])` is a run-time check;
- dictionaries (§10.2) with scalar keys and values (`Int`, `Long`, `Short`, `Byte`, `Double`,
  `Float`, `Boolean`, `Char`, `String`): literals, `val d: Dict<K, V> = {}`, `d[k]`,
  `d[k] = v`, `d[k] op= v`, `get`, `size`, `isEmpty`, `contains`, `remove`, `clear`, `Dict`
  parameters and return types, and local type inference from `d[k]`/`d.get(..)`/`d.size()`;
- sets (§10.3) of those scalar types: literals, `val s: Set<T> = {}`, `add`, `remove`, `discard`,
  `contains`, `size`, `isEmpty`, `clear`, and `Set` parameters;
- calls through `std.io`, `std.thread`, `std.process`, `std.pipe`, `std.shared`, `std.mail`,
  `std.math.natural`, `std.math.real`, and `std.symbolic`, which are ordinary functions
  over `external def` runtime entry points. `external def` declarations are emitted with
  their declared signature, and calls to them pass arguments of the declared types.

Narrower than the native compiler:

- `spawn`, `threadStart`, `go`, and `async` take **named functions** only; lambda
  literals are native-only.
- The value aggregates of §4 (`Complex`, `Rational`, `Quaternion`) and therefore
  `std.math.complex` are native-only, because the self-host has no aggregate value
  types. For the same reason `std.math.algebra` (§11) and `std.logic` (§13), whose values
  are structs, are native-only.
- The self-host does not type-check: the share-nothing and borrow rules of §2–§3 are
  enforced by the native compiler only.
- Set operators (`|`, `&`, `-`, `^`, comparisons, `in`), `for` over a set, and the set-algebra
  methods are native-only.
- Dictionary values that are lists or nested dictionaries, `for` over a dictionary, `k in d`,
  `keyAt`/`valueAt`/`copy`/`toString`, printing a dictionary, and calling a method on a
  dictionary read in one expression (`d[k].length()`; bind `d[k]` to a local first) are
  native-only.

`tests/run_selfhost_concurrency.ps1` runs every fixture in `tests/selfhost_concurrency`
through both compilers and requires the same exit code (42) from each.
