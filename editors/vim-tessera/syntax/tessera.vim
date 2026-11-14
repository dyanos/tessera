" Vim syntax file
" Language:    Tessera (.csec)
" Maintainer:  Tessera contributors
" Keep in sync with editors/vscode-tessera/syntaxes/tessera.tmLanguage.json and
" editors/openidea-tessera/crates/tessera-syntax/src/{lexer,highlight}.rs.

if exists('b:current_syntax')
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn case match
syn iskeyword @,48-57,_,192-255

" Vim gives a later :syn match/region priority over an earlier one starting at the same column
" (keywords beat both), so the generic items come first.
syn match   tesseraOperator     /=>\|->\|<-\|>>|\|<<|\|\*\*\|[=!<>]=\|<<\|>>\|[-+*/%]=\|++\|--\|<:\|<%\|>:\|[=+\-*/%&|^~!<>?]/ display
syn match   tesseraFuncCall     /\<\l\w*\ze\s*(/ display
syn match   tesseraTypeName     /\<\u\w*\>/ display

" ---- keywords (lexer KEYWORDS) ----
syn keyword tesseraConditional  if else match case
syn keyword tesseraRepeat       for while
syn keyword tesseraStatement    return import build
syn keyword tesseraDeclaration  def val var class struct object extends override operator
syn keyword tesseraDeclaration  template typename external inline constexpr
syn keyword tesseraModifier     mut box unsafe unatomic
syn keyword tesseraConcurrency  async await go
syn keyword tesseraWordOperator and or xor to until new
syn keyword tesseraComprehension map pmap reduce preduce filter
syn keyword tesseraDslEntry     molecule cfd ode protein
syn keyword tesseraSelf         this super
syn keyword tesseraBoolean      true false
syn keyword tesseraConstant     null

" Contextual words.
syn keyword tesseraInfixWord    inner outer union intersect diff implies step is
syn keyword tesseraPolicy       cpu simd openmp gpu contained
syn match   tesseraPolicyList   /\%(\<p\%(map\|reduce\)\s*\)\@<=(\s*\%(cpu\|simd\|openmp\|gpu\)\%(\s*|\s*\%(cpu\|simd\|openmp\|gpu\)\)*/ contains=tesseraComprehension,tesseraPolicy,tesseraOperator
syn keyword tesseraDslWord      atom bond at steps dt temperature grid viscosity velocity euler from lattice spacing chain mcmc contained
syn region  tesseraDslBlock     matchgroup=tesseraDelimiter start=/\<\%(molecule\|cfd\|ode\|protein\)\>[^{;]*\zs{/ end=/}/ transparent contains=TOP,tesseraDslBlock,@tesseraDslItems
syn cluster tesseraDslItems     contains=tesseraDslWord

" ---- contracts (LANGUAGE_SPEC.md 12): `requires x > 0 ensures result >= x` ----
" A clause runs from its keyword to the body: `{`, a lone `=`, or `;`. `result` is the return value.
syn region  tesseraContract     matchgroup=tesseraContractKeyword start=/\.\@<!\<\%(requires\|ensures\|invariant\|decreases\)\>\%(\s*[=:,);]\)\@!/ end=/\ze\%({\|;\|[=!<>]\@<!=[=>]\@!\|\<\%(requires\|ensures\|invariant\|decreases\)\>\)/ contains=TOP
syn keyword tesseraResult       result contained containedin=tesseraContract

" ---- types ----
syn keyword tesseraType         Int Long Short Byte Float Double Real Bool Boolean Char String
syn keyword tesseraType         Nat Integer Natural Unit Void Tensor Vector Matrix Array Dict Set
syn keyword tesseraType         Shared Weak Regex Future
syn keyword tesseraBuiltin      System Console Math IO File

" ---- declarations ----
syn match   tesseraFuncDecl     /\%(\<def\s\+\)\@<=\%(operator\>\)\@!\h\w*/ display
syn match   tesseraTypeDecl     /\%(\<\%(class\|struct\|object\|typename\|molecule\|cfd\|ode\|protein\)\s\+\)\@<=\h\w*/ display
syn match   tesseraOperatorDecl /\%(\<def\s\+operator\s\+\)\@<=[^ \t(]\+/ display

" ---- literals ----
syn match   tesseraNumber       /\<0[xX]\x\+\>/ display
syn match   tesseraNumber       /\<0[bB][01]\+\>/ display
syn match   tesseraNumber       /\<0[oO]\o\+\>/ display
syn match   tesseraNumber       /\<\d\+\%(\.\d*\)\=\%([eE][+-]\=\d\+\)\=\>/ display
syn match   tesseraNumber       /\.\@<!\.\d\+\%([eE][+-]\=\d\+\)\=\>/ display

syn match   tesseraEscape       /\\\%([0abfnrtv"'\\$]\|x\x\{2}\|u\x\{4}\|U\x\{8}\)/ contained display
syn region  tesseraInterp       matchgroup=tesseraInterpDelim start=/\${/ end=/}/ contained contains=TOP
syn region  tesseraString       start=/\%(\<[uU]\)\="/ skip=/\\\\\|\\"/ end=/"/ contains=tesseraEscape,tesseraInterp,@Spell
syn region  tesseraRawString    start=/\<[rR]\ze"/ end=/"/
syn match   tesseraChar         /'\%(\\.\|[^\\']\)'/ contains=tesseraEscape

" ---- comments ----
syn keyword tesseraTodo         TODO FIXME XXX NOTE contained
syn region  tesseraComment      start=+//+ end=/$/ contains=tesseraTodo,@Spell
syn region  tesseraDocComment   start=+///\%(/\)\@!+ end=/$/ contains=tesseraTodo,@Spell
syn region  tesseraComment      start=+/\*+ end=+\*/+ contains=tesseraTodo,@Spell
syn region  tesseraDocComment   start=+/\*\*\%(/\)\@!+ end=+\*/+ contains=tesseraTodo,@Spell

" ---- attributes, operators, LaTeX math ----
syn match   tesseraAttribute    /\[@\h\w*/ display
syn region  tesseraMath         matchgroup=tesseraMathDelim start=/\$\$\=/ end=/\$\$\=/ contains=tesseraMacro,tesseraNumber,tesseraOperator
syn match   tesseraMacro        /\\\a\+/ contained display

" ---- highlight links ----
hi def link tesseraConditional    Conditional
hi def link tesseraRepeat         Repeat
hi def link tesseraStatement      Statement
hi def link tesseraDeclaration    Keyword
hi def link tesseraModifier       StorageClass
hi def link tesseraConcurrency    Keyword
hi def link tesseraWordOperator   Operator
hi def link tesseraComprehension  Keyword
hi def link tesseraDslEntry       Structure
hi def link tesseraDslWord        Keyword
hi def link tesseraInfixWord      Operator
hi def link tesseraPolicy         Keyword
hi def link tesseraSelf           Special
hi def link tesseraBoolean        Boolean
hi def link tesseraConstant       Constant
hi def link tesseraContractKeyword PreProc
hi def link tesseraResult         Special
hi def link tesseraType           Type
hi def link tesseraTypeName       Type
hi def link tesseraBuiltin        Identifier
hi def link tesseraFuncDecl       Function
hi def link tesseraOperatorDecl   Function
hi def link tesseraTypeDecl       Typedef
hi def link tesseraFuncCall       Function
hi def link tesseraNumber         Number
hi def link tesseraString         String
hi def link tesseraRawString      String
hi def link tesseraChar           Character
hi def link tesseraEscape         SpecialChar
hi def link tesseraInterpDelim    Delimiter
hi def link tesseraDelimiter      Delimiter
hi def link tesseraTodo           Todo
hi def link tesseraComment        Comment
hi def link tesseraDocComment     SpecialComment
hi def link tesseraAttribute      PreProc
hi def link tesseraOperator       Operator
hi def link tesseraMathDelim      Delimiter
hi def link tesseraMath           String
hi def link tesseraMacro          Macro

let b:current_syntax = 'tessera'

let &cpo = s:cpo_save
unlet s:cpo_save
