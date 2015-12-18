#ifndef TINSH_BUILTINS_BUILTINS_H
#define TINSH_BUILTINS_BUILTINS_H

// all builtins must return an int
// and take in a char **argv, int argc

#include "../symtable.h"
#include "../posix/ptypes.h"

#define NUMBUILTINS 1

struct builtin {
    char *name;
    int (*bi_fn)(char **argv, int argc);
};

// BUILTINS MUST TAKE A process * AS ARG
// AND RETURN A STATUS INT CAST TO VOID *
extern int bi_exit (char **argv, int argc);

static struct builtin builtins[];

void builtins_init();
void launch_builtin (process *p, int infile, int outfile, int errfile, int foreground);

#endif
