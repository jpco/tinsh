# jpsh
Worse than bash! ~~Better than csh!~~

Hopefully this will eventually become a functional (double entendre)
shell language.

### Syntax
_TODO: Rewrite syntax section_

### Done
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
 - string manipulation commands will need to come at some point...
 - move subshelling to after job formationn
 - subtleties
     - masking is not quite right; interactions of ', ", \\
        - and newline! interaction of ' and newline!
     - bash can do `time <aliased cmd>`; how to determine?
        - we know more about builtins than other functions

##### Bugs
 - long buffers don't quite work....
 - multiple subshells in a line don't seem to be working

##### General
 - don't include header files in header files!!!!
 - sort out `char **` vs `const char **` in exec/\* files
 - define func table for increased zoominess
 - get `m_str` iterator fns for `spl_line_eval` and `spl_pipe_eval`
