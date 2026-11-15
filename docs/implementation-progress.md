# Execution Support Progress

## Verified

- Concurrency and the standard library (both compilers; see `LANGUAGE_SPEC.md`):
  microthreads, `go`/`async`/`await` futures, OS threads, OS processes, pipes, shared memory
  (including across processes), mail, `std.io` (chunked and memory-mapped input), `Nat` tools,
  `Complex`, and `std.symbolic`. `tests/run_microthread.ps1` (21 cases), `-Suite stdlib` (14), and
  `tests/run_selfhost_concurrency.ps1` (9, native and self-host must both exit 42). In the self-host,
  `spawn`/`threadStart`/`go`/`async` take named functions, and `Complex`/`std.math.complex` are
  native-only. The self-host now emits `external def` declarations with their real signatures, passes
  arguments of the declared types (Long, String), stores 64-bit values into `Long` locals, decodes
  escaped `Char` literals, and supports `text[i]`. Each of these changed `csec_compiler.csec`'s own
  output, and the regenerated output is a stable bootstrap fixed point.
- `text[i]` String byte indexing and target-aware optimisation: modules get the host triple and data
  layout before codegen, so `-O3` vectorises byte loops. `-march=native` uses the host CPU.
  `examples/parallel_wc.csec` reaches about 92% of a C++ mmap+SIMD reference on 16 threads.

- Heap allocations can be released explicitly with a `free(x)` builtin. In the direct compiler
  `new T[n]`, `new Class(...)`, and boxes are `malloc`-backed, so `free(x)` bit-casts the argument
  pointer and calls the runtime `free`; the type checker types the call as `Unit`. The self-host
  emitter allocates collections on the stack, so there is nothing to release there — it treats
  `free(x)` as a no-op (a real `free` on a stack pointer would corrupt memory). `free` therefore has
  consistent semantics on both paths (release heap memory where it exists, no-op where allocation is
  stack-based) and the same program result. `191_free_heap_allocation_execution.csec` returns `42`
  on both paths; freeing a class instance (`new Point(...)`) works the same way. Freeing a non-heap
  value (e.g. an array literal) is undefined, as in C.
- Every fixture under `tests/positive` compiles on the direct compiler, and the tracked execution
  suites pass on both paths. The `pmap(gpu)`/`preduce(gpu)` backend has no device code generator
  yet, so it falls back to CPU execution: `054_pmap_backend_variants` (which exercises `cpu`,
  `simd`, `gpu`, and implicit-`cpu` variants) compiles and returns `0` on both the direct and
  self-host paths. `gpu` is a performance hint that currently produces the same results as `cpu`;
  the loop still carries `csec.pmap.backend.gpu` metadata for a future GPU lowering pass. (The
  self-referential-class fixtures `063`/`101` remain non-terminating *as written* — an eager
  `var next: Node = new Node(0)` default constructs forever — and the compiler correctly diagnoses
  that rather than crashing; they are ill-formed programs, not a codegen gap.)
- The self-host compiler reaches an LLVM fixed point. Recompiling `selfhost/csec_compiler.csec`
  with `nativeflow_stage5_current.ll` reproduces `nativeflow_stage6_current.ll` byte-for-byte,
  and the regenerated compiler reproduces itself again. The regenerated compiler also passes the
  whole execution fixture list below, so the fixed point is functional and not just
  self-reproducing.
- Integer expressions, comparisons, boolean operations, calls, control flow, range loops,
  literal arrays with static indexing, imports, and common return paths execute through the
  self-host LLVM path.
- Dynamic `Int` arrays support `new Int[size]`, runtime indexing, element reads, writes, and
  compound assignments. The native compiler and self-host LLVM path both execute
  `tests/positive/04_collections/050_dynamic_array_indexing.csec` with exit code `45`.
- `Vector[Int]` parameters now use the pointer ABI at call sites, and indexed reads inside the
  callee use the same flat `Int` array storage. `057_vector_parameter_indexing.csec` exits with
  `42` through the self-host LLVM path.
- Non-capturing `Int -> Int` lambdas lower to hidden LLVM functions in both native and self-host
  compilation. The regression fixture `tests/positive/09_functional/095_lambda_invocation.csec`
  exits with `42` on both paths.
- Self-host lowering supports explicit by-value `Int` captures by passing captured values to the
  hidden lambda function. `096_lambda_explicit_capture_invocation.csec` exits with `42`.
- Capture-all `[=]` lambdas infer referenced outer `Int` locals and pass them by value. The
  regression fixture `097_lambda_capture_all_invocation.csec` exits with `42`.
- By-reference `[&]` `Int` captures pass outer storage pointers to the hidden lambda function,
  preserving mutations across calls. `098_lambda_by_reference_invocation.csec` exits with `12`.
- Unary `Int -> Int` function-type parameters lower to function pointers and invoke them
  indirectly. `099_higher_order_lambda_invocation.csec` passes a lambda to `apply` and exits
  with `42`.
- Binary `Int, Int -> Int` function-type parameters correctly preserve nested parameter commas,
  lower to function pointers, and invoke them indirectly. `100_higher_order_binary_lambda_invocation.csec`
  exits with `42`.
- Higher-order calls compose unary lambda values without losing the nested indirect-call result.
  `101_higher_order_compose_invocation.csec` exits with `41`.
- `Boolean`-returning lambda functions and function-type parameters now retain their `i1` ABI
  through hidden lambda definitions, direct higher-order calls, and indirect calls. The result is
  widened only when an `Int` control-flow path needs it. `102_higher_order_boolean_lambda_invocation.csec`
  exits with `42`.
- Explicit by-value captures also work for directly invoked `Boolean` lambdas.
  `103_boolean_capture_invocation.csec` exits with `42`.
- Lambda values now use a `{ code, environment }` closure ABI. It preserves captured `Int`
  values through higher-order calls for both `Int` and `Boolean` function signatures.
  `104_higher_order_capture_invocation.csec` and
  `105_higher_order_boolean_capture_invocation.csec` both exit with `42`.
- By-reference captures use pointer fields in that same closure environment, preserving mutation
  across higher-order calls. `106_higher_order_by_reference_capture_invocation.csec` exits with
  `12`.
- Ordinary `Boolean` parameters and return values preserve the LLVM `i1` ABI through direct
  calls and control-flow conditions. `107_boolean_parameter_and_return.csec` exits with `42`.
- Local `Boolean` declarations and assignments now use `i1` storage in both `Int` and
  `Boolean` function bodies. `108_boolean_local_state.csec` exits with `42`.
- String locals and parameters use pointer storage in the self-host LLVM path. `length` and
  `size()` lower through `csec_string_length`; `141_string_length_execution.csec` exits with
  `10`.
- String `==` and `!=` lower through `csec_string_equals` for literals, locals, and parameters
  on both paths. The direct compiler previously compared the pointers, so equal strings in
  distinct globals compared unequal; it now emits the same content comparison as the self-host
  path. `142_string_equality_execution.csec` exits with `42` through both `--emit-ir` and the
  self-host path.
- String concatenation lowers through `csec_string_concat`, and `String` functions return `ptr`
  values that can initialize String locals. `143_string_concat_and_return.csec` exits with `6`.
- Class layouts and pointer receivers support `String` constructor parameters and `String` fields.
  `072_string_constructor_field.csec` stores a String constructor value, invokes an instance
  method, and exits with `9` after lowering the field read through `ptr` storage.
- String-valued instance methods return class-backed pointer fields through the pointer ABI.
  `073_string_field_return.csec` returns a String from an instance method, initializes a String
  local from that call, and exits with `7`.
- String instance methods can concatenate a class-backed String field with a literal through
  `csec_string_concat`. `074_string_field_concat.csec` exits with `6`.
- String predicate members `isEmpty`, `contains`, `startsWith`, and `endsWith`, plus `indexOf`,
  lower through the runtime pointer ABI. `144_string_predicate_execution.csec` exits with `40`.
- String transformation members `trim`, `toUpper`, `toLower`, `substring`, and `toString` return
  pointer values that initialize String locals. `145_string_transform_execution.csec` exits with
  `10`.
- `String.charAt` lowers its `i8` runtime result into the `Int` expression ABI.
  `146_string_char_at_execution.csec` exits with `69`.
- Untyped String-transform locals retain the pointer ABI, and `println` lowers String and `Int`
  arguments to the native output runtime. The existing `120_string_members.csec` integration
  fixture now executes through the self-host LLVM path with exit code `0`.
- `Float` expressions now use LLVM `float` values end-to-end for literals, unary negation,
  `+`, `-`, `*`, `/`, direct calls, pointer-receiver class method calls, local storage, and
  comparisons in `Int` and `Boolean` control-flow conditions. Float-returning bodies support
  nested `if` and `while` CFG lowering with Float local assignment. `147_float_execution.csec`,
  `148_float_class_execution.csec`, and `112_basic_math_class.csec` all execute through the
  self-host LLVM path with exit code `0`.
- `Double` expressions now preserve their `double` ABI for literals, unary negation, arithmetic,
  direct and class-method calls, local storage, and Boolean control-flow conditions. Double-
  returning bodies support `if`/`while` CFG lowering with Double local assignment.
  `149_double_execution.csec`, `150_double_class_execution.csec`, and
  `151_double_control_flow_execution.csec` execute through the self-host LLVM path with exit
  code `0`.
- Boolean arrays now lower to stack-backed LLVM `i1` storage for literal and `new Boolean[n]`
  declarations, indexed loads, indexed assignment, and `Vector[Boolean]` parameter calls inside
  Int- and Boolean-returning functions. `152_boolean_array_execution.csec` and
  `153_boolean_vector_parameter_execution.csec` execute through the self-host LLVM path with exit
  code `0`.
- `Vector[Int]` values now preserve their pointer ABI when forwarding a parameter through a
  function return and storing that result in an explicitly typed local. The returned vector can be
  indexed by the caller; `154_vector_return_execution.csec` executes through the self-host LLVM
  path with exit code `0`.
- `Vector[String]` parameter forwarding also preserves the existing pointer storage on return.
  The caller can infer the returned vector element type when indexing it; the native and self-host
  LLVM paths both execute `158_string_vector_return_execution.csec` with exit code `0`.
- `Float` arrays now use stack-backed LLVM `float` storage for `new Float[n]`, indexed reads,
  indexed assignment, Float comparisons, and `Vector[Float]` parameter calls. The native and
  self-host LLVM paths both execute `155_float_array_execution.csec` with exit code `0`.
- `Vector[Float]` and `Vector[Double]` parameter forwarding preserve pointer storage when a
  function return initializes an explicitly typed local. The caller can index the returned vector;
  the native and self-host LLVM paths both execute `159_float_vector_return_execution.csec` and
  `160_double_vector_return_execution.csec` with exit code `0`.
- `Double` arrays now use stack-backed LLVM `double` storage for `new Double[n]`, indexed reads,
  indexed assignment, Double comparisons, and `Vector[Double]` parameter calls. The self-host
  and direct compiler LLVM paths both execute `156_double_array_execution.csec` with exit code
  `0`.
- `String` arrays now use stack-backed LLVM pointer storage for `new String[n]`, indexed reads,
  indexed assignment, and `Vector[String]` calls that return a String element. Canonical primitive
  names are also recognized during direct array creation. Local call initializers infer declared
  function return types, preserving pointer return values without an explicit annotation. The direct
  compiler and self-host LLVM paths both execute `157_string_array_execution.csec` with exit code
  `0`.
- `Char` arrays now preserve their LLVM `i8` element ABI for `new Char[n]`, indexed assignment,
  `Vector[Char]` parameter reads, Char-returning calls, and comparisons after widening to the
  integer condition ABI. `Vector[Char]` forwarding also preserves the underlying array pointer on
  return. The direct compiler and self-host LLVM paths both execute
  `161_char_array_execution.csec` and `162_char_vector_return_execution.csec` with exit code `0`.
- `Short` arrays now preserve their LLVM `i16` element ABI for `new Short[n]`, indexed
  assignment, `Vector[Short]` forwarding, Short-returning indexed reads, and comparisons. The
  direct compiler and self-host LLVM paths both execute `169_short_array_execution.csec` with
  exit code `0`.
- `Byte` arrays and `Vector[Byte]` now share the `Char` `i8` element ABI for allocation,
  indexed writes and reads, parameter calls, and comparisons. The self-host LLVM path executes
  `170_byte_array_execution.csec` with exit code `0`.
- `Long` values now preserve their LLVM `i64` ABI for 32-bit-overflowing integer literals, local
  storage, arithmetic, direct function calls, and comparisons. The direct compiler and self-host
  LLVM paths both execute `163_long_execution.csec` with exit code `0`.
- `Natural` and `Integer` now share the `Long` `i64` ABI in the self-host LLVM generator for
  function parameters and returns, local storage, arithmetic, calls, and comparisons.
  The direct parser also recognizes both as basic types, rather than class names. The direct and
  self-host LLVM paths both generate and execute `164_natural_execution.csec` and
  `165_integer_execution.csec` with exit code `0`, including values above the signed `Int` range.
- `Byte` now preserves its direct compiler `i8` ABI through self-host function parameters and
  returns, local storage, arithmetic, calls, and comparisons. The direct and self-host LLVM paths
  both execute `167_byte_execution.csec` with exit code `0`.
- The direct compiler declares every top-level function prototype before emitting any body, so a
  function may call another declared later in the file. Forward references and mutual recursion
  now work: previously a call to a not-yet-emitted function was dropped and its return value
  replaced with a default (`return isOdd(n - 1)` became `ret i1 false`).
  `181_mutual_recursion_execution.csec` exits with `0` on both paths.
- `new T[n]` heap allocates the array in the direct compiler instead of stack allocating it, so
  an array created in a function and returned stays valid in the caller. An array or vector local
  initialized from a function call binds the returned pointer directly, so indexing it does not
  read through an extra pointer slot. `178_new_array_return_execution.csec` builds an `Array[Int]`
  in a callee, returns it, and indexes it in `main`; it exits with `0` on both paths. (The
  self-host emitter already heap-allocated and indexed correctly.)
- `Vector` and `Array` are recognized as the same flat array type. The type checker accepts a
  function that declares `Vector[T]` and returns a `new T[n]` (typed `Array[T]`) value, and a
  local declared one way initialized from the other (`179_vector_return_alias_execution.csec`,
  `0` on both paths). `GenericType::equals` treats `Vector[T]` and `Array[T]` as equal, so
  overload resolution matches a `Vector` argument against an `Array` parameter and vice versa
  (`180_vector_array_arg_alias_execution.csec`, `0` on both paths). Relaxing the checker also
  resolved map/reduce/filter element types in `050_collection_in_function.csec`, which now
  compiles and runs (`0`) through the direct compiler.
- `Long` arrays and `Vector[Long]` now use stack-backed LLVM `i64` element storage for
  `new Long[n]` (also `Natural` and `Integer`), indexed assignment, indexed reads, parameter
  calls, comparisons, and forwarding an array pointer through a function return. `Long`-returning
  block bodies lower their `return` expression instead of falling back to a literal. The direct
  compiler and self-host LLVM paths both execute `171_long_array_execution.csec` and
  `173_long_vector_return_execution.csec` with exit code `0`.
- Call arguments split on commas that sit outside every nested group, so a nested call such as
  `addPair(addPair(1, 2), addPair(3, 4))` no longer splices the inner arguments into the outer
  call. The same depth-aware split applies to array literals, array dimension lists, and
  constructor argument lists. `172_nested_call_execution.csec` exits with `0` on both paths.
- Integer literal arrays without a type annotation stay `Int` arrays; they were previously given
  `i16` element storage and read back as `i32`. `Vector[Short]`, `Vector[Long]`,
  `Vector[Natural]`, and `Vector[Integer]` annotations select the narrower or wider element
  storage for a literal initializer. This restores `057_vector_parameter_indexing.csec` (`42`)
  and `154_vector_return_execution.csec` (`0`).
- `String` arguments passed to `Boolean`-returning functions keep the pointer ABI at the call
  site instead of being truncated to `i32`. This restores `142_string_equality_execution.csec`
  (`42`), which previously faulted.
- `String`-returning functions lower their whole body, not just the first `return` expression.
  Local declarations, assignments, `if`/`else if` chains, and loops that precede a `return` are
  now emitted. `Boolean`-returning bodies share the same statement lowering, so their locals keep
  their declared types instead of being allocated as `i1`. `174_string_body_control_flow.csec`
  exits with `0` on both paths.
- Statement lowering is shared across result types through `csec_generate_llvm_flat_body_typed`;
  only the `return` path differs per type. The `i32`, `i1`, `ptr`, `float`, and `double`
  generators all delegate to it, so every result type gets the same local-type dispatch, arrays,
  loops, and discarded calls. Nested `if`/`else if` chains now chain their `if.end` blocks
  together instead of emitting empty or unreachable blocks.
- `Float`- and `Double`-returning bodies keep the declared type of every local. They previously
  allocated only their own result type, so an `Int` or `String` local in a `Float` body was
  dropped and later references pointed at an alloca that was never emitted.
  `175_float_body_control_flow.csec` exits with `0` on both paths.
- Class constructor parameters of every scalar type get a layout slot, so `Long`, `Natural`,
  `Integer`, `Float`, `Double`, `Short`, and `Byte` constructor state is stored and read back
  through the pointer receiver. A class such as `Counter(start: Long)` previously produced an
  empty struct layout, which aborted generation of the entire module. `Long`-returning instance
  methods also lower through the class-method call path, which only `Int`, `Boolean`, `String`,
  `Float`, and `Double` had. `176_wide_class_field_execution.csec` exits with `0` on both paths.
- Class bodies also declare `Long`, `Natural`, `Integer`, `Float`, `Double`, `Short`, and `Byte`
  fields directly. They take layout slots, receive their declared initializers, and keep their
  width through reads and assignments inside methods, so mutation persists across calls.
  `Long`-, `Short`-, and `Byte`-returning block bodies lower their whole body rather than only
  the first `return`. `177_declared_wide_field_execution.csec` exits with `0` on both paths.
- Struct-backed class inheritance preserves ancestor `Int` field layout and dispatches
  `super.method()` with the same receiver pointer. `068_inherited_mutable_fields.csec` exits
  with `13`.
- Self-host lowering supports `Int`-returning static object methods and qualified calls. The
  regression fixture `tests/positive/05_oop/058_object_multiple_methods.csec` executes with
  exit code `85`.
- Self-host lowering supports classes whose constructor has `Int` parameters and whose methods
  read those parameters. Constructor values are passed as explicit receiver arguments to the
  lowered method. `052_method_calls_and_paths.csec` exits with `21`, and the multi-method
  `055_class_many_methods.csec` exits with `62`.
- Class methods also initialize and read `Int` fields declared with literal expressions in the
  class body. `053_objects_and_members.csec` executes with exit code `16`.
- Classes with `Int` fields use a struct-backed pointer receiver in the self-host LLVM path, so
  mutable field updates persist across calls. `056_class_field_types.csec` executes with exit
  code `44` after two `increment()` calls.
- Scalar-receiver classes support `this.field` reads and `super.method()` dispatch through the
  declared parent class. `064_this_and_super_paths.csec` executes with exit code `1`.
- The self-host emitter lowers `this.method(args)` self-calls: it dispatches to another method of
  the enclosing class on the same receiver (the receiver pointer for pointer-backed classes, or
  the reloaded scalar constructor state otherwise), passing any method arguments. It previously
  fell through to a zero constant. `182_this_method_self_call_execution.csec` exits with `0`
  through the self-host path. (The direct compiler's class-method codegen is still incomplete —
  see the inheritance gap above — so this fixture is self-host only.)
- Static objects support literal `Int` fields and expression-bodied methods. Together with mixed
  String/Int constructor classes, `059_class_object_mix.csec` executes with exit code `8113` on
  both paths. The direct compiler previously returned `8080` because an object's field symbols
  never reached the codegen scope: `SymbolTable::addSymbol` bailed out with `return false` when the
  field was already present in the enclosing `NamespaceSymbol.variables` map (populated during the
  type-check phase, which shares the symbol), so the emitted field global (`@maxRetries`) was never
  bound and lookups inside object methods resolved to nothing (`ret i32 0`). The namespace
  variable/field path now refreshes the persistent entry and always (re)binds the current-scope
  symbol, so object methods read their fields.
- Struct-backed class receivers now support `Boolean` constructor state and `Boolean` instance
  method returns with `i1` slots and calls. `069_boolean_constructor_state.csec` exits with `42`.
- Direct class-body `Boolean` fields support literal initialization, assignment in a Boolean
  method body, and persistent reads across calls. `070_boolean_field_mutation.csec` exits with
  `42`.
- Child class layouts carry inherited `Boolean` field slots, and `super` calls preserve their
  `i1` result ABI. `071_inherited_boolean_field.csec` exits with `42`.

## Building the Direct Compiler

The direct compiler (`csec++.exe`) links LLVM through the C++ API (`src/ast.h` includes
`<llvm/IR/Value.h>`), so it needs LLVM headers and libraries that the native-runtime DLL build
does not. This checkout carries them under `vcpkg_installed/x64-windows`, so a from-source build
works:

```powershell
cmake -S . -B build_cmake -G "Visual Studio 18 2026" -A x64 `
  -DLLVM_DIR="$PWD\vcpkg_installed\x64-windows\share\llvm"
cmake --build build_cmake --config Debug
```

Both `x64\Debug\csec++.exe` and `x64\Debug\System.Native.dll` are produced (the runtime DLL is
also rebuilt and copied next to the exe, so `build_runtime.bat` is only needed for a
runtime-only rebuild). The `.sln` also works but pins absolute LLVM paths, so CMake is safer.

## Known Gaps in the Direct Compiler

- Lambda closures now work in the direct compiler. Every lambda is a uniform `{ code, env }`
  closure: the function takes the environment pointer first, `[=]`/explicit captures store the
  outer values by value, `[&]` stores their addresses, and a lambda that captures nothing gets an
  empty environment. `IdentifierNode` returns a function-typed value (the closure pointer) as-is,
  and a call through a lambda variable or a function-type parameter loads the code and
  environment and calls `code(env, args)`. `095`–`106` (non-capturing, by-value capture,
  by-reference capture, and higher-order variants passing a lambda to another function) all
  execute correctly through the direct compiler as well as the self-host path. Closures are stack
  allocated, so a lambda that outlives the frame that created it is not yet supported; the
  fixtures use and pass lambdas within the creating frame.
- Array *literal* locals now work in the direct compiler. `val xs = [1, 2, 3]` binds the literal
  pointer directly (as `new T[n]` already did) instead of failing to store it into an aggregate
  slot, so `057_vector_parameter_indexing.csec` returns `42` and `152_boolean_array_execution.csec`
  returns `0`. Binding the literal made the nested-`pmap` lowering proceed where it previously
  bailed, which exposed a hang: the pmap capture walk (`collectPMapCaptures`) had no
  `PMapStatementNode` case, so an inner pmap's free variables were not captured and the outlined
  outer function referenced them across frames. Adding that case fixes `055_nested_pmap_shape`
  (no longer hangs).
- `map`/`filter` results bound to a local now feed a downstream `preduce` correctly in the direct
  compiler, so `056_preduce_variants` returns `143` (was `85`). Two problems were fixed: (1)
  `val squares = map(...)` failed to bind because a `map`/`filter` result is a bare element-base
  pointer that did not match the declared array LLVM type, leaving the variable undefined — those
  initializers now bind directly like an array literal; and (2) the bare pointer carries no length,
  so a consuming `preduce` recovered a size of zero (`filter`'s result type has no static size at
  all, and its surviving count is dynamic). `map`/`filter` now record a runtime element count in a
  `CodeGenerator.arrayRuntimeLength` sidecar keyed by the result pointer (`map` keeps the source
  length; `filter` records the count of elements that passed the predicate), and
  `ReduceStatementNode` uses that runtime length as its loop bound when present. The self-host path
  still computes `056` incorrectly; this fix is direct-compiler-only (no emitter or fixed-point
  change).
- A call to an inherited method now resolves its return type in the direct compiler. During type
  checking a child class's method map does not yet contain copies of inherited methods (that copy
  happens at codegen), so `MethodCallNode::getType` — which only looked up the receiver's own class
  — returned `Real` for `dog.getAge()` (defined on the parent `Animal`), and a caller summing it
  into an `Int` failed with "declared to return 'Int' but returns 'Real'". The class-method path
  now walks the parent chain (like the `this`/`super` path already did). `108_oop_and_templates`,
  which combines inheritance, a static object, and generic (`template`) classes, compiles and
  returns `17`; this was the last non-`gpu` fixture the direct compiler could not build.
- Overloaded object/namespace methods no longer collide in the direct compiler. A namespaced
  method mangles purely by scope and name (`O#p`), so `def p(x: String)` and `def p(x: Boolean)`
  produced the same LLVM name; the second body attached to the first function and read its
  parameter with the wrong type — a Boolean body over a pointer argument emitted an illegal
  `zext ptr -> i32` and aborted codegen (`114_out_object`), while a `p(Int)` collision silently
  produced wrong code. An overload whose signature differs from an existing same-named function now
  gets a signature-qualified name (`O#p.Boolean`); `findNamespacedFunctionByArgs` already matches
  both the base name and `base.*`, so calls still resolve to the right overload.
  `114_out_object` compiles and runs (exit `0`).
- An `import` whose path uses non-ASCII (e.g. Korean) identifiers no longer hangs the compiler.
  Import expansion built a `std::filesystem::path` directly from the raw UTF-8 `import` target;
  on Windows that decodes the narrow string through the active code page, and the MSVC STL spins
  forever on some multi-byte sequences. The target is now decoded as UTF-8 via `std::filesystem::
  u8path` (a small `utf8Path` helper). `082_utf8_imports_and_members` compiles and returns `6`
  (`계산기.두배(3)`); it previously hung `expandImports` before lexing.
- Nested array indexing now works in the direct compiler. An array-of-arrays literal
  (`[[1, 2], [3, 4]]`) is stored as a buffer of pointers to the inner arrays, but `getLLVMType`
  lowers an `ArrayType` element to an inline `[N x T]` aggregate. Indexing through an intermediate
  array level therefore loaded the inner array as an aggregate value and the next `GEP` asserted
  ("Ptr must have pointer type"). `ArrayAccessNode` (both the value and element-pointer paths) now
  loads an array-typed intermediate element as an opaque pointer, matching the stored
  representation. `043_indexing_and_compound_assignment` returns `28` (chained `nested[0][1]`) and
  `111_tensor_indexing` returns `9` (comma multi-index `matrix[0, 1]` and `cube[1, 0, 1]`); both
  previously crashed the compiler.
- A self-referential class that eagerly constructs its own type in a field default
  (`var next: Node = new Node(0)`) no longer crashes the direct compiler. Constructor field
  initializers are emitted inline, so such a default recurses forever at codegen and previously
  overflowed the stack (`063_class_self_reference`, `101_linked_list` aborted with `0xC00000FD`).
  Construction now records the class being built in `CodeGenerator.classesUnderConstruction` and,
  on re-entry into the same class's field initialization, emits a diagnostic
  ("eagerly constructs its own type in a field initializer (infinite construction); reference or
  nullable fields are not yet supported") instead of crashing. The guard sits after the
  constructor-argument loop so a legitimate same-class argument (`new Wrap(new Box(42))`, or
  `new A(new A(5))`) is unaffected. Properly supporting recursive data structures still needs
  out-of-line constructors and reference/nullable field semantics; this only makes the compiler
  robust against the non-terminating case.
- Class inheritance now works in the direct compiler. `this` is bound to the class type during
  type checking so `this.field` and `this.method()`/`super.method()` resolve to real types;
  `AccessFieldNode::codegen` reads the field value (with a pointer accessor for assignment
  targets); `this`/`super` method calls dispatch to `Class_method` on the receiver, walking the
  parent chain for inherited and overridden methods; and the class struct layout and field
  bindings include inherited fields in declaration order. `064_this_and_super_paths` (`1`),
  `068_inherited_mutable_fields` (`13`), and `071_inherited_boolean_field` (`42`) execute
  correctly through both the direct compiler and the self-host path.

## Operator and Method Overloading (Self-Host)

Operator overloading works in the self-host emitter. Operator method names are mangled to legal
LLVM identifiers (`operator+` becomes `Counter_operator_add`), and a binary operator whose left
operand is a class instance with a matching `operator<op>` method dispatches to that method
instead of the primitive instruction. The guard only fires when the left operand is a single
local bound to `new Class(...)` whose class defines the operator, so primitive arithmetic is
untouched (the fixed point is unchanged). `116_operator_overload_class.csec` exits `0`, and
`183_operator_overload_dispatch_execution.csec` verifies through its exit code that `counter + 41`
and `counter * 21` actually call the operator methods. Operator methods on scalar-receiver classes
that read constructor state now receive it: because a scalar class's methods take the constructor
values as leading parameters (the same mechanism that lets `c.get()` read a field), the operator
dispatch re-evaluates the `new Class(...)` constructor arguments and passes them as the receiver
state before the operand. `189_scalar_operator_state_execution.csec` — `Counter(37) + 5` whose
`operator+` returns `value + right` — returns `42` on both the direct and self-host paths (the
fixed point is unchanged, since `csec_compiler.csec` defines no scalar-class operators).

A `val` bound to a String-returning instance method call (`val g = n.greet("Bob")`) is now typed as
a String rather than Int. `csec_c_local_type` only recognised the built-in String methods
(`trim`/`toUpper`/…); a user class method returning String fell through to Int, so the local was
allocated as an i32 slot for a pointer result. It now resolves the receiver's class and the called
overload's return type (through a new `csec_instance_call_returns_string` helper declared in
`NativeRuntime.h`, so it is reachable from `csec_c_local_type`'s translation region). A second bug
surfaced once the type was correct: the String-method fast-paths in `csec_class_method_definition`
emit a `(ptr %arg.this)` field-accessor signature and so are valid only for parameter-less getters;
a method with its own parameters (`greet(who)`) was matching the field-return fallback, dropping
`who`. Those fast-paths are now gated on the method having no parameters, so `greet` falls through to
the general generation that keeps its parameter. `190_string_method_state_execution.csec` —
`Name("Hi ").greet("Bob").length` — returns `6` on both paths, with the fixed point unchanged.

String-returning instance methods now accept arguments. `csec_emit_ptr_instance_call` lowers the
method arguments by their declared parameter types and handles pointer-receiver and stateless
classes, so `g.greet("hi")` and `g.wrap("[", "x")` execute. `184_string_method_argument_execution`
exits `0` through the self-host path. A scalar-receiver class that carries constructor state is
still not passed that state into such a call.

Method overloading works in the self-host emitter. Overloaded methods get a per-signature LLVM
name (the parameter type names are appended, so `Formatter_show_Int` and `Formatter_show_String`
no longer collide), and a call resolves to the overload whose parameter types match the argument
expressions (String versus non-String). The instance-call handlers lower each argument by the
resolved overload's parameter type, and `csec_is_string_expression` recognizes an instance call
that resolves to a String-returning overload so untyped contexts such as `println` route it
through the String lowering. `117_method_overloading` exits `0`, and
`185_method_overload_dispatch_execution` verifies through its exit code that overloads differing
only by parameter type (`size(Int)` and `size(String)`, both returning Int) dispatch correctly.

Method overriding through inheritance works. An overridden method dispatches on the instance's
own class, and `118_method_overriding` prints `2`, `child=7`, and `child=ready` correctly.
Operator overloading composes with overriding and with overloaded operators: `119_operator_over`
`loading_and_overriding` prints `5`, `base+ok`, `14`, and `scaled+ok`, dispatching each `+` to the
Int or String operator of the instance's class (including the overriding subclass).

`String` concatenation now accepts a numeric operand on either side: `"count=" + count` and the
chain `"a" + 1 + "b"` stringify the integer through `csec_to_string_i64` before concatenating.
`csec_is_string_expression` treats a `+` as a string concatenation when either operand is a
string, and instance operator expressions returning String are recognized so untyped contexts
route them through the String lowering. `186_string_int_concat_execution` exits `0`.

A top-level function that returns String is recognized as a String expression, so it composes
with concatenation: `s + repeat(s, n - 1)` (a recursive String builder) and passing one String
function's result to another both work. `187_string_function_concat_execution` exits `0`.

A concatenation chain with a function call in the middle and string literals on both ends
(`"<" + rep("x", 2) + ">"`) now lowers correctly on the self-host path.
`188_string_concat_chain_execution` returns `5` (the length of `<xxx>`) through both the direct and
self-host paths. The root cause was that `csec_i32_top_level_operator` computes an operator's
precedence from a token's *text* without checking its kind, so a string literal such as `"<"`/`">"`
(text `<`/`>`) was treated as a comparison operator (and a string like `"("` corrupts the bracket
depth); for `"<" + rep(...) + ">"` the trailing `">"` outranked the `+`, so the concatenation case
never fired and the expression dropped to the null-pointer fallback. Rather than change that shared
scanner — which also drives the i32/i1 lowering and, when "fixed", re-split a large boolean
condition in `csec_compiler.csec` and exposed latent emitter bugs that broke the bootstrap — a
scoped `csec_ptr_top_level_operator` (which counts only genuine `'O'`/`'K'` operator tokens) is used
in `csec_emit_ptr_expression` and `csec_is_string_expression`. Because it is confined to the
pointer/String lowering, `csec_compiler.csec` compiles byte-identically (the fixed point is
unchanged) while user concatenations split on the real `+`.

Overloaded operator methods that read scalar-receiver constructor state are still not passed that
state (the operator dispatch, like the method-call dispatch, only forwards a pointer receiver or
none).

## Still Incomplete

- The self-hosted compiler does not implement the additions of LANGUAGE_SPEC.md §14-§18
  (default/named arguments, `break`/`continue`, enums and patterns, `?`, interfaces, template bounds,
  tensor reductions, `-g`); they are direct-compiler (`csec++`) features.
- Automatic drop (§14.6) frees an unshared `new` local but not the objects its fields own, nor
  arrays; tensor storage is double for every element type (§17.1). `-g` describes functions and
  lines, not local variables.
- Self-host: an expression-bodied function that concatenates a String parameter
  (`= f(text + "\n")`) treats the parameter as an Int. Block bodies work.
- `selfhost/llvm_subset.csec` (bootstrap smoke test) and `10_integration/203` crash or hang with
  the pre-change runtime as well; both are pre-existing.

- Lambda closures: non-`Int` signatures beyond the verified `Boolean` return ABI, broader
  higher-order ABI coverage, and closure environment ownership.
- Arrays beyond the current `Int`/`Boolean`/`Byte`/`Char`/`Short`/`Long`/`Float`/`Double`/`String`
  flat-storage path: other element types, nested/dynamic shape semantics, newly-created array
  returns, bounds behavior, and general ownership.
- Unannotated float literal arrays default to `Float` storage rather than `Double`.
- Reference semantics beyond the current stack-backed instance path, and general field access
  syntax. `Int` inheritance, including struct-backed parent fields and `super` dispatch, is
  supported, as is scalar constructor and declared-field state of every width.
- Returning a newly created array now executes correctly through the direct compiler as well as the
  self-host path. `178_new_array_return_execution` (`Array[Int]` return) and
  `154_vector_return_execution` (`Vector[Int]` parameter/return) both return the expected values,
  and a `Vector[Int]` return of `new Int[n]` reads back correctly (`v[0] + v[1]`). The earlier
  "declared to return 'Vector' but returns 'Array'" rejection and wrong-value behavior no longer
  reproduce — the array-literal/`new T[n]` direct-binding work fixed it.
- Rich string/char interactions, broad function ABI coverage, and mixed numeric conversion
  semantics remain incomplete in the self-host path. Enums, variant patterns, guards and
  exhaustiveness (LANGUAGE_SPEC.md §15) and for-each over arrays run in the direct compiler
  (`tests/positive/14_language`).

## Validation Commands

The native runtime (`System.Native.dll`) must be rebuilt and copied next to `csec++.exe` after
any change under `src/native_runtime/`; `build_runtime.bat` at the repository root does both
steps.

```powershell
# Self-host Long array, Vector[Long] forwarding, and nested call paths (intentional return: 0)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\171_long_array_execution.csec `
  .\selfhost\long_array_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\long_array_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\173_long_vector_return_execution.csec `
  .\selfhost\long_vector_return_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\long_vector_return_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\172_nested_call_execution.csec `
  .\selfhost\nested_call_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\nested_call_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\174_string_body_control_flow.csec `
  .\selfhost\string_body_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_body_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\175_float_body_control_flow.csec `
  .\selfhost\float_body_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\float_body_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\176_wide_class_field_execution.csec `
  .\selfhost\wide_class_field_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\wide_class_field_selfhost_probe.ll

# Self-host bootstrap fixed point: regenerating the compiler must reproduce stage 6 exactly
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\selfhost\csec_compiler.csec .\selfhost\stage6_check.ll llvm
Compare-Object (Get-Content .\selfhost\stage6_check.ll) `
  (Get-Content .\selfhost\nativeflow_stage6_current.ll)

# Native dynamic array path
.\x64\Debug\csec++.exe --emit-ir -o .\selfhost\dynamic_array_check.ll `
  .\tests\positive\04_collections\050_dynamic_array_indexing.csec
.\x64\Debug\csec++.exe --run-ir .\selfhost\dynamic_array_check.ll

# Self-host dynamic array path
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\04_collections\050_dynamic_array_indexing.csec `
  .\selfhost\dynamic_array_selfhost.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\dynamic_array_selfhost.ll

# Self-host Vector[Int] parameter and indexed-read path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\04_collections\057_vector_parameter_indexing.csec `
  .\selfhost\vector_parameter_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\vector_parameter_selfhost_probe.ll

# Self-host static object method path (the program intentionally returns 85)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\058_object_multiple_methods.csec `
  .\selfhost\object_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\object_selfhost_probe.ll

# Self-host class instance method path (intentional returns: 21 and 62)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\052_method_calls_and_paths.csec `
  .\selfhost\oop_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\oop_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\055_class_many_methods.csec `
  .\selfhost\class_methods_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\class_methods_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\053_objects_and_members.csec `
  .\selfhost\class_fields_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\class_fields_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\056_class_field_types.csec `
  .\selfhost\class_mutable_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\class_mutable_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\064_this_and_super_paths.csec `
  .\selfhost\inheritance_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\inheritance_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\059_class_object_mix.csec `
  .\selfhost\class_object_mix_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\class_object_mix_probe.ll

# Self-host non-capturing lambda path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\095_lambda_invocation.csec `
  .\selfhost\lambda_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\lambda_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\096_lambda_explicit_capture_invocation.csec `
  .\selfhost\lambda_capture_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\lambda_capture_selfhost_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\097_lambda_capture_all_invocation.csec `
  .\selfhost\lambda_capture_all_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\lambda_capture_all_selfhost_probe.ll

# Self-host struct-backed inheritance path (intentional return: 13)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\068_inherited_mutable_fields.csec `
  .\selfhost\inheritance_mutable_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\inheritance_mutable_selfhost_probe.ll

# Self-host Boolean class constructor and instance method path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\069_boolean_constructor_state.csec `
  .\selfhost\boolean_class_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\boolean_class_selfhost_probe.ll

# Self-host mutable Boolean class field path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\070_boolean_field_mutation.csec `
  .\selfhost\boolean_field_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\boolean_field_selfhost_probe.ll

# Self-host inherited Boolean field and super-call path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\071_inherited_boolean_field.csec `
  .\selfhost\inherited_boolean_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\inherited_boolean_selfhost_probe.ll

# Self-host String constructor field and instance method path (intentional return: 9)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\072_string_constructor_field.csec `
  .\selfhost\string_constructor_field_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_constructor_field_selfhost_probe.ll

# Self-host String field return and caller-local initialization path (intentional return: 7)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\073_string_field_return.csec `
  .\selfhost\string_field_return_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_field_return_selfhost_probe.ll

# Self-host String field concatenation in an instance method (intentional return: 6)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\05_oop\074_string_field_concat.csec `
  .\selfhost\string_field_concat_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_field_concat_probe.ll

# Self-host by-reference lambda capture path (intentional return: 12)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\098_lambda_by_reference_invocation.csec `
  .\selfhost\lambda_byref_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\lambda_byref_selfhost_probe.ll

# Self-host higher-order lambda path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\099_higher_order_lambda_invocation.csec `
  .\selfhost\higher_order_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\higher_order_selfhost_probe.ll

# Self-host binary higher-order lambda path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\100_higher_order_binary_lambda_invocation.csec `
  .\selfhost\higher_order_binary_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\higher_order_binary_selfhost_probe.ll

# Self-host composed higher-order lambda path (intentional return: 41)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\101_higher_order_compose_invocation.csec `
  .\selfhost\higher_order_compose_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\higher_order_compose_selfhost_probe.ll

# Self-host Boolean higher-order lambda path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\102_higher_order_boolean_lambda_invocation.csec `
  .\selfhost\higher_order_boolean_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\higher_order_boolean_selfhost_probe.ll

# Self-host direct Boolean capture lambda path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\103_boolean_capture_invocation.csec `
  .\selfhost\boolean_capture_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\boolean_capture_selfhost_probe.ll

# Self-host captured closure higher-order paths (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\104_higher_order_capture_invocation.csec `
  .\selfhost\closure_int_hof_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\closure_int_hof_probe.ll
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\105_higher_order_boolean_capture_invocation.csec `
  .\selfhost\closure_bool_hof_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\closure_bool_hof_probe.ll

# Self-host by-reference closure higher-order path (intentional return: 12)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\106_higher_order_by_reference_capture_invocation.csec `
  .\selfhost\closure_byref_hof_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\closure_byref_hof_probe.ll

# Self-host Boolean parameter and return ABI path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\107_boolean_parameter_and_return.csec `
  .\selfhost\boolean_param_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\boolean_param_selfhost_probe.ll

# Self-host Boolean local declaration and assignment path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\09_functional\108_boolean_local_state.csec `
  .\selfhost\boolean_local_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\boolean_local_selfhost_probe.ll

# Self-host String local, parameter, length and size path (intentional return: 10)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\141_string_length_execution.csec `
  .\selfhost\string_length_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_length_selfhost_probe.ll

# Self-host String equality and inequality path (intentional return: 42)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\142_string_equality_execution.csec `
  .\selfhost\string_equality_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_equality_selfhost_probe.ll

# Self-host String concatenation and return-value path (intentional return: 6)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\143_string_concat_and_return.csec `
  .\selfhost\string_concat_selfhost_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_concat_selfhost_probe.ll

# Self-host String predicate and index member path (intentional return: 40)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\144_string_predicate_execution.csec `
  .\selfhost\string_predicate_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_predicate_probe.ll

# Self-host String transform member path (intentional return: 10)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\145_string_transform_execution.csec `
  .\selfhost\string_transform_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_transform_probe.ll

# Self-host String character access path (intentional return: 69)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\146_string_char_at_execution.csec `
  .\selfhost\string_char_at_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_char_at_probe.ll

# Self-host full String-member and println integration path (intentional return: 0)
.\x64\Debug\csec++.exe --run-ir .\selfhost\nativeflow_stage5_current.ll -- `
  .\tests\positive\10_integration\120_string_members.csec `
  .\selfhost\string_members_probe.ll llvm
.\x64\Debug\csec++.exe --run-ir .\selfhost\string_members_probe.ll
```
