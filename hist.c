#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// local includes
#include "defs.h"

// self-include
#include "hist.h"

// History is implemented as a cyclic array.
// We assume LEN_HIST < UINT_MAX.
static char *hist[LEN_HIST];
static unsigned int len;
static unsigned int base_pos;

// if pos < len, we are "in history"
// if pos = len, we are below history
static unsigned int pos;

void hist_add (char *line)
{
        if (line == "") return;
        char *hline = malloc ((strlen(line)+1) * sizeof(char));
        strcpy (hline, line);

        if (len == LEN_HIST) {  // full history
                int nbase_pos = base_pos+1;
                if (base_pos == len-1) nbase_pos = 0;
                free (hist[nbase_pos]); // history was full!
                hist[nbase_pos] = hline;

                base_pos = nbase_pos;
                pos = len;
        } else {                // non-full history
                if (len != 0) base_pos++;
                len++;
                pos = len;
                hist[base_pos] = hline;
        }
}

/**
 * NOTE:
 *  - returned char* is newly allocated on the heap!
 *
 * Cannot hurt hist (I think)...
 */
char *hist_up (void)
{
        char *retval = NULL;

        if (len == LEN_HIST) {
                if (pos == (base_pos+1) % len) {
                        retval = hist[pos];
                } else {
                        if (pos == 0) pos = len-1;
                        else pos--;
                        retval = hist[pos];
                }
        } else {
                if (pos == 0) {
                        if (len == 0) retval = "";
                        else retval = hist[0];
                } else {
                        retval = hist[--pos];
                }
        }

        char *ret = malloc ((strlen(retval)+1) * sizeof(char));
        strcpy (ret, retval);

        return ret;
}

/**
 * NOTE:
 *  - returned char* is newly allocated on the heap!
 *
 * Cannot hurt hist (I think)...
 */
char *hist_down (void)
{
        char *retval = NULL;

        if (len == LEN_HIST) {
                if (pos != base_pos) {
                        pos = (pos+1) % len;
                        retval = hist[pos];
                } else {
                        pos = len;
                        retval = "";
                }
        } else {
                if (pos < len) pos++;
                if (pos == len) retval = "";
                else retval = hist[pos];
        }
char *ret = malloc ((strlen(retval)+1) * sizeof(char));
        strcpy (ret, retval);

        return ret;
}

void free_hist (void)
{
        int i;
        for (i = 0; i < len; i++)
                free (hist[i]);
}
