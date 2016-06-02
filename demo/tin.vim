" Vim syntax file
" Language:     tin
" Maintainer:   Jack Conger <mail@jpco.io>
" Last Change:  right now

if exists("b:current_syntax")
    finish
endif

" important keywords: set, fn, flow, etc.
syn keyword   tinFlow        if ifx else break continue return match case
syn keyword   tinLoop        cfor while whilex until untilx loop
syn keyword   tinLoop        for nextgroup=tinForIdentifier,tinIdArgument skipwhite
syn keyword   tinDeclarator  set unset let with alias nextgroup=tinIdentifier,tinIdArgument skipwhite
syn match     tinDeclarator  "\v^fn" nextgroup=tinIdentifier,tinIdArgument skipwhite
syn match     tinIdentifier  "\%([^[:cntrl:][:space:][:punct:][:digit:]]\|_\)\%([^[:cntrl:][:punct:][:space:]]\|_\)*" nextgroup=tinOperator  contained
syn match     tinForIdentifier  "\%([^[:cntrl:][:space:][:punct:][:digit:]]\|_\)\%([^[:cntrl:][:punct:][:space:]]\|_\)*"  contained nextgroup=tinOperator

" general builtins
syn keyword   tinKeyword     test read exit hash eval

" operators
" syn keyword   tinOperator    in
syn match     tinOperator    "\~"
syn match     tinOperator    "!\="
syn match     tinOperator    "\v\=\=?"
syn match     tinOperator    "\v\|"
syn match     tinOperator    "\v\|\|"
syn match     tinOperator    "\v\&\&"

" arguments
syn match     tinArgument   "\v\-[^ ]+"
syn match     tinIdArgument "\v\-[^ ]+" contained containedin=tinDeclarator nextgroup=tinIdentifier skipwhite

" tinOperator for redirections
syn match     tinRedirection    "\v\~\>"
syn match     tinRedirection    "\v\<\~"
syn match     tinRedirection    "\v(\d*)\<\<\-"
syn match     tinRedirection    "\v\-\&\>(\+|\d*)"
syn match     tinRedirection    "\v\-(\d*)\>(\+|\d*)"
syn match     tinRedirection    "\v(\d*)\<(\d*)\-"

" backslash escape
syn match     tinEscape      contained "\\\(x\x\+\|\o\{1,3}\|.\|$\)"

" need regions: strings, parens, comments (with todos), etc.
" strings
syn region tinDoubleQuote start="\"" end="\"" contains=tinParen,tinEscape
syn region tinSingleQuote start="'" end="'"
syn region tinRegexQuote  start="`" end="`"
" parens
syn region tinParen start="(" end=")" contains=ALLBUT,tinForIdentifier,tinIdentifier,tinTodo,tinIdArgument
syn region tinVarProc start="\[" end="\]" contained

" comments
syn keyword tinTodo TODO FIXME XXX NOTE contained
syn match tinComment "#.*$" contains=tinTodo
syn region tinComment start="^###$" end="^###$" contains=tinTodo

" folding
syn region tinFoldBraces start="{" end="}" transparent fold

let b:current_syntax = "tin"

" sublinks
hi def link tinIdArgument    tinArgument
hi def link tinRedirection   tinOperator
hi def link tinForIdentifier tinIdentifier

" main links
hi def link tinFlow          Statement 
hi def link tinLoop          Repeat
hi def link tinDeclarator    Type
hi def link tinKeyword       Keyword
hi def link tinTodo          Todo
hi def link tinComment       Comment
hi def link tinIdentifier    Identifier
hi def link tinDoubleQuote   String
hi def link tinRegexQuote    SpecialChar
hi def link tinSingleQuote   Character
hi def link tinOperator      Operator
hi def link tinParen         PreProc
hi def link tinVarProc       Delimeter
hi def link tinEscape        Exception
hi def link tinArgument      Label
