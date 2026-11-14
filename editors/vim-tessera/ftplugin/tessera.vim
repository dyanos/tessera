" Tessera filetype settings.
if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = 1

setlocal comments=s1:/*,mb:*,ex:*/,:///,://
setlocal commentstring=//\ %s
setlocal formatoptions-=t formatoptions+=croql
setlocal expandtab shiftwidth=4 softtabstop=4 tabstop=4
setlocal suffixesadd=.csec
setlocal path+=lib
setlocal include=^\\s*import\\s\\+

" `gf` on `import std.math.algebra;` / `import "path/file";`: dots become directories.
setlocal includeexpr=substitute(substitute(v:fname,'^\"\\\|\"$','','g'),'\\.','/','g')

if exists('loaded_matchit') || has('nvim')
  let b:match_words = '\<if\>:\<else\>'
endif

let b:undo_ftplugin = 'setlocal comments< commentstring< formatoptions< expandtab< shiftwidth<'
      \ . ' softtabstop< tabstop< suffixesadd< path< include< includeexpr<'
      \ . ' | unlet! b:match_words'
