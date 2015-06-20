#ifndef JPSH_DEFS_H
#define JPSH_DEFS_H

#define MAX_LINE        1000
#define MAX_ALIASES     100
#define MAX_VARS        1000
#define MAX_JOBS        50
#define MAX_SPLIT       100
#define LEN_HIST        400
#define NUM_SEPARATORS  14
#define SUBSH_LEN       1000

extern char **environ;
const char separators[NUM_SEPARATORS];
int is_separator(char c);

typedef struct job_struct {
        char **argv;
        int argc;

        int bg;

        int *p_in;
        int *p_out;
        
        struct job_struct *p_prev;
        struct job_struct *p_next;

        char *file_in;
        char *file_out;
} job_t;

#endif
