#ifndef JPSH_DEFS_H
#define JPSH_DEFS_H

#include "eval/stack.h"

// Definitions for preventing
// magic numbers. These numbers
// are probably too small, to be honest.

// Max line length.
#define MAX_LINE        1000 
// Maximum number of aliases.
#define MAX_ALIASES     100

// Maximum number of variables (at a time).
#define MAX_VARS        1000

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

/**
 * The struct describing a job.
 *
 * Fields are
 *  - argv: obvious.
 *  - argc: obvious.
 *  - bg: whether or not the job is to run in the background.
 *  - p_in: the pipe in (not set until the exec phase)
 *  - p_out: the pipe out (not set until the exec phase)
 *  - p_prev: the job piping into this one. Set during eval phase.
 *  - p_next: the job this one pipes into. Set during eval phase.
 *  - file_in: the file to act as stdin.
 *  - file_out: the file to act as stdout.
 *  - job_fn: used during the eval phase.
 */
typedef struct job_struct {
        // the usual argument parameters.
        char **argv;
        char **argm;
        int argc;

        // whether the job is in the background.
        int bg;

        // for piping, the file descriptors in & out
        // used during exec stage
        int *p_in;
        int *p_out;
        
        // for piping, the prev. and next job
        // used during eval stage
        struct job_struct *p_prev;
        struct job_struct *p_next;

        // the file redirections to take place.
        stack *rd_stack;

        // used in the eval process
        void (*job_fn)(struct job_struct *);
} job_t;

#endif
