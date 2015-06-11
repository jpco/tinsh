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
    - `__jpsh_debug` toggles displaying the evaluated command
    - `__jpsh_~home` decides what `~` refers to
 - Variable parsing, setting, unsetting
 - Tab completion (mostly)
    - files/dirs
    - commands
 - Kind-of coloration (though not in-buffer!)
    - can be toggled with `__jpsh_color` var
 - multiple lines in one line
 - History
 - a(n as-of-yet unused) notion of scope for variables
    - environment variables and aliases always have global scope

### Todo (in no particular order)
CURRENT: EVAL REFACTOR

##### Interactive
 - Tab completion (more/better)
    - var/alias autocomplete? Esp. `__jpsh_~home`
    - deal with s/dquotes
    - deal with parens
    - bash autocomplete files
    - suggestion on conflict >>>> PREVIEW FUNCTION
    - refactor... the code is pretty messy
 - Redo line coloration... that didn't last long
 - Preview functionality!
 - Long-buffer support
 - `^` / `^^n`
 - `__jpsh_prompt`
 - Broaden .jpshrc location support
 - Readline/config support? (maybe long-term goal)

##### Non-interactive
 - `:` / `{ }` support, then we can start with control flow \& `with`
 - `int`, `bool`, and `path` vars
 - Piping/redirection (have to set up the stdin/stdout alteration)
    - (`pipe(3)` and `dup2(3)`)
 - subshells! (after piping)
 - Globs! (interactive >>>> PREVIEW FUNCTION)
 - Generally less-brittle parsing (things are improving!)
