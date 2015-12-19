#include <stdio.h>
#include <pthread.h>

#include "builtins.h"
#include "../types/hashtable.h"
#include "../symtable.h"
#include "../posix/ptypes.h"

#define NUMBUILTINS 3

static struct builtin builtins[NUMBUILTINS] = 
{
    { "exit", bi_exit },
    { "cd", bi_cd },
	{ "history", bi_history }
};

void builtins_init (void)
{
    int i;
    for (i = 0; i < NUMBUILTINS; i++) {
        add_sym (builtins[i].name, builtins[i].bi_fn, SYM_BUILTIN);
    }
}

int launch_bi (void *pvoid)
{
    process *p = (process *)pvoid;
	int argc;
	for (argc = 0; p->argv[argc]; argc++);

	// ew
    return (((int (*)(char **, int))p->wh_exec->value) (p->argv, argc));
}

void launch_builtin (process *p, int infile, int outfile, int errfile, int foreground)
{
    // I don't think builtins *have* background
	// anything that might shouldn't be a builtin
	launch_bi (p);
}