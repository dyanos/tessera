# Tessera Syntax Overview

This document summarizes the current Tessera syntax supported by the compiler
prototype. The language is still experimental, so this file describes the
implemented surface rather than a stable specification. For the normative
grammar, see [grammar.md](grammar.md).

## Files And Entry Point

Source files use the `.csec` extension. Executable programs usually define a
zero-argument `main` function:

```ts
def main(): Int {
    return 0;
}
```

A bare `csec++ app.csec` compiles straight to a native executable (`--emit-exe`
is the default output mode); `--static` links the runtime into the binary so it
needs no `System.Native` shared library beside it. `--emit-ir`, `--emit-obj`,
and `--run`/`--run-ir` select the other output modes.

## Declarations

Variables are declared with `val` or `var`. Use `val` for single-assignment
bindings and `var` for values that are reassigned:

```ts
val count: Int = 3;
var scale: Double = 0.5;
val label: String = "run";
scale = scale + 1.0;
```

The type annotation is optional when the declaration has an initializer whose type is known: the
variable (or constant) then takes the initializer's type.

```ts
val n = 3;                    // Int
val big = 3000000000;         // Long (does not fit an Int)
val ratio = 0.5;              // Float (a plain decimal literal is a Float)
val name = "run";             // String
val done = n > 2;             // Bool, also for mixed Int/Long comparisons
val total = sumAll(xs);       // the function's declared return type
val ages = {"alice": 30};     // Dict<String, Int>
var hits = ages["alice"] + 1; // Int, the dictionary's value type
```

An annotation is still required when there is nothing to infer from, e.g. an empty
dictionary `val d: Dict<String, Int> = {}`, and it wins over the initializer's type
(`val wide: Long = 1`).

Functions use `def` with typed parameters and return type:

```ts
def add(left: Int, right: Int): Int {
    return left + right;
}
```

External native functions can be declared with `external def`:

```ts
external def puts(message: String): Int;
```

Native library metadata can be attached with attributes:

```ts
[@DllImport("legacy_stdio_definitions.lib", "puts")]
external def nativePuts(message: String): Int;

def main(): Int {
    nativePuts("hello");
    return 0;
}
```

Static/import libraries can be linked either from the command line or from
source attributes:

```sh
csec++ --emit-exe app.csec -o app.exe --link-path ./native/lib --link-lib mymath.lib
csec++ --emit-exe app.csec -o app --link-path ./native/lib --link-lib libmymath.a
```

```ts
[@LinkPath("./native/lib")]
[@LinkLibrary("mymath.lib")]
[@StaticLibraryImport("mymath.lib", "native_add")]
external def nativeAdd(left: Int, right: Int): Int;

[@CppImport("mymath.lib", "double Math::cos(double)", "msvc")]
external def mathCos(value: Double): Double;

def main(): Int {
    println(nativeAdd(2, 3));
    return 0;
}
```

On Windows, pass `.lib` files. On Unix-like platforms, pass `.a`, `.so`,
`-lname`, or a direct library path. `StaticLibraryImport` links the library and
declares the external symbol at the same time; `LinkLibrary` only adds a link
input. `CppImport` receives a C++ function signature and generates an ABI
mangled symbol name. The optional third argument is `msvc` or `itanium`; when it
is omitted, the current platform default is used.

For inspection, the compiler can print generated names directly:

```sh
csec++ --mangle msvc "int add(int, int)"
csec++ --mangle itanium "double Math::cos(double) const"
```

## Primitive Types

Common scalar types include:

- `Int`
- `Long`
- `Double`
- `Bool`
- `Char`
- `String`

## Expressions

The compiler supports standard arithmetic, comparison, boolean, bitwise, and
assignment-style expressions:

```ts
val a: Int = 2 + 3 * 4;
val b: Bool = a >= 10 and a != 0;
val c: Int = (a << 1) | 1;
```

Supported boolean operators include `and`, `or`, and `xor`.

String literals are inferred as `String`. If either side of `+` is `String`,
the other primitive value is converted with the runtime `toString` path:

```ts
val message = "score=" + 4 + ", ok=" + true;
```

Plain string literals are Unicode by default. `u"..."` is accepted as an
explicit Unicode string prefix, and `r"..."` creates a raw regular-expression
literal for `match` patterns:

```ts
val route = request match {
    case "GET / HTTP/1.1" => "home";
    case r"^GET /users/[0-9]+ HTTP/1\.[01]" => "user";
    case u"GET /안녕 HTTP/1.1" => "hello";
    case _ => "not_found";
};
```

Implemented `String` members include:

```ts
text.length;
text.size();
text.count();
text.isEmpty();
text.contains("needle");
text.startsWith("prefix");
text.endsWith("suffix");
text.indexOf("needle");
text.substring(1, 3);
text.charAt(0);
text.toUpper();
text.toLower();
text.trim();
text.toString();
```

## Classes, Overloading, And Overriding

Classes can declare fields and methods. Methods can be overloaded by parameter
types:

```ts
class Formatter() {
    def show(value: Int): Int {
        return value + 1;
    }

    def show(value: String): String {
        return value + "!";
    }
}
```

Subclasses can override a superclass method with the same parameter signature:

```ts
class Child extends Formatter {
    override def show(value: String): String {
        return "child=" + value;
    }
}
```

Functions, methods, and operators can be overloaded by parameter types. A call picks the exact
match; failing that, the overload that needs the least numeric widening (`Int` -> `Long` ->
`Double`), so `v * 2` prefers `operator *(k: Long)` over `operator *(k: Double)` and `lit(1)` picks
`lit(v: Long)`. A keyword can name a method and be used after `.` (`frame.filter(...)`,
`xs.map(...)`). Classes and structs may refer to each other in any order.

Operator overloads are declared as `def operator <op>(...)` inside a class or struct.
For example, `counter + 4` calls `counter.operator+(4)`:

```ts
class Counter() {
    def operator +(right: Int): Int {
        return right + 1;
    }
}
```

A unary operator (`-`, `+`, `!`, `~`) is overloaded by an `operator` method without parameters,
next to the binary form of the same operator:

```ts
struct Money(cents: Long) {
    def operator -(): Money = new Money(0 - cents)          // -m
    def operator -(o: Money): Money = new Money(cents - o.cents)  // m - n
}
```

## Structs

A `struct` declares a value type — its fields are copied field-by-field on
assignment or argument passing, like a C struct. Structs may declare methods:

```ts
struct Point(x: Int, y: Int) {
    def sum(): Int = x + y
}

def main(): Int {
    val p: Point = new Point(3, 4);
    val q: Point = p;          // copy: p and q are independent
    return p.sum() + q.sum();  // both still usable
}
```

A `class`, by contrast, is a reference type with identity (see *Memory And
Ownership*).

## Multiple Return Values And Destructuring

A function can return several values by declaring a tuple return type and
listing the values after `return`:

```ts
def load(): (Int, Int, Int) {
    return 10, 20, 12;
}
```

The result is unpacked at the call site by listing the destination names. Use
`_` to ignore a component:

```ts
def main(): Int {
    a, b, c = load();      // binds a, b, c
    val q, _ = divmod();   // ignore the second value
    return a + b + c;
}
```

## Memory And Ownership

Tessera uses a Rust-lite ownership model that is **strict by default**: values of
an *owned* (non-copy) type must be explicitly moved with `<-` or borrowed with
`&`/`&mut` — you cannot silently make a second owner. Pass `--no-strict-ownership`
to fall back to the earlier box-only checking for unmigrated code.

**Copy types** are duplicated freely with no marker: the scalar primitives
(`Int`, `Long`, `Float`, `Double`, `Bool`, `Char`, …), `String` (immutable and
shared), references, function values, and value `struct`s.

**Owned types** are affine and must move or be borrowed: reference `class`es,
arrays/vectors, `box T`, and the owned container/smart-pointer generics
(`Shared`, `Weak`, …).

```ts
class Account(balance: Int) { def balance(): Int = balance }

def peek(acct: &Account): Int = acct.balance()   // borrow (read-only)
def close(acct: Account): Int = acct.balance()   // takes ownership (consumes)

def main(): Int {
    val acct: Account = new Account(100);
    val a = peek(&acct);        // borrow: acct stays owned, still usable
    val b = close(<- acct);     // move: ownership transferred; acct is gone now
    // using `acct` here => compile error: "use of moved value 'acct'"
    return a + b;
}
```

- `<- x` moves the owned value `x` (the source is consumed).
- `&x` / `&mut x` borrow `x`; a borrow is a temporary reference that must not
  outlive the value. Member access, indexing, and method calls auto-deref a
  borrowed receiver (`ref.field`, `ref[i]`, `ref.method()`).
- A **temporary** (a literal, `new X()`, a call result, a computed value) is an
  rvalue with no other owner, so it moves implicitly — only a *named* owned
  local needs an explicit `<-`.

**Explicit heap ownership** is available through `box T`:

```ts
var b: box Int = 7;      // owns a heap Int
var c: box Int = clone(b); // deep copy; b is still valid
val n = consume(<- b);   // move b into consume()
free(c);                 // release c's memory early
```

`clone(x)` produces a fresh owned copy; `free(x)` consumes and releases a named heap value.
Automatic scope-exit destruction currently covers `box`, `Tensor`, `Shared`, and `Weak`; heap
classes and arrays/vectors must be released explicitly with `free(x)` until ownership-aware drop
glue is implemented.

**Shared / Weak** provide reference counting for shared ownership:

```ts
val s = Shared(42);   // strong count 1
val s2 = s.clone();   // retains (strong count 2)
val w = Weak(s);      // non-owning handle
val value = s.get();  // read the payload
val maybe = w.get();  // payload while a strong owner is alive, else a zero value
```

The control block is freed only when both the strong and weak counts reach
zero, so a `Weak` edge can break an ownership cycle.

Two escape hatches remain available: `unsafe def ... { ... }` for raw-pointer
work, and the `box`/borrow rules above for everything else.

## Asynchronous Functions

An `async def` declares an asynchronous function; `await` (a prefix operator)
suspends until an awaited async result is ready and is valid only inside an
`async def`:

```ts
async def fetch(): Int = 42
async def run(): Int {
    val x: Int = await fetch();
    return x;
}
def main(): Int {
    return run();
}
```

The ownership rule extends across suspension points: a borrow (`&`/`&mut`) may
not be held live across an `await`, because after suspension another task could
mutate or drop the borrowed value. End the borrow before awaiting. Calling an
`async def` directly still runs it synchronously, and awaiting its (non-future)
result yields the value. For real concurrency use `async f(x)`, which runs the
call in a microthread and yields a `Future<T>` to `await` (see *Processes*
below).

## Processes (Erlang-Style Microthreads)

Lightweight processes are spawned with `spawn` and communicate only by message
passing. Each process has its own stack and a private FIFO mailbox of `Int`
messages; processes share nothing. The builtins come from the standard library
module `std.microthread` (see *Standard Library* below and `LANGUAGE_SPEC.md`).

```tessera
import std.microthread

def main(): Int {
    val parent = self();
    val worker = spawn([=]() -> {
        val n = receive();          // blocks this process only
        send(parent, n * 2);
        return 0;
    });
    send(worker, 21);
    return receive();               // 42
}
```

| Builtin | Meaning |
| --- | --- |
| `spawn(f): Int` | Start a process running `f`; returns its pid. `f` is a zero-parameter `[=]` lambda literal or the name of a zero-parameter function. |
| `self(): Int` | Pid of the current process (`main` is pid 1). |
| `send(pid, msg: Int): Int` | Append `msg` to `pid`'s mailbox. Returns 1, or 0 if `pid` is no longer alive (not an error). |
| `receive(): Int` | Take the oldest message, blocking while the mailbox is empty. |
| `receiveTimeout(ms, fallback): Int` | Like `receive`, but returns `fallback` if nothing arrives within `ms` milliseconds (Erlang's `after`). |
| `yield(): Int` | Let other runnable processes run. |
| `isAlive(pid): Boolean` | Whether `pid` has not finished yet. |
| `monitor(pid, tag: Int): Int` | Deliver `tag` to the caller when `pid` exits (immediately if it already has). |
| `mailboxSize(): Int` | Number of messages waiting for the current process. |
| `processCount(): Int` | Number of live processes, including the caller. |
| `joinProcesses(): Int` | Run other processes until none can make progress; returns how many are still alive (blocked forever). |

Semantics:

- **Share nothing.** A spawned lambda's by-value captures are copied into the
  new process when it is spawned, so later changes on either side are not
  visible to the other. `spawn` rejects `[&]` lambdas, lambdas with
  parameters, and closure-valued variables (pass the lambda literal directly).
- **Scheduling.** Cooperative and M:1: all processes run on the OS thread that
  spawned them, switching only at `receive`/`receiveTimeout` (with an empty
  mailbox), `yield`, `joinProcesses`, and process exit. A process that loops
  without doing any of these keeps the others from running. Processes are run in
  FIFO order. (Fibers on Windows; an assembly stack switch on x86-64/arm64 Linux and
  macOS, `ucontext` on other POSIX targets; 512 KiB reserved stack
  per process.)
- **Lifetime.** When `main` returns the program exits, even if other processes
  are still alive; call `joinProcesses()` first to let them finish. If every
  process is blocked in `receive` with no timeout pending, the runtime reports
  `deadlock` and exits with code 70.
- Without `import std.microthread` these names are ordinary identifiers.

### `go`, `async`, and `await`

`go f(args)` runs a call in a new microthread and discards its result (it yields
the new pid). `async f(args)` does the same but yields a `Future<T>`; `await`
blocks the current microthread until the future is ready and yields the `T`.
Arguments are evaluated immediately in the caller and copied into the task.

```tessera
def square(n: Int): Int = n * n

def main(): Int {
    val a = async square(6);
    val b: Future<Int> = async square(7);
    go square(1);                 // fire and forget
    return await a + await b;     // 85
}
```

`await` of a non-future value is only allowed inside an `async def` (which still
runs synchronously when called). The operand of `go`/`async` must be a call of a
named function and may not take borrows.

## Standard Library

`import a.b.c` also searches the standard-library roots (`lib/` next to the
compiler runtime, `--lib-path <dir>`, and `CSEC_LIB_PATH`).

An import can name its module: `import std.dataframe = df;` lets that file write
`df.readParquet(...)` for any top-level function of the module. The unqualified
names still work. A variable that shares the alias name keeps its own members, and
string literals and comments are left as they are (LANGUAGE_SPEC.md 1.1).

The shipped modules live in `lib/std`:

| Module | Highlights |
| --- | --- |
| `std.microthread` | `spawn`, `send`, `receive`, `receiveTimeout`, `self`, `yield`, `monitor`, `joinProcesses` |
| `std.thread` | `threadStart(lambda)`, `threadJoin`, `mutexNew/Lock/TryLock/Unlock`, `atomicNew/Add/Get/CompareExchange`, `channelNew/Send/Receive/ReceiveTimeout/Closed/Close/Free` |
| `std.dataframe` | Polars-style: `readParquet`, `scanParquet`, `col`, `lit`, `when`, `len`; `DataFrame.filter/select/withColumns/groupBy().agg/sort/join/unique/describe`; `LazyFrame.collect/explain` (LANGUAGE_SPEC.md §8) |
| `std.process` | `runCommand`, `commandOutput`, `startProcess`, `waitProcess`, `killProcess`, `currentProcessId` |
| `std.math.natural` | `natDiv`, `natMod`, `natPow`, `natGcd`, `natLcm`, `natSqrt`, `isPrime`, `factorial`, `binomial`, `fibonacci` |
| `std.math.real` | `pi()`, `euler()`, `approxEqual`, `hypot`, `cbrt`, `nthRoot`, `logBase`, ... |
| `std.math.complex` | `imaginaryUnit()`, `complex`, `conj`, `cabs`, `carg`, `cexp`, `clog`, `csqrt`, `cpowInt`, `cpow`, `csin`, `ccos` |
| `std.math.algebra` | `integersMod(n)` (Z/nZ), `primeField(p)` (GF(p), p must be prime), `F.x()` (GF(p)[x]), `galoisField(p, k)` (GF(p^k)); elements use `+ - * / % **`, unary `-`, `==` (LANGUAGE_SPEC.md §11) |
| `std.symbolic` | `sym("x^2+1")`, `diff`, `integrate`, `expand`, `simplify`, `subst`, `solve`, `evalAt`, `symText`, `symLatex` |
| `std.pipe` | `pipeNew`, `pipeWriteLine`, `pipeReadLine`, `pipeClose`, `pipeAtEnd`; `pipeOpenProcess(cmd)` with `processWriteLine`/`processReadLine`/`processFinish` |
| `std.shared` | `sharedNew(bytes)`, `sharedOpen(name, bytes)`, `sharedGet/SetLong`, `sharedAddLong`, `sharedRead/WriteString`, `sharedLock/Unlock` |
| `std.mail` | `mailSender(box, name)`, `mailReceiver(box)`, `mailSend`, `mailReceive`, `mailReceiveTimeout`, `mailLastSender`, `mailLastTag` |
| `std.io` | `openRead`, `readChunk`, `lastReadCount`, `releaseChunk`, `mapFile`, `mappedBytes`, `mappedSize`, `fileSize`, `parseInt` |
| `std.json` | `parseJson`, `jsonObject`, `jsonArray`; `Json.getString/getLong/getDouble/getBool/has/kind/length/key/get/copy/set/push/remove/toString/pretty`; `jsonQuote`, `jsonKey` (LANGUAGE_SPEC.md §9) |
| `std.net` | `tcpConnect`, `tcpListen`, `tcpAccept`, `tcpSend`, `tcpReceive`, `tcpClose` |
| `std.http` | `httpRequest(method, url).header/bearer/json/timeout.send/open`, `httpGet`, `fetchText`, `fetchJson`, `download`; `nextLine`, `nextEvent` (SSE); `htmlTitle`, `htmlToText`, `htmlLinks`, `urlResolve`, `urlEncode`, `urlHost` |
| `std.openai` | `openai(base, key, model)`, `openaiFromEnv()`; `newChat().system/user/tool/temperature`; `complete`, `stream`, `ask`, `embeddings`, `models`; tool calls via `assistant(result)` + `toolResult(id, output)` |

Pipes, shared memory, and mail work between OS threads and microthreads, and named
shared memory and mailboxes also work between OS processes. Their blocking
operations yield to other microthreads while waiting. Microthreads can also exchange
strings directly with `sendText(pid, text)` / `receiveText()`. See
`LANGUAGE_SPEC.md` section 6.

```tessera
import std.mail
import std.microthread

def main(): Int {
    val inbox = mailReceiver("orders");
    spawn([=]() -> {
        mailSend(mailSender("orders", "kitchen"), "pizza");
        return 0;
    });
    val order = mailReceive(inbox);          // "pizza"; the sender ran while main waited
    val from = mailLastSender(inbox);        // "kitchen"
    return 0;
}
```

A module enables compiler intrinsics by declaring them with
`[@Intrinsic] external def name(...)`; this is how `std.microthread` and
`std.thread` expose `spawn`/`threadStart`.

`Complex`, `Rational`, and `Quaternion` are built-in value types:
`new Complex(1.0, 2.0) * new Complex(0.0, 1.0)`, with fields `.re`/`.im`.

```tessera
import std.symbolic
import std.math.complex

def main(): Int {
    val f = sym("x^3 + 2*x");
    val df = symText(diff(f, "x"));          // "3*x^2 + 2"
    val i = imaginaryUnit();
    val minusOne = i * i;                    // -1 + 0i
    return 0;
}
```

Rings and fields (`std.math.algebra`) are values with overloaded operators:

```tessera
import std.math.algebra

def main(): Int {
    val F = primeField(7);                  // GF(7); a non-prime stops with AlgebraError
    val a = F.of(3);
    val b = a * F.of(5) + 1;                // 2 (mod 7)
    val inv = a ** -1;                      // 5 (mod 7)
    val R = integersMod(12);                // Z/12Z, a ring that is not a field
    val u = R.of(5).inverse();              // 5 (mod 12); R.of(4).inverse() is an error
    val x = F.x();
    val f = x ** 2 + x * 3 + 1;             // a polynomial in GF(7)[x]
    val r = f % (x + 1);                    // Euclidean remainder: 6
    val K = galoisField(2, 8);              // GF(256)
    val g = K.generator();
    println((g ** 7 + g + 1).inverse().toString());
    return 0;
}
```

### String bytes

`text[i]` reads byte `i` of a String as a `Char` with a single load, which is what byte-scanning
loops need (it is vectorised at `-O3`). It is valid for `0 <= i <= text.length` and is not
bounds-checked. The bounds-safe but O(i) alternative is `text.charAt(i)`.

```tessera
import std.io

def countLines(path: String): Long {
    val mapping = mapFile(path);
    val view = mappedBytes(mapping);
    var lines: Long = 0;
    var i: Long = 0;
    while (i < mappedSize(mapping)) {
        if (view[i] == '\n') { lines = lines + 1; }
        i = i + 1;
    }
    unmapFile(mapping);
    return lines;
}
```

`examples/parallel_wc.csec` splits this loop across OS threads (`std.thread`). Build it with
`csec++ -O3 -march=native --emit-exe examples/parallel_wc.csec -o parallel_wc.exe`.

## Templates

Function and class templates use C++-style `template<...>` declarations with
type parameters and integer-like non-type parameters:

```ts
template<typename T>
def identity(value: T): T = value

template<typename T, Int N>
def capacity(): Int = N

template<typename T>
class Box(value: T) {
    def get(): T = value
}

def main(): Int {
    val x = identity<Int>(42);
    val n = capacity<Int, 8>();
    val box = new Box<Int>(x);
    return box.get() + n;
}
```

Template class constructor parameters are stored as fields, and class fields can
use those constructor parameters in their initializers:

```ts
template<typename T>
class Holder(initial: T) {
    var current: T = initial;
    def get(): T = current
}
```

Generic library-shaped types such as `Vector[Int]` and `Matrix[Double]` can be
used in signatures as opaque reference types until a concrete runtime
implementation is provided.

## Control Flow

Conditional statements:

```ts
if (value > 0) {
    return 1;
}
return 0;
```

Loops:

```ts
for (x <- xs) {
    print(x);
}

var i: Int = 0;
while (i < 5) {
    i = i + 1;
}
```

## Collections And Ranges

Array literals and ranges are used by collection transforms. A range is written
with `to` (inclusive) or `until` (exclusive) and appears in `for` comprehensions
and collection headers:

```ts
val xs = [1, 2, 3, 4];
for (i <- 0 to 4)     { print(i); }   // 0,1,2,3,4
for (i <- 0 until 4)  { print(i); }   // 0,1,2,3
```

Mapping, filtering, and reduction syntax:

```ts
val squares = map(x <- xs) {
    x * x;
};

val even = filter(x <- xs) {
    x % 2 == 0;
};

val total = reduce(acc, x <- xs, 0) {
    acc + x;
};
```

Parallel map and policy-based reduction:

```ts
val ys = pmap(openmp, x <- xs) {
    x * x;
};

val sum = preduce(simd, acc, x <- xs, 0) {
    acc + x;
};
```

Current policy names include `cpu`, `simd`, `openmp`, and `gpu`. Some policies
are parsed before full backend lowering is implemented.

## Dictionaries

A dictionary literal is written as in Python, `{key: value, ...}`, and has the type
`Dict<K, V>`. `K` and `V` are inferred from the entries: every key must have one type and
every value must have one type (`Int` and `Long` values widen to `Long`, integers and
floating-point values to the floating-point type, lists of one element type to `Vector[T]`).

```ts
val ages = {"alice": 30, "bob": 25};                  // Dict<String, Int>
val scores = {"math": [90, 85, 77], "art": [60, 70]}; // Dict<String, Vector[Int]>
val points = {"p": {"x": 1.5, "y": 2.0}};             // Dict<String, Dict<String, Float>>
var counts: Dict<String, Long> = {};                  // an empty dictionary needs a type

ages["carol"] = 41;         // insert or replace
ages["alice"] += 1;         // read-modify-write
val bob = ages["bob"];      // a missing key stops the program with "KeyError: 'key'"
val zed = ages.get("zed", 0);
if ("bob" in ages) { ages.remove("bob"); }
println(ages);              // {'alice': 31, 'carol': 41}

for (name <- ages) { print(name); }              // keys, in insertion order
for (name, age <- ages) { println(name + age); } // keys and values
```

Keys are `Int`, `Long`, `Char`, `Bool`, or `String` (compared by content). Values may be
any scalar, `String`, list, nested dictionary, or class instance. Methods: `size()`
(also `length()`/`count()`), `isEmpty()`, `contains(key)` (also `key in d`, `key notin d`),
`get(key)`, `get(key, default)`, `remove(key)`, `keyAt(i)`, `valueAt(i)`, `copy()`,
`clear()`, `keys()` (a `Set`), and `toString()`. Dictionaries are reference values: `val e = d`
and passing `d` to a function share the same dictionary (use `copy()` for an independent one).

## Sets

A set literal is written as in Python, `{a, b, c}`, and has the type `Set<T>`. A one-element set
needs a trailing comma, `{x,}` (`{x}` is a block whose value is `x`), and an empty set needs a type,
because `{}` alone is an empty dictionary:

```ts
val primes = {2, 3, 5, 7};              // Set<Int>
val odds = {1, 3, 5, 7, 9};
var seen: Set<String> = {};

println(primes | odds);                 // union         {2, 3, 5, 7, 1, 9}
println(primes & odds);                 // intersection  {3, 5, 7}
println(primes - odds);                 // difference    {2}
println(primes ^ odds);                 // symmetric difference {2, 1, 9}
val small = {3, 5} <= primes;           // subset (also >=, <, >, ==, !=)
if (4 notin primes) { primes.add(4); }  // add/remove return whether the set changed
for (p <- primes) { print(p); }         // insertion order
```

The keyword forms `union`, `intersect`, `diff`/`without`, `symdiff`, `subset`, `superset`,
`properSubset`, and `properSuperset` work on sets too. Elements are `Int`, `Long`, `Char`, `Bool`,
or `String`. Methods: `add`, `remove`/`discard`, `contains`, `size`, `isEmpty`, `clear`, `copy`,
`elementAt(i)`, `union`, `intersection`, `difference`, `symmetricDifference`, `isSubset`,
`isSuperset`, `isDisjoint`, `toString`.

## Tensors

Tensor types carry an element type and a compile-time shape, `Tensor<Elem, D1,
D2, …>`. Construct one with `new`, giving a fill value; read and write elements
with a comma-separated index list, which also serves as an l-value:

```ts
val m: Tensor<Double, 3, 3> = new Tensor<Double, 3, 3>(0.0);  // 3x3 of zeros
m[0, 0] = 2.0;                 // element write (l-value)
val trace = m[0, 0] + m[1, 1] + m[2, 2];
```

Whole-tensor arithmetic is component-wise, with scalar broadcast, and the
product operators bind tighter than `+`/`-`:

```ts
val a: Tensor<Int, 2, 2> = new Tensor<Int, 2, 2>(1);
val b: Tensor<Int, 2, 2> = new Tensor<Int, 2, 2>(2);
val s   = a + b;        // component-wise add
val sc  = a * 3;        // scalar broadcast
val mm  = a @ b;        // matrix multiply
val dot = a inner b;    // inner (dot) product; also `outer`, `tensor` (Kronecker)
```

Rank-1 tensors expose named components `x`/`y`/`z`/`w` (elements 0–3), so
`v.x`, `v.y = …` work alongside `v[i]`. Fortran-style sections assign a scalar
or matching tensor into a slice, e.g. `row[0:3] = 1.0`.

```ts
// One layer of a feed-forward network: h = ReLU(x @ W + b)
def layer(x: Tensor<Double, 1, 3>, w: Tensor<Double, 3, 4>,
          b: Tensor<Double, 1, 4>): Tensor<Double, 1, 4> {
    return (x @ w) + b;   // then apply an activation element-wise
}
```

A complete two-layer network is in
[examples/feedforward_nn.csec](../examples/feedforward_nn.csec).

## Math And Set Helpers

Scalar math helpers include:

```ts
abs(x);
sign(x);
floor(x);
ceil(x);
round(x);
lcm(a, b);
clamp(x, low, high);
between(x, low, high);
approxEq(a, b, eps);
```

Set helpers include:

```ts
emptySet();
singleton(x);
cardinality(s);
disjoint(a, b);
complement(a, universe);
```

## Native Dynamic Library Calls

Runtime dynamic loading uses handles and symbol pointers represented as `Long`:

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

Integer-like calls use `callNative0`, `callNative1`, `callNative2`, and
`callNative3`. Floating-point calls use `callNativeDouble0`,
`callNativeDouble1`, and `callNativeDouble2`.

## Process Arguments

Compiled programs can read their command-line arguments:

```ts
val argc: Int = commandLineArgCount();
val input: String = commandLineArg(1);
```

`commandLineArg(0)` is the executable path when the platform provides it.
Out-of-range indexes return an empty string.

The local name `nativeCos` is intentionally different from the imported C symbol
`"cos"`. A Tessera call named `cos(...)` is already a built-in scalar math
call, so use a different local name when importing the C library symbol directly.

## LaTeX-Inspired Math

The parser includes experimental inline and block LaTeX-inspired math forms for
matrix, summation, product, set, and function-style notation. See the fixtures in
`tests/positive/07_math_latex` for currently supported examples.

## Notes

- Statements generally end with `;`.
- Blocks use `{ ... }`.
- Ownership is strict by default. Compile with `--no-strict-ownership` to fall
  back to box-only move-checking for code that has not been migrated;
  `--strict-ownership` forces it on explicitly.
- For the full ownership/borrow/move/destruction model and its milestone status
  see [memory-model-design.md](memory-model-design.md); runnable examples are in
  `examples/strict_ownership_demo.csec` and
  `examples/strict_ownership_errors.csec`.
- The language and runtime are evolving; tests under `tests/positive`,
  `tests/semantic_positive`, `tests/negative`, and `tests/native` are the best
  reference for exact accepted syntax.
