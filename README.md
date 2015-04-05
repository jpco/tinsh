# jpsh
Worse than sh!

### Syntax
 - Builtins:
    - `exit`, `cd`, `pwd`: same as bash
    - `environ`: (preferably) identical to env command
    - `set`, `setenv`: sets an environment variable with syntax `set{,env} <name> <value>`
    - `lsalias`, `lsvars`, `lsenv`: list aliases, variables, and environment, respectively (last is hopefully equivalent to `env`).
 - Variables: Both regular and environment variables are referenced in parentheses, like `(home)` expands to `/home/<username>`. To write something in parentheses not to be parsed, `\(home)` works. Unmatched parens are not problematic and do not need to be escaped.
 - Aliases: a word which matches an alias will be expanded.

### Done
 - Aliasing
 - (Basic) config

### Todo (in no particular order)
 - History
 - Variable parsing, setting, unsetting
 - Line coloration! (I have already tried and failed to implement this twice)
 - Piping/redirection (have to set up the stdin/stdout alteration)
 - Tab autocompletion
 - Quotes, both the " and ' kind
 - Broaden .jpshrc location support
 - Generally less-brittle parsing
