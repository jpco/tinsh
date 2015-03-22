# jpsh
Worse than sh!

## Syntax
 - Builtins:
    - `exit`, `cd`, `pwd`: same as bash
    - `environ`: (preferably) identical to env command
    - `setenv`: sets an environment variable
    - `lsalias`, `lsvars`, `lsenv`: list aliases, variables, and environment, respectively (last is hopefully equivalent to `env`).
 - Variables: in parentheses, like `(home)` expands to `/home/<username>`. To write something in parentheses not to be parsed, `\(home)` works. Unmatched parens are not problematic and do not need to be escaped.
 - Aliases: a word which matches an alias will be expanded.

## Done
 - Aliasing
 - (Basic) config ([exec] section not implemented)

## Todo
 - History is kinda busted
 - Line coloration! (I have already tried and failed to implement this twice)
 - Finish variable parsing
 - Piping/redirection
