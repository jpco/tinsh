# jpsh
Worse than bash! ~~Better than csh!~~

_guided by the need to be at least okay_

(we gotta beat [csh](http://harmful.cat-v.org/software/csh) at least!)

### Syntax
 - Builtins:
    - `exit`, `cd`, `pwd`: same as bash
    - `set`, `setenv`: sets an (environment) variable with syntax `set{,env} <name> <value>`
    - `unset`, `unenv`: unsets variables with syntax `un{set,env} <name>`
    - `lsalias`, `lsvars`: list aliases and variables, respectively.
    - `color`: colors the following commands
 - Variables: Both regular and environment variables are referenced in parentheses, like `(home)` expands to `/home/<username>`. To write something in parentheses not to be parsed, `\(home)` works. Unmatched parens are not problematic and do not need to be escaped.
 - Aliases: any command word (a word which is the first argument of a job) which matches an alias will be expanded.
 - Redirection:
    - output redirection is given by `-[fd|&]>[fd|+]`
         - the left area indicates the source fd, while the right one
           indicates the destination. a `&` in the left spot indicates
           moving both stdout and stderr. If the left part is absent,
           defaults to stdout.
         - the right area indicates the destination fd in the case of a
           redirect (like sending stderr to stdout, `-2>1`); otherwise, it 
           outputs to a file given in the next word, appending if `+` is
           present, overwriting
           otherwise.
    - Input redirection is given by `<-` so far. In to stdin. Not interesting.
 - Default arguments. Running a jpsh script, the file name of the script
   is given by `(_)` and the arguments passed to the script are in `(_%d)`
   where `%d` is the index of the argument (one-indexed). `(_n)` is the
   number of args.

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
    - `-(fd|&)?>(fd|+)?` and `<-`

### Todo (in no particular order)
CURRENT:
 - error code checking & propagation! (MALLOC)
 - `hist_get()`

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
 - `^` / `^^n` (use `hist_get()`)
 - `__jpsh_prompt`
 - Broaden .jpshrc location support
 - Readline/config support? (maybe long-term goal)

##### Non-interactive
 - redirection modes `->*`, `->^`, `~>` (and `<~`, and `<<-`)
    - goal: `(-|~)(fd|&)?>(fd|+|*|^)?` and `<<?(-|~)`
 - `:` / `{ }` support, then we can start with control flow \& `with`
    - goal: `with`, `if`, `else`, `while`, `for`, `cfor`, `fn`
 - `int`, `bool`, and `path` vars
 - Globs! (interactive >>>> PREVIEW FUNCTION)
    - this is connected to compl.c so don't re-invent the wheel
 - `job->bg` is deactivated because background job handling needs to be
    more robust than just forgetting about the job after the fact
 - builtins aren't right. those things need to be rejiggered to support
    redirection before flow control can happen.
 - string manipulation will need to come at some point...
 - move subshelling to after job formationn
 - subtleties
     - masking is not quite right; interactions of ', ", \\
        - and newline! interaction of ' and newline!
     - bash can do `time <aliased cmd>`; how to determine?

##### Bugs
 - long buffers don't quite work....
 - multiple subshells in a line don't seem to be working
 - segfault on `(not_set_var)`
