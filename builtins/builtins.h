#ifndef TINSH_BUILTINS_BUILTINS_H
#define TINSH_BUILTINS_BUILTINS_H

// all builtins must return an int
// and take in a char **argv, int argc

#include "../symtable.h"
#include "../posix/ptypes.h"

#define NUMBUILTINS 1

struct builtin {
    char *name;
    long (*bi_fn)(char **argv, int argc);
};

// BUILTINS MUST TAKE char **argv, int argc AS ARG
// AND RETURN A STATUS int
extern long bi_exit (char **argv, int argc);
extern long bi_cd (char **argv,  int argc);

static struct builtin builtins[];

void builtins_init();
void launch_builtin (process *p, int infile, int outfile, int errfile, int foreground);

#endif
