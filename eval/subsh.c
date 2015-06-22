#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// local includes
#include "eval_utils.h"
#include "queue.h"
#include "spl_line.h"
#include "mask.h"
#include "var.h"
#include "../str.h"
#include "../debug.h"
#include "../exec.h"

extern queue *ejob_res;
extern queue *ejobs;
extern queue *jobs;

void subsh_eval (void)
{
        char *cmdline;
        char *cmdmask;
        q_pop(ejob_res, (void **)&cmdline);
        q_pop(ejob_res, (void **)&cmdmask);

        char *buf = cmdline-1;
        while ((buf = masked_strchr(buf+1, cmdmask, '`'))) {
                *buf = '\0';
                ptrdiff_t diff = buf + 1 - cmdline;
                char *end = masked_strchr(buf+1, cmdmask+diff, '`');
                if (end == NULL) {
                        print_err ("Unmatched \"`\" in command.");
                        return;
                }
                *end = '\0';
                ptrdiff_t ediff = end + 1 - cmdline;
                char *ret = subshell(buf+1, cmdmask+diff);

                char *rmask = mask_str(ret);

                char *nbuf = vcombine_str(0, 3, cmdline, ret, end+1);

                char *nmask = calloc(strlen(nbuf)+1, sizeof(char));
                memcpy(nmask, cmdmask, strlen(cmdline));
                memcpy(nmask + strlen(cmdline), rmask, strlen(ret));
                memcpy(nmask + strlen(cmdline) + strlen(ret),
                                cmdmask+ediff+1, strlen(cmdline+ediff+1));

                free (cmdline);
                free (cmdmask);
                cmdline = nbuf;
                cmdmask = nmask;
                buf = cmdline + ediff;
        }

        q_push(ejob_res, cmdline);
        q_push(ejob_res, cmdmask);
        q_push(ejobs, spl_line_eval);
}

