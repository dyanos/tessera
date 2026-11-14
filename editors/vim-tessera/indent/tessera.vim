" Tessera indentation: C-style braces, plus continuation of contract clauses
" (`requires` / `ensures` / `invariant` / `decreases` lines between a signature and its body).
if exists('b:did_indent')
  finish
endif
let b:did_indent = 1

setlocal cindent
setlocal cinoptions=L0,(s,Ws,m1,j1,J1
setlocal cinkeys-=0#
setlocal indentexpr=TesseraIndent()
setlocal indentkeys=0{,0},0),0],!^F,o,O,e,=requires,=ensures,=invariant,=decreases

let b:undo_indent = 'setlocal cindent< cinoptions< cinkeys< indentexpr< indentkeys<'

if exists('*TesseraIndent')
  finish
endif

let s:contract = '^\s*\%(requires\|ensures\|invariant\|decreases\)\>'

function! TesseraIndent() abort
  let line = getline(v:lnum)
  let prevnum = prevnonblank(v:lnum - 1)
  if prevnum == 0
    return 0
  endif
  let prev = getline(prevnum)
  " Contract clause after a signature / loop header (or after another clause): one level deeper
  " than the declaration, and the body brace returns to the declaration's level.
  if line =~# s:contract
    if prev =~# s:contract
      return indent(prevnum)
    endif
    return indent(prevnum) + shiftwidth()
  endif
  if prev =~# s:contract && line =~# '^\s*{'
    let n = prevnum
    while n > 1 && getline(n) =~# s:contract
      let n = prevnonblank(n - 1)
    endwhile
    return indent(n)
  endif
  return cindent(v:lnum)
endfunction
