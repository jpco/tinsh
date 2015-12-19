#include <stdlib.h>

#include "builtins.h"

// for freeing everything
#include "../symtable.h"

long bi_exit (char **argv, int argc)
{
	ht_empty (bintable, free_sym);
    exit (0);
}
