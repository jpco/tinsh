#ifndef TINSH_BUILTINS_BUILTINS_H
#define TINSH_BUILTINS_BUILTINS_H

#include "../symtable.h"
#include "../posix/ptypes.h"

struct builtin {
    char *name;
    int (*bi_fn)(char **argv, int argc);
};

// BUILTINS MUST TAKE char **argv, int argc AS ARG
// AND RETURN A STATUS int

int bi_exit (char **, int);
int bi_cd (char **, int);
int bi_history (char **, int);

static struct builtin builtins[];

void builtins_init();
void launch_builtin (process *p, int infile, int outfile, int errfile, int foreground);

#endif