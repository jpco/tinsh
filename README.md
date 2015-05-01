# jpsh
Worse than sh!

### Syntax
 - Builtins:
    - `exit`, `cd`, `pwd`: same as bash
    - `set`, `setenv`: sets an (environment) variable with syntax `set{,env} <name> <value>`
    - `unset`, `unenv`: unsets variables with syntax `un{set,env} <name>`
    - `lsalias`, `lsvars`: list aliases and variables, respectively.
    - `color`: colors the following commands
 - Variables: Both regular and environment variables are referenced in parentheses, like `(home)` expands to `/home/<username>`. To write something in parentheses not to be parsed, `\(home)` works. Unmatched parens are not problematic and do not need to be escaped.
 - Aliases: any command word (a word which is the first argument of a job) which matches an alias will be expanded.

### Done
 - Aliasing
 - (Basic) config
 - Variable parsing, setting, unsetting
 - History

### Todo (in no particular order)
 - Line coloration! (exists as command 'color', not in-buffer)
 - Piping/redirection (have to set up the stdin/stdout alteration)
 - Tab autocompletion
 - Quotes, both the " and ' kind
 - Broaden .jpshrc location support
 - Readline/config support?
 - Generally less-brittle parsing
 - Mom's Old-Fashioned Home-Made Syscall Safety
