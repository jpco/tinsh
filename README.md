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
 - config
    - special options `__jpsh_debug` and `__jpsh_color` affect internal behavior! :)
 - Variable parsing, setting, unsetting
 - Tab completion (partial)
    - files/dirs, no commands
 - History

### Todo (in no particular order)
 - Tab completion (more)
    - command autocomplete
    - var/alias autocomplete
    - preserving leading ./ (important for commands like "valgrind ./foo")
    - bash-autocomplete files
 - Line coloration (exists as builtin `color` & upon enter, not in-buffer)
    - enable/disable with a var
 - Piping/redirection (have to set up the stdin/stdout alteration)
    - better support for multiple commands on one line (both `;` and `|`)
 - [^n]
 - Quotes, both the " and ' kind
 - Broaden .jpshrc location support
 - Readline/config support?
 - Generally less-brittle parsing
