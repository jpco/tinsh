#ifndef JPSH_DEFS_H
#define JPSH_DEFS_H

#define MAX_LINE        1000
#define MAX_ALIASES     100
#define MAX_VARS        1000
#define MAX_JOBS        20
#define MAX_SPLIT       100
#define LEN_HIST        400
#define NUM_SEPARATORS  7

extern char **environ;
const char separators[NUM_SEPARATORS];
int is_separator(char c);

#endif
