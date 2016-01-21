#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../symtable.h"
#include "builtins.h"
#include "../util/defs.h"

int bi_set (char **argv, int argc)
{
    /*

TODO modifiers:
local = cscope
global = gscope
strong = strong var (does not need parens to be devar'd) (useful for defining fun new syntax)
env = environment var (conflicts with all others)

     */
    // 0 = default
    // 1 = local
    // 2 = global
    // 3 = env
    int scope = 0;
    char *names[MAX_LINE] = { 0 };
    char **cname = names;
    int on_val = 0;
    int i;
    for (i = 1; argv[i]; i++) {
        char *curr = argv[i];
        if (!on_val) {
            if (!strcmp (curr, "=")) {
                on_val = 1;
            } else if (!strcmp (curr, "env")) {
                scope = 3;
            } else {
                *cname++ = curr;
            }
        } else {
            if (!names[0]) {
                fprintf (stderr, "Invalid var assignment syntax (no name(s) provided)\n");
                return -1;
            }
            char **name;
            for (name = names; *name; name++) {
                if (scope == 3) {
                    setenv (*name, curr, 1);
                } else {
                    add_sym (*name, strdup(curr), SYM_VAR);
                }
            }
        }
    }

    return 0;
}
