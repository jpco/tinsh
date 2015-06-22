#include <stdlib.h>
#include <stddef.h>
#include <string.h>

// local includes
#include "queue.h"
#include "eval_utils.h"
#include "job_form.h"

// self-include
#include "spl_pipe.h"

extern queue *ejob_res;
extern queue *ejobs;
extern queue *jobs;

void spl_pipe_eval (void)
{
        char *cmdline;
        char *cmdmask;
        q_pop(ejob_res, (void **)&cmdline);
        q_pop(ejob_res, (void **)&cmdmask);

        char *buf = cmdline;
        char *nbuf = masked_strchr(buf, cmdmask, '|');

        size_t cmdlen = strlen(cmdline);

        char lflag = 0;

        while (buf != NULL && buf - cmdline < cmdlen && *buf != '\0') {
                if (nbuf != NULL) {
                        *nbuf = '\0';
                } else {
                        nbuf = buf + strlen(buf);
                        lflag = 1;
                }
                if (buf == '\0') {
                        continue;
                }

                ptrdiff_t bdiff = buf - cmdline;
                ptrdiff_t nbdiff = nbuf - buf;

                char *ncmd = malloc((nbdiff+1) * sizeof(char));
                strcpy(ncmd, buf);

                char *nmask = calloc(nbdiff+1, sizeof(char));
                memcpy (nmask, cmdmask + bdiff, nbdiff);

                buf = nbuf + 1;
                if (!lflag) {
                        nbuf = masked_strchr(buf, cmdmask+nbdiff, '|');
                }
                q_push(ejob_res, ncmd);
                q_push(ejob_res, nmask);

                if (lflag) {
                        q_push(ejob_res, NULL);
                } else {
                        q_push(ejob_res, (void *)0xBEEFCAFE);
                }

                q_push(ejobs, job_form);
        }

        free (cmdline);
        free (cmdmask);
}

