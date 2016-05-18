" Vim syntax file
" Language:     tin
" Maintainer:   Jack Conger <mail@jpco.io>
" Last Change:  right now

if exists("b:current_syntax")
    finish
endif

" important keywords: set, fn, flow, etc.
syn keyword   tinFlow        if ifx else
syn keyword   tinFlow        cfor while whilex until untilx break continue return
syn keyword   tinFlow        for nextgroup=tinForIdentifier skipwhite skipempty
syn keyword   tinDeclarator  set unset let with nextgroup=tinIdentifier skipwhite skipempty
syn match     tinDeclarator  "^fn" nextgroup=tinIdentifier skipwhite skipempty
syn match     tinIdentifier  "\%([^[:cntrl:][:space:][:punct:][:digit:]]\|_\)\%([^[:cntrl:][:punct:][:space:]]\|_\)*" nextgroup=tinOperator display contained
syn match     tinForIdentifier  "\%([^[:cntrl:][:space:][:punct:][:digit:]]\|_\)\%([^[:cntrl:][:punct:][:space:]]\|_\)*" display contained nextgroup=tinOperator

" general builtins
syn keyword   tinKeyword     test read exit hash

" tinOperator for redirections

" operators
syn keyword   tinOperator    in contained
syn match     tinOperator    "\~"
syn match     tinOperator    "!\="
syn match     tinOperator    "\v\=\=?"

" backslash escape
syn match     tinEscape      contained "\\\(x\x\+\|\o\{1,3}\|.\|$\)"

" need regions: strings, parens, comments (with todos), etc.
" strings
syn region tinDoubleQuote start="\"" end="\"" contains=tinParen,tinEscape
syn region tinSingleQuote start="'" end="'"
" parens
syn region tinParen start="(" end=")" contains=ALLBUT,tinForIdentifier,tinIdentifier

" comments
syn keyword tinTodo TODO FIXME XXX NOTE contained
syn match tinComment "#.*$" contains=tinTodo
syn region tinComment start="^###$" end="^###$" contains=tinTodo

" todo
"

" folding
syn region tinFoldBraces start="{" end="}" transparent fold

let b:current_syntax = "tin"

" links
hi def link tinFlow          Statement 
hi def link tinDeclarator    Type
hi def link tinKeyword       Keyword
hi def link tinTodo          Todo
hi def link tinComment       Comment
hi def link tinForIdentifier Identifier
hi def link tinIdentifier    Identifier
hi def link tinDoubleQuote   String
hi def link tinSingleQuote   Character
hi def link tinOperator      Operator
hi def link tinParen         PreProc
hi def link tinEscape        String
