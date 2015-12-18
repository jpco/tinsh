#include <stdio.h>
#include <pthread.h>

#include "builtins.h"
#include "../types/hashtable.h"
#include "../symtable.h"
#include "../posix/ptypes.h"

static struct builtin builtins[NUMBUILTINS] = 
{
    { "exit", bi_exit },
    // { "cd", bi_cd }
};

void builtins_init (void)
{
    int i;
    for (i = 0; i < NUMBUILTINS; i++) {
        fprintf (stderr, "adding %s\n", builtins[i].name);

        add_sym (builtins[i].name, builtins[i].bi_fn, SYM_BUILTIN);
    }
}

void *launch_bi_fork (void *pvoid)
{
    process *p = (process *)pvoid;
	int argc;
	for (argc = 0; p->argv[argc]; argc++);

	// ew
    return (void *)(((int (*)(char **, int))p->wh_exec->value) (p->argv, argc));
}

void launch_builtin (process *p, int infile, int outfile, int errfile, int foreground)
{
    // fork new thread at subthread entry point function
    pthread_t bi_thread;
    pthread_create (&bi_thread, 0, launch_bi_fork, p);
    // what to do with foreground? (for now assume everything is fg)
}
