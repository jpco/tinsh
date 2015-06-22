

// local includes
#include "queue.h"
#include "eval_utils.h"
#include "subsh.h"

// self-include
#include "comment.h"

extern queue *ejob_res;
extern queue *ejobs;
extern queue *jobs;

void comment_eval (void)
{
        char *cmdline;
        char *cmdmask;
        q_pop(ejob_res, (void **)&cmdline);
        q_pop(ejob_res, (void **)&cmdmask);

        char *pt = masked_strchr(cmdline, cmdmask, '#');
        if (pt != NULL) {
                *pt = '\0';
        }

        q_push(ejob_res, cmdline);
        q_push(ejob_res, cmdmask);
        q_push(ejobs, subsh_eval);
}
