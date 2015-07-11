#ifndef JPSH_DEFS_H
#define JPSH_DEFS_H

// Definitions for preventing
// magic numbers. These numbers
// are probably too small, to be honest.

// Max line length.
#define MAX_LINE        1000 
// Maximum number of aliases.
#define MAX_ALIASES     100

// Maximum number of variables (at a time).
#define MAX_VARS        1000

// Maximum number of jobs allowed per eval.
#define MAX_JOBS        50

#define MAX_SPLIT       100

// Length of the history.
#define LEN_HIST        400

// Number of separators (defined in defs.c)
// present in jpsh.
#define NUM_SEPARATORS  14

// Maximum length of subshell output.
#define SUBSH_LEN       1000

// Environment variables.
extern char **environ;

// Part of the language definition.
const char separators[NUM_SEPARATORS];

// A small test to determine whether a given
// char is a separator. (Keeps from writing too
// many for() loops!
int is_separator(char c);

// out = (-|~)[fd|&]>[fd|+|*|^]
// e.g.,
// "cmd -&>+ log" appends stdout & stderr to a log
// "cmd -3>* log" prepends whatever is at fd 3 to a log
// "cmd -3>^ log" prepends to the log in reverse order
// "cmd -2>1" pipes all stderr output to stdout
// "cmd ~> parseoutput.sh ~2> logerrs.sh"
//
// in = <([<]-|[fd|&]~)

typedef enum {RD_FIFO, RD_APP, RD_PRE, RD_REV, RD_OW, RD_LIT, RD_RD, RD_REP} rdm;

typedef struct rd_struct {
        int loc_fd;
        rdm mode;       // type of redirection to be done
        int fd;
        char *name;
} rd_t;

#endif
