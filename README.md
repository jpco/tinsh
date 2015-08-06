# tinsh
Worse than bash! ~~Better than csh!~~

Hopefully this will eventually become a functional (double entendre)
shell language.

### Syntax
_TODO: Rewrite syntax section. It's changed pretty significantly lately._

### Todo (in no particular order)
 - Beef up testing (type hinting) and finish flow control
 - functions

##### Redo
 - Tab completion (after variable rejiggery)
    - var autocomplete, Esp. regarding ~
    - correctly deal with s/dquotes
    - correctly deal with parens
    - bash autocomplete files (and man pages?!)
    - suggestion on conflict >>>> PREVIEW FUNCTION
    - clean up... the code is pretty messy
 - coloration (after variable rejiggery)
    - in-buffer

##### Interactive
 - Preview functionality!
 - history improvements
     - `^` / `^^n` (use `hist_get()`)
     - ctrl+r search?
     - save & reload across sessions
 - Readline/config support? (maybe)

##### Non-interactive
 - redirection modes `->*`, `->^`, `~>` (and `<~`, and `<<-`)
    - goal: `(-|~)(fd|&)?>(fd|+|*|^)?` and `<<?(-|~)`
 - `:` / `{ }` support, then we can start with control flow \& `with`
    - goal: `with`, `if`, `else`, `while`, `for`, `cfor`, `fn`
 - (light) type system: `bool`, `int`, and `path` (or at least the last two)
 - Globs! (interactive >>>> PREVIEW FUNCTION)
    - this is connected to compl.c so don't re-invent the wheel
 - `job->bg` is deactivated because background job handling needs to be
    more robust than just forgetting about the job after the fact
 - string manipulation commands will need to come at some point...
 - subtleties
     - masking and newline
     - alias some cmds after first word (like bash's `time foo`)

##### General
 - sort out `<type> **` vs `const <type> **` in exec/\* files
 - define func table for increased zoominess
 - get native `m_str` functionality for the things that need it
