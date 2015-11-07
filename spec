# The Tin Shell

## ARGUMENTS
tinsh [-c configfile] [-iln] [-d [N]] [-e command|tinfile]
 -i (force interactivity)
 -l (force login shell)
 -n (do not execute)
 -d (debug level; default 1)
 -e (execute given command)
 -c (read alternate config)

## PROCESS
per-command:
 - read command
 - word splitting
 - strong var/alias resolution
 - command splitting/block gathering
 - var resolution/shell expansion
 - redirection
 - execution
 - waiting/gathering (optional)

## SYNTAX
 - commenting: '#' = line comment, '###' delimits block comments
 - piping: the same as we all know it
 - variable resolution: not the same
    - all variables, fns, binaries, statements, etc. caught in parens
        $ echo (foo); vim (which program.py)
    - nested variables are fine; executed outside-in
        $ echo (parse (opt1) (opt2))
 - blocks: vaguely C-like
    - single-line blocks declared with ':', arbitrary-line with '{ }'
        $ if (foo) == bar: ls
        $ if not (ls -A) { echo "empty!" } else { echo "not!" }
    - line-break before block declarations is valid, as follows:
        $ if (foo) == bar
            : ls
        $ if not (ls -A)
          {
              echo "empty!"
          }
          else
          {
              echo "not!"
          }
    - blockless commands
        $ if (foo) == bar, ls
        $ if (foo) == bar
          ls
    - tinsh is block-scoped: local variables first defined in block will be deleted upon leaving block
        - however, 'set foo = bar; : set foo = baz; echo (foo)' will echo 'baz'
        - if a variable is set, by default, resetting it will change the outer variable's value
            - 'set foo = bar; : set -l foo = baz; echo (foo)' will mask the variable like in Rust
 - quoting
 - redirection
 - job control
    - (basically) POSIX compliant
        - dont do '&' on things like 'cd' though because thats just silly and will be ignored

## BUILTINS
 - cd
 - fn
    $ fn foo - this, that: ls (this) (that)
    $ set foo = (fn - this, that: ls (this) (that))
 - flow control
    - if/else
        - 'if/else [,:{\n] action'
    - while
        - $ set until = 'while not'
    - for
    - cfor
    - with
    - break [N]/continue [N]
    - return [N]
 - set/unset
 - subsh
 - exec(/eval?)
 - exit [N]
 - hash
 - pwd
 - test
    - command success
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
 - trap
 - umask
 - coproc
 - ?, $, ., F
    - ? query a symbol's value
    - $ subshell
    - . run in this shell
    - F run as function


## CONFIGURATION

## FEATURES (& sources, lol)
 - look up ksh, fish, zsh, bash, tcsh
