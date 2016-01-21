#include <stdio.h>

#include "../symtable.h"
#include "builtins.h"

int bi_unset (char **argv, int argc)
{
    int i;
    for (i = 1; argv[i]; i++) {
        int r;
        if ((r = rm_sym (argv[i]))) {
            if (r == 1) {
                fprintf (stderr, "Warning: Could not find symbol '%s'\n", argv[i]);
            } else if (r == 2) {
                fprintf (stderr, "Warning: Symbol '%s' is a binary", argv[i]);
            } else {
                fprintf (stderr, "Warning: Could not delete '%s'\n", argv[i]);
            }
        }
    }

    return 0;
}
