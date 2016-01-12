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

// Environment variables.
extern char **environ;

// Part of the language definition.
const char separators[NUM_SEPARATORS];

// A small test to determine whether a given
// char is a separator. (Keeps from writing too
// many for() loops!
int is_separator(char c);

#endif
