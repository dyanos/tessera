# csec Memory-Management Model — Design & Implementation Plan

Status legend: **[impl]** already implemented in this compiler · **[partial]** partially present ·
**[plan]** designed here, not yet built.

This document specifies an ownership / memory-management subsystem that is *substantially simpler
than Rust* while preventing the common memory-safety errors, without a tracing GC. It is grounded
in what the csec compiler already has: an owned wrapper `box T`, a move operator `<-`, borrow
operators `&` / `&mut`, an `OwnershipState` dataflow machine in the type checker, RAII cleanup
insertion, and an explicit `free`. The design generalizes from that foundation.

The layers are kept strictly separate: **source semantics → static analysis → IR → runtime →
backend ABI → MVP restrictions → future extensions.**

---

## 1. Executive recommendation

**Viable — yes, as a conservative affine-move model with lexical borrows.** The realistic safety
guarantees for the MVP:

- **No use-after-move** of owned values (compile-time). **[impl for `box`]**
- **No double free / no leak** of owned values on any control-flow path, via automatic
  scope-exit destruction. **[impl for `box` via RAII cleanup]**
- **No dangling borrow**: borrows are lexically scoped and may not escape (return, global, field,
  escaping closure, thread, async). **[partial]**
- **No aliasing mutation hazard**: at most one `&mut`, or many `&`, never both. **[impl]**
- **Deterministic destruction order** (reverse declaration order). **[impl]**

What it deliberately does **not** guarantee (accepted trade-offs for simplicity): general
lifetime-parameterized APIs, self-referential structs, returning borrows, borrow fields in ordinary
structs, and data-race freedom for shared mutable globals (those require explicit synchronization).

The key simplification vs Rust: **there are no user-visible lifetime parameters.** Every case where
Rust would demand a lifetime is instead handled by a *restriction* ("a borrow may not escape") plus
an escape hatch (`clone`, `Shared<T>`, or restructuring). This trades expressiveness for a much
lower learning cost, which is the stated goal.

---

## 2. Source-language semantics

### 2.1 Value categories

| Category | Examples | Assignment | Passed to fn | Destroyed |
|---|---|---|---|---|
| **Copy** | `Int Long Float Double Bool Char Byte Short`, small `@copy` structs | implicit copy | by value (copy) | trivially (no dtor) |
| **Owned (non-copy)** | `box T`, heap classes, arrays/vectors, `String`, `Shared<T>` | **move** | move (consume) or `&`/`&mut` (borrow) | `box`/Tensor/Shared/Weak: scope-exit drop; classes/arrays: explicit `free` today |
| **Borrow** | `&T`, `&mut T` | copy of the reference (still lexically bound) | by value | nothing (non-owning) |

The compiler classifies a type as Copy iff it is a primitive scalar or a struct explicitly marked
`@copy` all of whose fields are Copy. Everything else is Owned. **[partial: today only `box T` is
move-checked; §12 "Future" extends this to all Owned types.]**

### 2.2 Creation

`new T(...)`, array/vector literals, and `box` wrapping create a fresh **Owned** value in state
`Owned`. A local binding becomes that value's unique owner.

### 2.3 Move (recommended: **Option B, explicit `<-` / `move`**)

The spec asks us to choose between implicit move (`b = a` invalidates `a`) and explicit move
(`b = move a`). **Recommendation: explicit move**, which csec already uses via the `<-` operator.

| Criterion | Implicit move (A) | Explicit move (B) — **recommended** |
|---|---|---|
| Learning | "`=` sometimes destroys the RHS" is a hidden, surprising rule | one visible keyword means "this is consumed" |
| Readability | move sites invisible at the use site | move sites are grep-able and local |
| Error prevention | easy to accidentally move; refactors silently change meaning | accidental moves are rejected; intent is explicit |
| Compiler complexity | must infer move-vs-copy everywhere `=` appears | move only where `<-`/`move` appears; simpler dataflow |
| Refactoring safety | moving code can flip a copy into a move | explicit annotation survives refactor |

Semantics: `b <- a` transitions `a` from `Owned` to `Moved` and `b` to `Owned`. Any later use of `a`
is a compile error until `a` is reassigned. Ordinary `b = a` on an Owned value is an **error** that
suggests `<-` (move), `clone(a)`, or `&a` (borrow) — this is exactly today's
`ownership_transfer_requires_move` diagnostic. **[impl]**

### 2.4 Copy and Clone

- Copy values duplicate implicitly on `=` and on argument passing. **[impl]**
- Owned values duplicate only via **explicit** `clone(a)`, which produces a new independent Owned
  value and leaves `a` in state `Owned` (unmoved). **[plan — implemented alongside this doc, §"clone"]**

### 2.5 Borrow (lexical, non-escaping)

- `&a` produces a shared borrow `&T`; increments `a.immutableBorrows`. **[impl]**
- `&mut a` produces a mutable borrow `&mut T`; sets `a.mutableBorrowed`, and requires `a` to be
  declared `var`. **[impl]**
- A borrow is valid only within the lexical region in which it is created and may not escape (§6).
  Borrows are released at the end of the statement / call in the MVP (statement-scoped borrows),
  which is the simplest sound rule. **[impl: released after the call/statement]**
- While `a` has any `&`/`&mut`, `a` may not be moved (`ownership … while it is borrowed`). **[impl]**
- Aliasing: many `&` **xor** one `&mut`. Two `&mut`, or `&mut` while `&` exists, is an error. **[impl]**

### 2.6 Return transfers ownership

`return x` moves `x` to the caller (`x` becomes `Moved` in the callee, so its dtor is *not* run in
the callee). The caller's binding becomes the owner. Returning a borrow of a local is rejected
(§6). **[impl for the single-value case]**

### 2.7 Multiple return

`return a, b, c` moves all of `a,b,c` to the caller as an anonymous owned tuple; the caller
destructures with `x, y, z = f()`. Full semantics in §4. **[plan]**

### 2.8 Field / global assignment

- `parent.child <- child` moves `child` into the field; the container's dtor later destroys it;
  `child` is `Moved` afterward. An existing field value is destroyed first. **[plan; RAII exists]**
- `global <- config` moves into global storage; the previous global value is destroyed first;
  globals are destroyed at shutdown in reverse init order. Mutable owned globals require explicit
  `Shared`/synchronization (§8). **[plan]**

### 2.9 Destruction

At scope exit — on **every** path (normal, `return`, early return, `break`, `continue`, unwind,
panic, match-arm exit) — still-owned `box`, Tensor, Shared, and Weak bindings are destroyed in
reverse declaration order. Heap classes and arrays/vectors currently require explicit `free` while
their ownership-aware drop glue is under construction. `Moved`, `Copy`, and `Destroyed` bindings
are skipped.

Tensor temporaries are owned too. A fresh tensor result (`x @ w`, element-wise ops, `transpose`,
`relu`, `softmax`, slices, `new Tensor`) that is only borrowed by another tensor operation lives in a
compiler-owned slot and is dropped at the end of its full expression (statement or loop condition),
or earlier on `return`/`break`/`continue`. A fresh result bound with `val` is owned by that binding
when every later use only borrows it.
**[impl, direct compiler]**

---

## 3. Type-system design (source syntax)

```
box T            // owned<T>: unique heap ownership, move-checked, auto-destroyed   [impl]
&T               // borrowed<T>: shared, lexical, non-escaping                       [impl]
&mut T           // mutable borrow: exclusive, lexical, non-escaping                 [impl]
Shared<T>        // explicit strong reference-counted ownership                      [plan]
Weak<T>          // non-owning weak handle into a Shared<T>                          [plan]
@copy struct S   // opt into implicit copy (all fields must be Copy)                 [plan]
(A, B, C)        // owned tuple for multiple return                                  [plan]

x <- y           // move y into x                                                    [impl]
&x  /  &mut x    // borrow                                                           [impl]
clone(x)         // explicit deep copy of an Owned value                             [plan→impl]
free(x)          // explicit heap release (counterpart to new)                       [impl]
```

Field ownership categories (§7): `owned` (default for `box`/heap fields), `shared` (`Shared<T>`),
`weak` (`Weak<T>`). **Borrowed fields are prohibited in the MVP** — they are the feature that forces
lifetime parameters, so they are deferred.

---

## 4. Multiple-return specification

### 4.1 Semantic model (backend-independent)

`return e1, …, en` evaluates each `ei` left-to-right, moving each Owned `ei` (copying each Copy one)
into result slot `i`. The result is a value of anonymous owned tuple type `(T1, …, Tn)`. Destructuring
`p1, …, pn = f()` binds each `pi` to result slot `i`, transferring ownership. This model is
independent of the physical ABI (§4.4).

**Duplication rule:** `return buffer, buffer` is rejected — the second use is a use-after-move. Legal
forms: `return buffer, clone(buffer)` or `return s, s.clone()` where `s: Shared<T>`.

### 4.2 The seven-step destructuring protocol (transactional)

For any `p1, …, pn = <rhs>`:

1. **Evaluate** all RHS expressions (fully, left-to-right).
2. **Materialize** every returned value into a compiler temporary `t1..tn`.
3. **Validate destination aliasing** (reject `a, a = f()`; detect overlapping `object.b` targets).
4. **Destroy old destination values** that will be overwritten (existing owned contents of `pi`).
5. **Move** each `ti` into `pi` (or into `_`-sink temporaries).
6. **Destroy** any `_`-ignored `ti` at the end of the operation.
7. **Ordering** is defined left-to-right; steps 1–2 happen entirely before any 4–5, which makes the
   whole assignment **transactional**.

Because RHS values are fully materialized (steps 1–2) before any destination is touched (steps 4–5),
`a, b = b, a` is a correct swap, and overlapping destinations never read a half-updated state.

### 4.3 Edge-case table (all required cases)

| Source | Rule |
|---|---|
| `a, b = f()` | bind both, move each slot |
| `a, _ = f()` | bind `a`; slot 1 materialized then destroyed at end of op |
| `_, _ = f()` | both slots materialized then destroyed (call still runs for effects) |
| `return f()` | tuple result of `f` forwarded as this function's tuple result (no re-wrap) |
| `return f(), g()` | flatten: results are `(…f, …g)`; each Owned moved once |
| `x = f(); a, b = x` | `x` owns the tuple; second line moves fields out of `x`, then `x` is `Moved` |
| `a, b = cond ? f() : g()` | both arms must yield the same tuple type; branch merge (§6 join) |
| `a, b = try f()` | on success bind; on error, materialized slots are destroyed on the unwind path |
| `a, b = await f()` | **rejected in MVP** unless results are Owned/`Shared` (no borrow across await) |
| `global_a, global_b = f()` | step 4 destroys prior globals; step 5 moves into global storage |
| `object.a, object.b = f()` | field targets; step 4 destroys prior field contents |
| `a, object.b = f()` | mixed local + field target; same protocol |
| `a, a = f()` | **error**: double assignment to one destructuring destination |
| `a, b = b, a` | legal swap (transactional, §4.2) |
| `return value, &value` | **error**: returns owned `value` *and* a borrow of it → borrow escape |
| `return owner, owner.child_reference` | **error** if `child_reference` is a borrow of moved `owner`; OK if it is `Shared`/`Weak` |
| `return shared, shared` | **error** unless `shared: Shared<T>` (then it is two ref-count bumps) |
| `return move(a), move(b)` | two independent moves, legal |
| `return clone(a), a` | legal: slot 0 is a fresh copy, slot 1 moves `a` |

### 4.4 Backend lowering — compare A/B/C

- **A. Tuple return.** Lower `return a,b,c` to a real owned aggregate. Clean semantics, reuses the
  aggregate/destructor machinery, but forces materialization.
- **B. Hidden out-params.** `f(out0*, out1*, out2*)`; caller provides slots. Great for large values,
  but initialization safety is fiddly: if construction of `out1` panics after `out0` was written,
  the partially-filled slots must be destroyed — the compiler must emit per-slot "initialized" flags
  and unwind cleanup.
- **C. Aggregate + RVO/scalar-replacement.** Return a compiler aggregate; let the backend pick
  registers vs `sret`. Best of both.

**Recommendation: semantic model = A (owned tuple); backend = C.** Emit an LLVM aggregate; small
tuples return in registers, large ones via `sret` (a caller-provided hidden pointer) — exactly what
LLVM already does for `{T1,T2,T3}` returns. Scalar-replacement/RVO then elides the temporary when the
callee builds results directly into the return slot. Source semantics never depend on which path the
backend chose.

---

## 5. Static-analysis rules — the ownership state machine

Abstract state tracked **per binding** (this is the existing `OwnershipState`, generalized):

```
Uninitialized  → no value yet
Owned          → owns a live value (must be destroyed unless moved)
BorrowedShared(n) → owns value, currently lent out as n immutable borrows
BorrowedMutable   → owns value, currently lent out as one mutable borrow
Moved          → ownership transferred away (skip destruction)
PartiallyMoved → aggregate with some fields Moved (deferred; §12)
Destroyed      → dtor already run (skip)
MaybeInitialized → init on some paths only (CFG join); use requires proof or is an error
```

Valid transitions:

```
Uninitialized --(bind/new)--> Owned
Owned --(&x)--> BorrowedShared(1) --(&x)--> BorrowedShared(n+1) --(end borrow)--> Owned
Owned --(&mut x)--> BorrowedMutable --(end borrow)--> Owned
Owned --(x <- .. / return x / field<-x)--> Moved
Owned --(scope exit)--> Destroyed
Moved --(rebind x = new)--> Owned         (reassignment revives the slot)
BorrowedShared/BorrowedMutable --(move or 2nd &mut)--> ERROR
Moved/Destroyed --(use)--> ERROR (use-after-move / use-after-destroy)
MaybeInitialized --(use)--> ERROR unless all predecessors are Owned
```

CFG join rule: at a control-flow merge, per binding take the *meet*: `Owned ⊓ Moved = Moved` (a value
moved on one path is treated as moved), `Owned ⊓ Uninitialized = MaybeInitialized`. This makes the
checker conservative and never lets a value be used or double-destroyed after a conditional move.

---

## 6. Compiler architecture

Frontend already present in csec: lexer → parser → AST → `TypeChecker` (visitor) → LLVM codegen
(direct) / `.inc` self-host emitter. The ownership subsystem adds an analysis IR and passes:

```
AST
 └─ Name resolution                         [impl]
 └─ Type checking + Copy/Move classification [impl / partial]
 └─ Lower to Ownership-CFG (OCFG)            [plan]   ← per-function CFG of ownership ops
      nodes: Def, Move, Borrow(s/m), EndBorrow, Use, Call(consume/borrow), Return,
             FieldStore, GlobalStore, Drop, Branch, Merge
 └─ Dataflow passes over OCFG:
      1 Definite-initialization             [plan]
      2 Use-after-move                       [impl (linear approx) → OCFG]
      3 Escape analysis (borrows)            [partial]
      4 Borrow validation / alias checking   [impl]
      5 Closure/async capture analysis       [partial]
      6 Partial-init / partial-move          [plan / deferred]
 └─ Drop elaboration (insert dtor calls per path) [impl for box → generalize]
      tensor temporaries: per-statement owned slots   [impl]
 └─ Multiple-return lowering                 [plan]
 └─ ABI lowering (sret/registers)            [impl in backend]
 └─ Diagnostics                              [impl / extend]
```

Today the type checker runs a **linear, statement-ordered approximation** of passes 2/4 (which is
sound for straight-line code and the common cases). The plan promotes it to a real CFG dataflow so
that conditional moves and loops are handled precisely.

---

## 7. Dataflow algorithms (pseudocode)

```text
# Use-after-move (forward, may-analysis; ERROR on use of Moved)
state[entry] = { p: Owned for each owned param ; Uninitialized for locals }
worklist = [entry]
while worklist:
    n = pop(); in = meet(state[p] for p in preds(n))     # meet: Moved dominates
    out = transfer(n, in):
        Def x        : out[x] = Owned
        Move x       : require in[x] == Owned            else ERROR(use-after-move/moved-while-borrowed)
                       out[x] = Moved
        Borrow x (s) : require in[x] in {Owned,BorrowedShared} else ERROR ; out[x]=BorrowedShared(+1)
        Borrow x (m) : require in[x] == Owned            else ERROR(alias) ; out[x]=BorrowedMutable
        EndBorrow x  : decrement / clear
        Use x        : require in[x] not in {Moved,Destroyed,MaybeInit} else ERROR
    if out != state[n]: state[n]=out; push(succs(n))

# Definite initialization: same lattice; a Use of Uninitialized/MaybeInitialized is ERROR.

# Escape analysis (borrows): mark a borrow value b=&x with region = current lexical scope of x.
# On Return(v)/GlobalStore(v)/FieldStore(longlived,v)/CaptureByEscaping(v):
#   if v is a Borrow whose region ⊂ (does not outlive) the destination → ERROR(borrow escapes).

# Borrow validation / alias: enforce  (#mut==0 && #shared>=0) || (#mut==1 && #shared==0)  per x.

# Drop insertion (per basic block, reverse decl order):
for each exit edge e of block B (fallthrough, return, break, continue, unwind):
    for x in reverse(decls_live_at(B)):
        if state_on(e)[x] == Owned: emit Drop(x)     # not Moved/Destroyed/Copy

# Multiple-return temporaries: see §4.2 seven-step protocol; each _ sink and each overwritten
# destination gets an explicit Drop; RHS temporaries dropped after step 5.

# Cleanup on exceptional paths: unwind edges are ordinary CFG edges; drop-insertion runs on them
# too, so a panic between constructing out0 and out1 drops out0 (and any prior locals) exactly once.
```

---

## 8. Runtime support vs compile-time

**Fully compile-time** (no runtime cost): move checking, borrow/alias checking, escape analysis,
definite-init, drop *placement*. These vanish after type-checking.

**Runtime support required:**
- Destructors: emitted `Drop` calls invoke type dtors / `free` (already: box → `free`). **[impl]**
- `Shared<T>`: a heap control block `{ strong:i64, weak:i64, value:T }`; `.clone()` bumps `strong`,
  drop decrements and frees `value` at 0 and the block at 0/0. Non-atomic by default; `SharedSync<T>`
  uses atomic counters. Cycles leak unless broken with `Weak<T>` (weak does not keep `value` alive).
  **[plan]**
- Globals: a deterministic init list and a reverse-order shutdown dtor list. **[plan]**
- Unwind/panic: a personality-driven cleanup path that runs the same emitted `Drop`s. csec today
  uses scope cleanup lists (`registerCleanup`/`emitAllCleanups`); the panic path reuses them. **[partial]**

---

## 9. Examples

### 9.1 Twenty valid programs (expected: compile & run)

1. `var b: box Image = new Image()` then scope-exit auto-drop. **[impl]**
2. `t <- s` move between two `box` locals; `t` owns, `s` moved. **[impl]**
3. `take(<- img)` consume into a `box`-consuming parameter. **[impl]**
4. `observe(&count)` shared borrow of a Copy value. **[impl]**
5. `update(&mut count)` mutable borrow of a `var`. **[impl]**
6. `var c = clone(a)` — two independent owned values. **[impl w/ this change]**
7. `free(arr)` after last use of `new Int[n]`. **[impl]**
8. `return buffer` — single owned return. **[impl]**
9. `return a, b, c` — multiple owned return (tuple). **[impl]**
10. `a, b = f()` destructure two owned results. **[impl]**
11. `a, _ = f()` — ignored slot. **[impl]**
12. `a, b = b, a` — transactional swap. **[plan]**
13. `return clone(a), a` — clone one, move the other. **[plan]**
14. `s = Shared(v); u = s.clone()` — two strong owners. **[impl]**
15. `parent.child <- child` — move into a field. **[plan]**
16. `if cond { x <- y }` then never use `y` afterward — conditional move OK. **[plan/CFG]**
17. borrow inside a loop body, released each iteration. **[impl]**
18. `for (x <- xs) { use(x) }` iteration with borrowed element. **[impl]**
19. `map/filter/reduce` producing a fresh owned collection. **[impl]**
20. return from an early-return branch drops the other still-owned locals. **[impl]**

### 9.2 Twenty invalid programs (expected: compile-time error)

1. use `img` after `take(<- img)` → *use of moved value 'img'*. **[impl]**
2. `t = s` on `box` without `<-` → *ownership transfer requires '<-'*. **[impl]**
3. `&mut img` while `&img` live → *cannot mutably borrow while immutable borrow exists*. **[impl]**
4. `&mut x` on an immutable `val` → *cannot mutably borrow immutable value*. **[impl]**
5. move `img` while borrowed → *cannot move while it is borrowed*. **[impl]**
6. `return &value` (local) → *returns a reference to local 'value'*. **[plan]**
7. `return buffer, buffer` → *value 'buffer' returned twice (use clone/Shared)*. **[plan]**
8. `a, a = f()` → *destination 'a' assigned twice*. **[plan]**
9. `return value, &value` → *borrow of 'value' escapes with its owner*. **[plan]**
10. store `&local` into a global → *borrow stored in longer-lived location*. **[plan]**
11. capture `&local` in an escaping closure → *borrow captured by escaping closure*. **[plan]**
12. `async move { use(buffer) }` then use `buffer` → *use of moved value*. **[plan]**
13. `borrow across await` → *borrow may not cross 'await'*. **[plan]**
14. two `&mut a` simultaneously → *cannot create a second mutable borrow*. **[impl]**
15. use uninitialized `var x: box T` → *use of possibly-uninitialized value*. **[plan]**
16. use after conditional move (moved on one branch) → *'x' may be moved here*. **[plan/CFG]**
17. partial move `return user.data` (MVP) → *moving a field out is not supported; return the whole value or clone*. **[plan]**
18. double free: `free(p); free(p)` → *use of freed value 'p'*. **[plan]**
19. return a borrow field alongside its owner (`return owner, owner.ref`) → *borrow outlives owner*. **[plan]**
20. `Shared` cycle with no `Weak` → *lint: reference cycle will leak; break with Weak*. **[plan]**

---

## 10. MVP implementation plan (incremental, Codex-ready)

Repository already has: `src/*.cpp` (direct compiler), `src/native_runtime/*.inc` (self-host
emitter), `tests/{positive,negative,semantic_positive,semantic_negative}`, `docs/`.

Milestones (each ends green on all suites + a byte-identical self-host fixed point):

- **M0 — baseline (done).** `box T`, `<-`, `&`/`&mut`, use-after-move, borrow/alias checks, RAII
  drop for box, `free`, ownership semantic tests. **[impl]**
- **M1 — `clone` (done). [impl]** `clone(x)` builtin: type = type of `x` (fresh owned), leaves `x`
  unmoved; codegen mallocs + memcpys the boxed payload. Test `semantic_positive/ownership_clone`.
- **M2 — Copy/Move classification pass. [impl, default on]** Strict ownership is now the **default**
  (branch `memory-model-m2-strict-and-async`): `=`/argument passing of any non-copy Owned type
  requires `<-`/`&`/`&mut`. `isCopyType` classifies Copy vs Owned — primitives, `String` (immutable/
  shared), references, function values and value-`struct`s are Copy; reference `class`es, arrays,
  `box`, and owned container/smart-pointer generics are Owned. Two rules keep it ergonomic: a
  temporary (literal, `new X()`, call result) is an rvalue and moves implicitly (only *named* owned
  locals need `<-`), and member access auto-derefs borrows (`ref.method()`, `ref.field`, `ref[i]`,
  and `map`/`for` over `&Vector[T]` all behave like the borrowed value — see `stripBorrow`). All 270
  positive fixtures were migrated (use-once args take `<-`, reused values take `&`) and compile+run
  under the strict default; the self-host source also compiles clean under it. `--no-strict-ownership`
  opts out (box-only) for legacy code. See `type_checker.cpp`, `main.cpp`, and the borrow auto-deref
  in `MethodCallNode`/`AccessFieldNode`/`ArrayAccessNode`.
- **M3 — Ownership-CFG + dataflow framework. [partial]** The high-value precision gaps are closed:
  move-checking is now **path-sensitive across diverging `if`-branches and `while` bodies**. Both `if`
  arms are analysed from the same pre-branch ownership snapshot and only the fall-through paths are
  joined; a `while` body that always returns has its moves discarded (it runs at most once and
  diverges, so the loop only falls through when the body never executed). A move on a branch/body that
  always returns no longer marks the value moved for the following code (`statementDiverges` +
  `mergeMovedFrom` + `visit(IfStatementNode)`/`visit(WhileStatementNode)` in `type_checker.cpp`; tests
  `strict_ownership/positive/{04,05}`, `negative/{03,04}`). Soundness is preserved (a move on a
  fall-through branch/body used afterward, and straight-line use-after-move, are still rejected). A
  full OCFG + worklist solver (definite-init as a lattice, multi-iteration loop re-move detection)
  remains a larger refactor, but the checker was already *sound* — this is precision only.
- **M4 — Drop elaboration generalized. [partial]** Per-path drop insertion now fires on **every
  return path**, not just the first: `emitAllCleanupsExcept` no longer clears the cleanup registry
  after emitting, so `if (c) { return 1; } return 0;` frees its owned locals on both the `then` and
  the fall-through paths (reverse order preserved). Previously the first return consumed the registry
  and every later path leaked (`codegen.cpp`; test `positive/048_box_early_return_drop`). Remaining:
  drops for non-box owned locals (only reachable in strict mode, and needs per-type destructors) and
  unwind-edge drops (moot until the language has exceptions).
  *Tensor temporaries [impl, direct compiler]:* each tensor result produced by `allocateTensor` is
  marked fresh (instruction metadata, cleared when the value is bound to a name and stripped before
  verification). A borrowing tensor operation (`@`, `inner`/`outer`, element-wise ops, `transpose`/
  `adjoint`/`conjugate`/`relu`/`softmax`/`sum`/`mean`/`norm`/`mse`, axis reductions, `argmax`/`argmin`,
  `*Like`, element reads and slices) moves each fresh operand into an owned slot of the current
  statement's frame (`CodeGenerator::dropTemporary`). The slot is an entry-block `ptr` alloca set to
  null; null is the drop flag, so a temporary made on only one side of a join (`and`/`or` right-hand
  side, a branch) is still dropped exactly once, through `csec.tensor.drop`, which ignores null.
  Drops run at statement end (`BlockNode`), on `return` (`emitAllCleanupsExcept`), and on
  `break`/`continue` for frames inside the loop (`emitCleanupsAbove`); a slot written again by a
  re-evaluated loop condition drops its previous value first. `val y = <fresh tensor>` binds `y`
  directly and registers the existing `TensorFree` cleanup only when `BlockNode` proves every later
  use of `y` borrows it (`onlyBorrowingTensorUses`): an operand of a binary operator or a tensor
  builtin, the base of an index/slice, a `.x/.y/.z/.w` component, or `return y`. Any other use (an
  alias `var q = y`, an assignment, a field store, a user-function argument, a lambda capture, the
  block's result value) leaves `y` unowned, because the backend has no escape analysis yet. A store
  through `=` also clears the fresh mark, so an assignment expression's value is never dropped as a
  temporary. Not covered: tensor results of user function calls (the
  callee may return an alias of a parameter), fresh tensors passed to user functions (treated as
  moved into the callee, which does not drop parameters), `var` tensors, expression-bodied lambdas,
  and the self-host emitter. Tests `positive/11_tensor/122_tensor_temporary_drops`,
  `123_tensor_val_escape`.
- **M5 — Multiple return (done). [impl]** Parser `return a,b` / `a,b = f()` / `(A,B,C)` return type;
  `TupleType` → LLVM aggregate (sret/registers); `TupleExpressionNode`/`DestructuringAssignmentNode`;
  seven-step protocol; ownership transfer of box elements; `_` ignore; duplicate-move and
  duplicate-destination rejection. Tests 192/193 + two semantic negatives.
- **M6 — Escape analysis for borrows. [partial]** Return case done: `return &x` and a borrow element
  in a returned tuple (`return value, &value`) are rejected with beginner diagnostics. The
  global/field/escaping-closure cases need the OCFG (M3) and remain **[plan]**.
- **M7 — `Shared<T>` + `Weak<T>` (done). [impl]** Strong reference counting: `Shared(x)`
  allocates a control block `{ i64 strong, payload }` (strong=1); `.clone()` retains; `.get()` reads
  the payload; each owning binding releases (decrements) at scope exit and frees the block at zero.
  Refcount ops are inline IR (no runtime library needed); the cleanup system gained a release-vs-free
  kind. Tests 194 (clone) and 195 (multi-clone, strong=3). Weak<T> is a non-owning handle: `Weak(s)` bumps a separate weak count, `w.get()` returns the
  payload only while some strong owner is alive (else a zero value), and the block is freed only when
  strong AND weak both reach zero -- so a cycle with a Weak edge is reclaimed. Tests 194/195 (Shared),
  196 (Weak). Only the automatic cycle *lint* remains [plan]; duplicate a Shared with `.clone()`.
- **M8 — Globals & closures/async capture modes. [impl]** Two parts:
  - *Globals:* a top-level `var` lowers to an LLVM global (`InternalLinkage`) read/written across
    functions (`VariableDeclarationNode.cpp` module-scope path + `symbol_table.cpp` two-phase
    refresh). Borrow escape *into* a global is structurally blocked (`slot = &local` → "Invalid
    assignment target"), like the field case — no soundness hole. Owned-wrapper globals give a clean
    diagnostic rather than the earlier hang.
  - *async/await (branch `memory-model-m2-strict-and-async`):* adds `async` (function modifier) and
    `await` (prefix expression) keywords. `await` is valid only inside an `async def`, and the M8
    **no-borrow-across-await** rule is enforced: any `&`/`&mut` still live in scope at an await point
    is rejected ("borrow of 'x' is held across an 'await'"), since after suspension another task could
    mutate or drop the borrowed value. `await` lowers to its operand value (sequential evaluation)
    until a coroutine runtime exists — the surface syntax and the borrow rule are what M8 specifies.
    See `type_checker.cpp` (`inAsyncFunction` + the await check), `Expression.cpp`, `PrefixExpressionNode.cpp`;
    tests `tests/async` + `run_async.ps1`.

Deferred beyond MVP: partial moves (M9), borrow fields with proven lifetimes, region allocation
(§ below), general lifetime parameters (likely never for v1 line).

**Current status (implemented & verified on the direct compiler):** M0–M8 all implemented. M0/M1/M5/
M7 fully; M2 strict ownership **on by default** (whole corpus migrated); M3 path-sensitivity for
diverging `if`/`while`; M4 per-path drop on every return path; M6 return/tuple cases (field/global/
closure escape structurally blocked); M8 top-level globals **plus async/await with the
no-borrow-across-await rule** (branch `memory-model-m2-strict-and-async`). The MVP cornerstone —
single ownership, explicit `<-` move, lexical `&`/`&mut` borrows, use-after-move, borrow/alias
checking, deterministic scope-exit destruction, explicit `clone`, `free`, `Shared`/`Weak`, and
**multiple owned return values with destructuring** — is working end to end. Regression: 269/269
positive (syntax + codegen), 5/5 semantic_positive, 11/11 semantic_negative rejected, 7/7
strict_ownership. **The only genuinely blocked items** are the breaking strict-by-default flip of M2
(a v2 corpus-migration decision), M4 drop-elaboration for all owned locals (depends on the full
OCFG), owned-global shutdown destruction, and M8's `await` rule (infeasible without `async`).

---

## 11. Test plan

- **Compile-pass** (`semantic_positive`, `positive`): each valid example §9.1; destructor-order
  runtime tests (print on drop, assert reverse order); panic/early-return cleanup; multi-return;
  nested aggregates; closures; shared ref-count; FFI ownership annotations.
- **Compile-fail** (`semantic_negative`): each invalid example §9.2, asserting the *specific*
  diagnostic string (the runners already diff expected error text).
- **Runtime** (`native`): drop-order, double-free prevention, shared strong/weak counts, no-leak
  (allocation counter balances to zero at exit).
- **Property/fuzz**: random straight-line ownership programs → checker must never accept a
  use-after-move and never drop twice; random move/borrow sequences vs a reference interpreter of
  the state machine (§5); grammar-fuzz the parser for the new `return a,b` / destructuring forms.

---

## 12. Trade-off analysis

| System | Learning cost | Impl complexity | Runtime cost | Expressiveness | Concurrency | Safety |
|---|---|---|---|---|---|---|
| **This model** | **low** (no lifetimes; one move keyword) | **moderate** (affine + lexical borrow, no lifetime inference) | **zero** for owned; opt-in refcount for `Shared` | medium (no borrow-return, no self-ref) | manual sync for shared mutable | move/use-after-move/dangling-borrow/aliasing safe at compile time |
| Rust | high (lifetimes, variance, NLL) | high | zero + opt-in Rc/Arc | high | `Send/Sync` static data-race freedom | strongest static |
| C++ RAII | medium | low (already in language) | zero | high | none by default | **no** move/borrow checking (use-after-move is silent UB) |
| Swift/ObjC ARC | low | medium (compiler + runtime) | refcount on every strong op | high | atomic refcounts | no dangling, but cycles leak; no data-race |
| Go GC | very low | high (runtime GC) | GC pauses/throughput | high | GC-safe | memory-safe, not deterministic destruction |
| Vale | low–medium | high (generational refs / regions) | small check cost | high | evolving | use-after-free caught at runtime/compile |
| Cyclone regions | medium–high | high | zero | medium | n/a | region-safe, but region annotations leak into APIs |
| Linear/affine types | medium | medium | zero | low unless extended | n/a | very strong, but "use exactly once" is ergonomically harsh |
| Arena/region alloc | low to use | low | bulk free, possible fragmentation | low (lifetime = arena) | n/a | no per-object dangling within arena |

**Positioning:** this model = *C++ RAII's zero-cost determinism* + *an affine move/borrow checker
that is a strict subset of Rust's* (no lifetimes) + *opt-in ARC (`Shared`)*. It accepts less
expressiveness (no returned borrows, no self-referential structs without `Shared`/`Weak`) as the
price of a dramatically smaller rule set.

### Region-based option (evaluation)

A per-scope region ("everything a function allocates dies when the function returns") is attractive
for throughput and trivial cleanup, but: (a) it fights **destructors** (order within a bulk free is
unclear), (b) **escaping/returned** values must be promoted/allocated in the caller's region — this
is essentially the multiple-return `sret` story, workable but adds a promotion pass, (c) **long-lived
aggregates** and **object-graph promotion** are awkward, (d) **FFI** wants stable per-object
pointers. **Recommendation: per-object RAII (scope-exit drops) for the MVP**, with regions available
later as an *optimization* for provably non-escaping allocation-heavy scopes (an arena the drop pass
can target), not as the core semantic model.

---

## Diagnostics (beginner-friendly template)

Every ownership error follows the shape: **what happened → where ownership moved → why the later use
is invalid → the simplest fix (clone / borrow / move / Shared / restructure).** Examples:

```
error: use of moved value 'img'
  --> main.csec:9:12
   |
 8 |   result = take(<- img)      // 'img' moved here into 'take'
 9 |   return img.id() + result   // used here after it was moved
   |          ^^^ 'img' no longer owns a value
 help: if you need it in both places, duplicate it: clone(img)
       or borrow instead of consuming: take(&img)   (if 'take' can take &Image)
```

```
error: reference to local 'value' escapes its function
  --> lib.csec:3:12
 3 |   return &value              // 'value' is destroyed when 'invalid' returns
   |          ^^^^^^ this borrow would dangle
 help: return the value itself (move it): return value
       or return an owned copy: return clone(value)
```

These correspond 1:1 to §9.2. csec already emits the first family; M6 adds the escape family.

---

## Separation of concerns (summary map)

- **Source semantics:** §2, §3, §4.1–4.3 — what the programmer sees.
- **Static analysis:** §5, §7 — the state machine and dataflow (compile-time only).
- **Intermediate representation:** §6 (Ownership-CFG).
- **Runtime behavior:** §8 — dtors, `Shared` control block, global shutdown, unwind.
- **Backend ABI lowering:** §4.4 — tuple = aggregate; sret/registers; RVO.
- **MVP restrictions:** §10 M0–M8 + the explicit "deferred" list.
- **Future extensions:** partial moves, borrow fields, regions-as-optimization, richer capture,
  generalized owned-by-default for all non-copy types.
