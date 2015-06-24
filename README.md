# jpsh
Worse than bash! ~~Better than csh!~~

_guided by the need to be at least okay_
(we gotta beat [http://harmful.cat-v.org/software/csh](csh) at least!)

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
 - History
 - a(n as-of-yet unused) notion of scope for variables
    - environment variables and aliases always have global scope
 - subshells (kind of broken)
 - Input/output redirection
    - `->` is basic stdout-to-file. `-2>` is stderr-to-file, and `-&>` is both.
    - `-2>1` and `-1>2` (or equivalently, `->2`) map stderr and stdout to each other.
    - Need not use 1 and 2 explicitly; other file descriptors, if they work in the system, will work.
    - `->+` appends to a file.

### Todo (in no particular order)
CURRENT: memory errors on subshells!
 - e.g., try 'echo `foo`'

##### Interactive
 - Tab completion (more/better)
    - var/alias autocomplete? Esp. `__jpsh_~home`
    - deal with s/dquotes
    - deal with parens
    - bash autocomplete files
    - suggestion on conflict >>>> PREVIEW FUNCTION
    - clean up... the code is pretty messy
 - Redo line coloration... that didn't last long
 - Preview functionality!
 - `^` / `^^n`
 - `__jpsh_prompt`
 - Broaden .jpshrc location support
 - Readline/config support? (maybe long-term goal)

##### Non-interactive
 - redirection modes `->*`, `->^`, `~>`
 - `:` / `{ }` support, then we can start with control flow \& `with`
    - goal: `with`, `if`, `else`, `while`, `for`, `cfor`, `fn`
 - `int`, `bool`, and `path` vars
 - Globs! (interactive >>>> PREVIEW FUNCTION)
    - this is connected to compl.c so don't re-invent the wheel
 - `job->bg` is deactivated because background job handling needs to be
    more robust than just forgetting about the job after the fact
 - builtins aren't right. those things need to be rejiggered before flow
    can happen.
 - string manipulation will need to come at some point soon

 - subtleties
     - masking is not quite right; interactions of ', ", \\
        - and newline! interaction of ' and newline!
     - bash can do `time <aliased cmd>`; how to determine?

##### Bugs
 - long buffers don't quite work....
 - subshells are pretty broken.
