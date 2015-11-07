#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/linkedlist.h"

#include "ll_utils.h"

char *masked_strchr (char *str, char delim)
{
    int i;

    // mstate & 4 = double quotes
    // mstate & 2 = single quotes
    // mstate & 1 = backslash
    char mstate = 0;

    for (i = 0; str[i]; i++) {
        char ccurr = str[i];
        if (ccurr == delim) {
            if (mstate & 3) goto bs_parse;
            else if (mstate & 4) {
                if (ccurr == ' ' || ccurr == '(') goto bs_parse;
            } else return str + i;
        } else if (ccurr == '\'') {
            if (mstate & ~2) {
                goto bs_parse;
            }

            if (mstate & 2) mstate &= ~2;
            else mstate |= 2;

        } else if (ccurr == '"') {
            if (mstate & ~4) goto bs_parse;

            if (mstate & 4) mstate &= ~4;
            else mstate |= 4;
        } else {

        }

bs_parse:
        if (mstate & 1) mstate &= ~1;
        if (ccurr == '\\') {
            if (!(ccurr & 2)) mstate |= 1;
        }
    }

    return NULL;
}

void word_split (char *in, linkedlist *ll)
{
    char *lindup = in;
    char *indup = in;
    while ((indup = masked_strchr (indup, ' '))) {
        *indup = 0;
        ll_append (ll, strdup (lindup));
        *indup = ' ';
        lindup = ++indup;
    }
    ll_append (ll, strdup (lindup));
}

char **cmd_ll_to_array (linkedlist *ll)
{
    size_t alen = ll_len (ll) + 1;      // for NULL termination
    char **argv = malloc (alen * sizeof (char *));
    argv[alen - 1] = NULL;

    char *arg;
    ll_iter *lli = ll_makeiter (ll);
    size_t i;
    for (i = 0; (arg = (char *)ll_iter_next (lli)); i++) {
        argv[i] = arg;
    }

    free (lli);

    return argv;
}
