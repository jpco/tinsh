#include <stdlib.h>

#include "builtins.h"

// for freeing everything
#include "../symtable.h"
#include "../types/hashtable.h"
#include "../inter/hist.h"
#include "../inter/term.h"

int bi_exit (char **argv, int argc)
{
    term_exit ();
	free_hist ();

	ht_empty (bintable, free_sym);
	ht_destroy (bintable);

	struct scope *csc;
	struct scope *cscp;
	for (csc = cscope; csc; csc = cscp) {
		ht_empty (csc->symtable, free_sym);
		ht_destroy (csc->symtable);
		cscp = csc->parent;
		free (csc);
	}

    exit (0);
}
