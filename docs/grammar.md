# Tessera Grammar (normative)

This is the normative grammar for Tessera as implemented by the `csec++` front end
(`src/lexer.cpp`, `src/parser.cpp`, `src/Token.cpp`, `src/Expression.cpp`,
`src/Statement.cpp`, `src/Declaration.cpp`). The self-host lexer
(`src/native_runtime/lexer_runtime.inc`) mirrors the lexical grammar; keep both
in sync with this document. `docs/syntax.md` is the tutorial companion; where the
two differ, *this file* is authoritative.

## Notation

EBNF-ish: `A B` = sequence, `A | B` = alternative, `[A]` = optional, `{A}` =
zero-or-more, `(A)` = grouping, `'x'` = literal token, `« … »` = prose
constraint. Terminals are the token kinds produced by the lexer.

---

## 1. Lexical grammar

```
token        = keyword | identifier | literal | operator | comment ;
whitespace   = ' ' | '\t' | '\r' | '\n' ;                 (* skipped, not significant *)
comment      = '//' { any-but-newline }
             | '/*' { any } '*/' ;                          (* skipped *)
```

### 1.1 Identifiers and keywords

```
identifier   = (letter | '_') { letter | digit | '_' } ;   (* not a keyword *)
```

**Reserved keywords** (cannot be identifiers):

```
case class def else extends false for if import match new null object
operator override return struct super this true val var while
to until and or xor
map pmap reduce preduce filter
external inline constexpr template typename mut box unsafe unatomic
        async await go
        build break continue enum
molecule cfd ode protein
```

Everything else is a free identifier. The physics-DSL *body* words
(`atom bond at steps dt temperature grid viscosity velocity euler from step
lattice spacing chain mcmc`) are **contextual**: reserved only inside a
`molecule`/`cfd`/`ode`/`protein` block (matched via `Parser::matchWord`), and
ordinary identifiers everywhere else.

### 1.2 Literals

```
literal      = int-lit | decimal-lit | exp-lit | float-lit | char-lit | string-lit
             | regex-lit | bool-lit | 'null' ;
int-lit      = dec-int | '0x' hex { hex } | '0b' bin { bin } | '0o' oct { oct } ;
dec-int      = digit { digit } ;                            (* arbitrary length; Nat targets read it exactly *)
decimal-lit  = digit { digit } '.' digit { digit } ;          (* a Double *)
exp-lit      = ( decimal-lit | dec-int ) ('e'|'E') ['+'|'-'] digit { digit } ;   (* a Double *)
float-lit    = ( decimal-lit | exp-lit ) ( 'f' | 'F' ) ;      (* a Float: 1.5f *)
char-lit     = "'" ( char | '\' escape ) "'" ;
string-lit   = ['u'] '"' { char | '\' escape } '"' ;        (* u"…" = explicit unicode; plain is unicode too *)
regex-lit    = 'r' '"' { char } '"' ;                        (* used by `match` patterns *)
bool-lit     = 'true' | 'false' ;
```

### 1.3 Operators

Canonical operator set (maximal-munch: longest match wins). Mirrored in both
lexers — see `src/lexer.cpp:initializeOperators` and
`native_runtime/lexer_runtime.inc`.

```
assignment   = '=' | '+=' | '-=' | '*=' | '/=' | '%=' | '<-' ;
arithmetic   = '+' | '-' | '*' | '/' | '%' | '**' ;          (* '**' = power *)
comparison   = '==' | '!=' | '<' | '>' | '<=' | '>=' ;
bitwise      = '&' | '|' | '^' | '~' | '<<' | '>>' ;
pipeline     = '>>|' | '<<|' ;                              (* x >>| f(a) = f(x, a); tuple input expands: (x, a) >>| f = f(x, a) *)
increment    = '++' | '--' ;
borrow/move  = '&' | '<-' ;                                  (* '&mut' = '&' 'mut' *)
punct        = ':' '.' ',' ';' '(' ')' '{' '}' '[' ']' '_' ;
arrows       = '->' | '=>' ;                                 (* '->' lambda body / '=>' match-arm & fn-type *)
attribute    = '[@' ;
latex        = '$' | '$$' | '\' ;
misc         = '!' '#' '@' "'" '<:' '<%' '>:' '?' ;          (* '?' = postfix try, see 5 *)
logical      = '&&' | '||' ;                                 (* spellings of the keywords and / or *)
```

Note: boolean logic uses the keywords `and` / `or` / `xor`; `&&` and `||` are
the same as `and` and `or`. `&` / `|` are bitwise.

A decimal literal is a `Double`. Where a `Float` is expected (a `Float` variable,
parameter, return value or the other operand of a `Float`), a literal expression
is evaluated as a `Float`; `1.5f` is a `Float` anywhere.

---

## 2. Program and declarations

```
program      = { top-decl } ;
top-decl     = import | attribute* ( function | class | struct | object
             | external-fn | template-decl | enum-decl | interface-decl ) ;

import       = 'import' ( string-lit | dotted-name ) [ '=' ident ] [ ';' ] ;
               (* import std.io ;  import std.dataframe = df ;  see LANGUAGE_SPEC.md 1.1 *)
dotted-name  = identifier { '.' identifier } ;
attribute    = '[@' ident [ '(' [ arg-list ] ')' ] ']' ;     (* [@DllImport("m","cos")], [@Intrinsic] *)

function     = [ 'async' ] [ 'unsafe' ] [ 'inline' ] [ 'constexpr' ]
               'def' fn-name '(' [ params ] ')' [ ':' type ] { contract } fn-body ;
contract     = 'requires' expr | 'ensures' expr ;            (* contextual words; see LANGUAGE_SPEC.md 12 *)
fn-name      = member-name | 'operator' operator ;        (* a keyword may name a method: def filter(...) *)
                                                            (* operator - / + / ! / ~ with no parameters overloads the unary form *)
member-name  = identifier | keyword ;                      (* after '.' or 'def' only *)
fn-body      = block | '=' expr ;                            (* block form or expression body *)
params       = param { ',' param } ;
param        = identifier ':' type [ '=' expr ] ;           (* a default: evaluated at each call that omits it;
                                                               parameters after one with a default need one too *)

external-fn  = attribute* 'external' 'def' identifier '(' [ params ] ')' [ ':' type ] ';' ;

class        = 'class' identifier [ '(' [ params ] ')' ] [ 'extends' type ] [ implements ] class-body ;
struct       = 'struct' identifier '(' [ params ] ')' [ implements ] [ class-body ] ;   (* value type *)
implements   = 'implements' identifier { ',' identifier } ;   (* contextual word *)
object       = 'object' identifier class-body ;
class-body   = '{' { member } '}' ;
member       = field | method ;
field        = ( 'val' | 'var' ) identifier ':' type [ '=' expr ] [ ';' ] ;
method       = [ 'override' ] function ;

template-decl= 'template' '<' tparam { ',' tparam } '>' ( function | class | struct ) ;
tparam       = ( 'typename' | 'class' ) identifier [ ':' identifier { '+' identifier } ]
                                                            (* type parameter, optionally bounded by interfaces *)
             | type identifier ;                            (* non-type (e.g. Int N) *)

enum-decl    = 'enum' identifier [ '<' identifier { ',' identifier } '>' ] [ implements ]
               '{' variant { ( ',' | ';' ) variant } { method } '}' ;
variant      = identifier [ '(' [ params ] ')' ] ;          (* Circle(radius: Double), Empty *)

interface-decl = 'interface' identifier '{' { 'def' fn-name '(' [ params ] ')' [ ':' type ] [ fn-body ] [ ';' ] } '}' ;
                                                            (* `interface` is contextual; a body is a default *)
```

---

## 3. Types

```
type         = borrow-type | box-type | fn-type | tuple-type | applied-type ;
borrow-type  = '&' [ 'mut' ] type ;
box-type     = 'box' type ;
fn-type      = '(' [ type { ',' type } ] ')' '=>' type ;
tuple-type   = '(' type ',' type { ',' type } ')' ;         (* multiple-return *)
applied-type = type-name [ generic-args ] ;
generic-args = '<' type-arg { ',' type-arg } '>'            (* Tensor<Double,3,3> *)
             | '[' type-arg { ',' type-arg } ']' ;          (* Vector[Int], Matrix[Double] *)
type-arg     = type | int-lit ;                             (* non-type args: dimensions *)
type-name    = 'Int' | 'Long' | 'Double' | 'Float' | 'Real' | 'Bool' | 'Boolean'
             | 'Char' | 'String' | 'Nat' | 'Unit' | 'Void'
             | 'Tensor' | 'Vector' | 'Matrix' | 'Shared' | 'Weak' | 'Dict' | 'Set' | identifier ;
```

`Nat` is an arbitrary-precision integer (reference type). `struct`/value types are
Copy; reference `class`, arrays, `box T`, `Shared`/`Weak` are owned (move-checked).

---

## 4. Statements

```
statement    = decl-stmt | assign-stmt | if-stmt | while-stmt | for-stmt
             | return-stmt | loop-control | match-stmt | comprehension | dsl-block | expr-stmt ;
loop-control = ( 'break' | 'continue' ) terminator ;        (* inside while/for only *)
terminator   = [ ';' ] ;                                    (* see §7 *)

decl-stmt    = ( 'val' | 'var' ) l-names [ ':' type ] [ '=' expr ] terminator ;
                                                            (* no ':' type -> the type of expr *)
l-names      = identifier { ',' ( identifier | '_' ) }      (* destructuring bind *)
             | '$' latex-name '$' ;                         (* `val $\alpha$ = 0.5;` binds the name a formula uses *)

assign-stmt  = l-value assignment expr terminator ;
l-value      = identifier
             | l-value '.' identifier                       (* field / vector component *)
             | l-value '[' index-list ']'                   (* array/tensor element or section *)
             | names ;                                       (* tuple destructure: a, b, c = f() *)

if-stmt      = 'if' '(' expr ')' block [ 'else' ( block | if-stmt ) ] ;
while-stmt   = 'while' '(' expr ')' { loop-spec } block ;
for-stmt     = 'for' '(' identifier '<-' iterable ')' { loop-spec } block ;
loop-spec    = 'invariant' expr | 'decreases' expr ;        (* checked by csec++ --verify only *)
             | 'for' '(' identifier ',' identifier '<-' token-range ')' block ;
             | 'for' '(' identifier ',' identifier '<-' expr ')' block ;   (* key, value of a Dict *)
iterable     = expr | range ;                              (* an array, a range, a Dict or a Set *)
token-range  = identifier '.between(' expr ',' expr ')' [ '.withoutTrivia()' ] ;
range        = expr ( 'to' | 'until' ) expr [ 'step' expr ] ;   (* 'to' inclusive, 'until' exclusive *)
return-stmt  = 'return' [ expr { ',' expr } ] terminator ;   (* multiple values → tuple *)

comprehension= ( 'map' | 'filter' ) '(' identifier '<-' iterable ')' block
             | 'reduce' '(' identifier ',' identifier '<-' iterable ',' expr ')' block
             | ( 'pmap' ) '(' policy ',' identifier '<-' iterable ')' block
             | ( 'preduce' ) '(' policy ',' identifier ',' identifier '<-' iterable ',' expr ')' block ;
policy       = 'cpu' | 'simd' | 'openmp' | 'gpu' ;           (* contextual *)

expr-stmt    = expr terminator ;                            (* assert(c[, msg]) is a checked builtin *)
block        = '{' { statement } '}' ;
```

`dsl-block` is the physics simulation DSL (`molecule { … }`, `cfd { … }`,
`ode { … }`, `protein { … }`) with contextual body words; see `Statement.cpp`.

---

## 5. Expressions (by precedence, lowest → highest)

Recursive-descent cascade in `Expression.cpp`. Each level is left-associative
unless noted.

```
expr         = assignment-expr ;
assignment-expr = or-expr [ assignment expr ] ;             (* right-assoc *)
or-expr      = and-expr { ( 'or' | 'implies' ) and-expr } ;
and-expr     = compare-expr { 'and' compare-expr }
             | compare-expr { 'xor' compare-expr } ;
compare-expr = range-expr { comparison range-expr } ;
range-expr   = set-expr { ( 'to' | 'until' ) set-expr } ;
set-expr     = bitor-expr { ( 'union' | 'intersect' | 'diff' | … ) bitor-expr } ;
bitor-expr   = bitand-expr { ( '|' | '^' ) bitand-expr } ;
bitand-expr  = shift-expr { '&' shift-expr } ;
shift-expr   = add-expr { ( '<<' | '>>' ) add-expr } ;
add-expr     = mul-expr { ( '+' | '-' ) mul-expr } ;
mul-expr     = tensor-expr { ( '*' | '/' | '%' ) tensor-expr } ;
tensor-expr  = pow-expr { ( 'inner' | 'outer' | 'tensor' | '@' ) pow-expr } ;
pow-expr     = unary-expr { '**' unary-expr } ;             (* right-assoc *)
unary-expr   = ( '-' | '!' | '~' | '<-' | '&' [ 'mut' ] | 'await' ) unary-expr
             | ( 'async' | 'go' ) postfix-expr                  (* operand must be a call f(...) *)
             | postfix-expr ;
postfix-expr = primary { '.' member-name | call-args | '[' index-list ']' | '++' | '--' | '?' } ;
                                                            (* `e?`: the Some/Ok payload, or return the None/Err *)
                                                            (* postfix binds tighter than prefix: !s.f() = !(s.f()) *)
call-args    = '(' [ call-arg { ',' call-arg } ] ')' ;
call-arg     = [ identifier '=' ] expr ;                     (* `name = value`: a named argument; positional ones come first *)
index-list   = index { ',' index } ;
index        = expr | [ expr ] ':' [ expr ] [ ':' expr ] ;  (* Fortran section t[i:j:k], t[:] *)

primary      = literal | identifier | '(' expr ')'
             | 'build' '{' { expr [ ';' ] } '}'
             | 'new' applied-type call-args
             | identifier generic-args '.' identifier [ call-args ]   (* Option<Int>.None: a variant of a generic enum *)
             | lambda | if-expr | match-expr | array-lit | dict-lit | set-lit | latex-math
             | 'this' | 'super' | dsl-entry ;
lambda       = '[' [ capture-list ] ']' '(' [ params ] ')' '->' block ;
array-lit    = '[' [ expr { ',' expr } ] ']' ;
dict-lit     = '{' [ expr ':' expr { ',' expr ':' expr } [ ',' ] ] '}' ;
                                                            (* Dict<K, V>; see below *)
set-lit      = '{' expr ',' [ expr { ',' expr } [ ',' ] ] '}' ;   (* Set<T>; at least one ',' *)
if-expr      = 'if' '(' expr ')' block [ 'else' block ] ;
latex-math   = '$' … '$' | '$$' … '$$' ;                     (* inline LaTeX-math evaluated to a value *)
dsl-entry    = ( 'molecule' | 'cfd' | 'ode' | 'protein' ) … ;
```

A `{` in expression position starts a `dict-lit` when its first top-level item is `key: value`
(a `:` before any `;`, `=`, `->`, or the closing `}`), or when it is an empty `{}` used as a
value (after `=`, `(`, `,`, `:`, `[`, `<-`, or `return`); otherwise it is a block. The literal's
type is `Dict<K, V>`, with `K` and `V` the common types of its keys and values, or the declared
type when it initializes an annotated declaration. In nested type arguments the closing `>>` is
read as two `>`: `Dict<String, Dict<String, Int>>`.

A `{` that is not a `dict-lit` starts a `set-lit` when its top-level items are separated by commas
and no `:`, `;`, `=`, `<-` or `->` appears at the top level (so `{x}` is a block, `{x,}` a set). An
empty `{}` is a set when the declaration or assignment target has type `Set<T>`.

Token-processing sugar is intended for the self-hosted compiler:

```csec
for (text, ordinal <- tokens.between(start, end).withoutTrivia()) { ... }
if (token(tokens, ordinal) is keyword("if")) { ... }
```

The loop lowers to an ordinal range loop and binds `text` to the current token
text.  The pattern expression lowers to the native token-kind/text predicate.

`build { ... }` concatenates its expression segments into a `String`.  A string
literal may embed an identifier as `${name}`; it is equivalent to string
concatenation and is batched with adjacent builder fragments.

### 5.1 `match`

```
match-expr   = expr 'match' '{' { match-arm } '}' ;
match-arm    = 'case' pattern { '|' pattern } [ 'if' expr ] '=>' ( expr | block ) ';' ;
pattern      = '_' | [ '-' ] literal | regex-lit | identifier
             | [ identifier '.' ] identifier [ '(' pattern { ',' pattern } ')' ] ;   (* enum variants *)
```

On an enum value an identifier is a variant name or binds the value; a match on an
enum must handle every variant (also inside nested patterns) unless it has a
`case _`. Patterns joined with `|` bind no variables. A value no case matches stops
the program.

---

## 6. Ownership sigils (summary)

```
move         = '<-' expr ;      (* consumes the named owned value *)
borrow       = '&' expr | '&mut' expr ;   (* temporary reference; auto-derefs on '.', '[]', method call *)
```

Move/borrow are enforced by the type checker (strict by default; see
`docs/memory-model-design.md`).

---

## 7. Statement terminators

**Rule:** a statement is terminated by `;`. The parser is currently *tolerant* —
the terminator is consumed with `match(';')`, not `expect(';')`, so a program may
omit it where the next token unambiguously begins a new statement. New code
should include `;`; the tolerance exists for the REPL-style and generated inputs
and should not be relied upon. Blocks (`{ … }`) do not require a trailing `;`.

---

## 8. Known deviations / open items

- `..` is **not** a range operator in `csec++` (ranges use `to`/`until`); the
  self-host lexer lists `..` but no parser production consumes it.
- Member-access l-values of any depth are first-class `AccessFieldNode` chains
  (`a.b.c = …`, `a.b.c += …`, and reads of the same nest left-to-right). The one
  exception is a `this`/`super` **assignment target**, which still lowers via
  dotted-identifier handling (reads through `this`/`super` use the chain).
- `&&` / `||` are the keywords `and` / `or`.
- The self-hosted compiler (`selfhost/`) lexes the same words and operators (`break`,
  `continue`, `enum`, `&&`/`||` as `and`/`or`, the `f` suffix) but does not implement
  default/named arguments, enums, `match` patterns beyond literals, `?`, interfaces,
  `break`/`continue`, or `-g`.
```
