#ifndef JPSH_UTIL_DEFS_H
#define JPSH_UTIL_DEFS_H

// Definitions for preventing
// magic numbers. These numbers
// are probably too small, to be honest.

// Max line length.
#define MAX_LINE        1000 

#define MAX_SPLIT       100

// Length of the history.
#define LEN_HIST        400

// Number of separators (defined in defs.c)
// present in tinsh.
#define NUM_SEPARATORS  14

// Maximum length of subshell output.
#define SUBSH_LEN       1000

#define NUM_FLOW_BIS    6

#define NUM_CMD_BIS     11

#define NUM_BUILTINS    (NUM_FLOW_BIS+NUM_CMD_BIS)

#define PIPE_MARKER     ((void *)0xFEFE)

#define builtins (get_builtins())

char **get_builtins();

// Environment variables.
extern char **environ;

// Part of the language definition.
const char separators[NUM_SEPARATORS];

// A small test to determine whether a given
// char is a separator. (Keeps from writing too
// many for() loops!
int is_separator(char c);

#endif
