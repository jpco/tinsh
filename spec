# The Tin Shell

## 1. DESIGN GOALS
 - semantic meaning and syntactic meaning should be related.  this is a huge weakness
   in POSIX and POSIX-compatible shells.
    - the correct way to do something should be the easiest or most obvious way to do
      it.  the most common way something is done should be the automatic way to do it.
        - example: field splitting on variable resolution in bash. why is it automatic
          if it's usually encountered as a "gotcha"? why must we use extra syntax to
          *not* do something? this is simply bad design.
        - example: ["always use `read -r`"](http://wiki.bash-hackers.org/commands/builtin/read#read_without_-r)
    - spending a little more time typing can save a lot of time reading.  (some things
      can be a little wordy)
    - example: `for x in (y)` is a semantically powerful statement, which should apply
      to most anything which could be regarded as a list. but in bash it is bad practice
      to use this construct over lines in a file, for implementation-related reasons.
 - insofar as it does not conflict with the previous point, having fewer more-powerful
   language constructs is better than having more weaker constructs.
    - example: the tinsh block construct subsumes many disparate syntaxes/behaviors in
      bash (if/fi? do/done? function x { }?)
 - make things as extensible and customizable as possible.  A small, extensible core system
   & language (touching on the previous design goal) enabling powerful and useful in-language
   (function/alias) constructs, which can be customized or extended by those brave enough
   to do so, is the best of all worlds.
 - Have Sane Defaults.  While fish is against configurability in general, 

## ARGUMENTS
tinsh [-c configfile] [-iln] [-d [N]] [-e command|tinfile]
 -i (force interactivity)
 -l (force login shell)
 -n (do not execute)
 -d (debug level; default 1)
 -e (execute given command)
 -c (read alternate config)

## SYNTAX
 - commenting: '#' = line comment, a '###' line delimits block comments
 - piping: the same as we all know it
    - with one twist: the common gotcha in bash "echo 'foo' | read bar; echo $bar"
      echoes "".  in tinsh the analogous construct echoes "foo", as expected.
 - variable resolution: quite different
    - resolution includes variable resolution as well as subshells
    - all variables, aliases, fns, binaries, statements, etc. caught in parens
        `$ echo (foo); vim (which program.py)`
    - nested variables are fine; resolved inside-out
        `$ echo (parse (opt1) (opt2))`
    - variable inspection: `(val?)`
        - gives you a variable's type and (immediate) value
        - example: `set -a ls = ls --color=auto; echo (ls)` will echo the output of
          `ls --color=auto`, but `echo (ls?)` will output `alias ls --color=auto`.
        - the first token of `(foo?)` is *always* the type it will be resolved as.
    - variable processing:
        - slicing Python-style: (var[2:4]), (var[:-2]), (var[::-1])
        - more powerful constructs (substrings, etc.,) handled by the 'str' builtin.
    - by default vars don't get split, captured commands do;
      switch this behavior with an exclamation, i.e.,
        `$ let args = "-l --color=auto"; ls (args!)`
 - blocks: C-like
    - single-line blocks declared with ':', arbitrary-line with '{ }'
        $ if (foo) == bar: ls
        $ if ! (ls -A) { echo "empty!" } else { echo "not!" }
    - line-break before block declarations is NOT valid, since line breaks have
      semantic meaning!
    - if you do something like "ls { one; two; three }", tinsh will rewrite
        the command such that each *line* of the block (trimmed) is an argument for
        the binary
    - blockless builtins (only certain commands allow for this) (may delete?)
        $ if (foo) == bar, ls
    - tinsh is block-scoped: local variables first defined in block will be deleted upon leaving block
        - however, 'set foo = bar; { set foo = baz; } echo (foo)' will echo 'baz'
        - if a variable is set, by default, resetting it will change the outer variable's value
            - 'set foo = bar; : set -l foo = baz; echo (foo)' will mask the variable
 - globs
    - '*' as we know it
    - what in bash is 'foo.{a,b}' is in tinsh 'foo.[a,b]'
 - quoting
    - like we all know it (hold on is this true?)
 - redirection
    - output redirection: (~>|-(fd|&)?>(fd|+)?)
        - if we have ~>, this redirect behaves like process substitution; the next
          token is parsed as a command and is replaced with the path of a file
          descriptor to that command open for writing.
        - if no fd is given before '>', stdout is assumed;
          if & is used, both stdout and stderr are used
        - if fd specified after '>' char, first fd is dup2ed into second;
          otherwise, the next token is used as a filename. if + is used, appends
          to the file.
    - input redirection: `(fd)?<<?((fd)?-|~)`
        - if we have `<~`, the redirect behaves like process substitution
        - if we have `(fd)?<(fd)?-`, the redirect behaves as normal; if (fd) is omitted
          in the former place, stdin is assumed. if it is omitted in the latter place,
          the next token is used as a filename.
        - if `<<-` is given, the next token is used as literal input (analogous to
          a here string).  If `<<- {a\nb\nc}` is used, is analogous to a here document.
 - job control
    - (basically) POSIX compliant.

## BUILTINS
 - cd
 - fn
    - syntax: `fn <arg1> <arg2> <list_arg> ... <last_arg> { cmd \n cmd (arg1) \n cmd }`
        `$ fn foo - this that: ls (this) (that)`
    - technically syntactic sugar for
        `$ set foo = fn - this, that: ls (this) (that)`
 - flow control
    - if/else
        - test equality/etc
        - 'if/else (test) [,:{] action'
        - $ set if = 'ifx test'
    - if/else
        - test success of command
        - 'ifx/else command [,:{] action'
    - while/whilex
        - $ set until = 'while not'
        - $ set untilx = 'whilex not'
    - for
    - cfor
    - with
    - break [N]/continue [N]
    - return [N]
 - set/unset
    - fn set
    - arithmetic set
 - read
    - -l and -w args for word-based input and line-based input
    - -a reads everything possible until EOF
    - read -l is the default
    - argument can be supplied to read as input (read foo.txt)
 - exec(/eval?)
 - exit [N]
 - hash
 - test
    - string emptiness
        - isempty / nonempty
    - string comparison
        - '==', '!='
    - pathiness
        - ispath
        - isfile
        - isdir
        - isregular
        - islink
        - issymlink
        - ishardlink
    - intiness
        - isint
        - '<', '<=', '>', '>='
 - str
    - powers variable mangling in resolution
    - does many things:
        - str sub: slice a string
        - str match: position of one string in another
        - str pre: find (or remove) prefix in string
        - str suf: find (or remove) suffix in string
        - str repl: replace a substring with another
 - trap
 - umask
 - coproc
 - async_chain: run a block asyncronously; on completion, run a second block
    (perhaps could be as many blocks as we care to do)
    $ async_chain { update_quietly --noconfirm } { notify-send "finished update." }
 
## FAILURE
 - there are three main kinds of failure in Rust: I/O failure, syntax failure, and command failure.
    - I/O are reported as 'errors', and are fatal by default.
    - syntax failures are reported as 'warnings', and are usually handled as best as possible. They become fatal with the 'ewarning' option.
    - when commands fail (i.e., exit with a nonzero status), the shell itself does not, by default, care.  They become fatal with the 'estatus' option.


## CONFIGURATION
Config options: (all options accessable as var __tin_<value>, unless specified
                 otherwise)
 - inter:    interactive or not (readonly after startup)
 - login:    login shell or not (readonly after startup)
 - noexec:   whether we actually execute commands (readonly after startup)
 - debug:    debug output level
 - config:   as __config, config file (if extant) (readonly after startup)
 - filename: if running from file, name of file (readonly after startup)
 - command:  value of '-e' arg (readonly after startup)
 - argnum:   enforce number of arguments that a builtin or function takes.
             we can't enforce this with binaries, (or can we?) but that's fine.
 - split:    swaps the default splitting behavior and the behavior of '!' in variable
             resolution.
 - estatus:  if a command exits with a nonzero status, panic
 - ewarning: if a syntax warning occurs, panic
 - pipefail: if any command in a pipe chain fails, assume that error code
             (default is to assume the error code of the last command)
 - safe:     combination of 'argnum', 'estatus', 'ewarning', 'pipefail'

## FEATURES (& sources, lol)
 - look up ksh, fish, zsh, bash, tcsh
 - rust syntax
 - ion in the redox OS
