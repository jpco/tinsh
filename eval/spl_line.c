#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// local includes
#include "../types/m_str.h"
#include "../types/queue.h"
#include "spl_pipe.h"

// self-include
#include "spl_line.h"

extern queue *elines;
extern queue *ejobs;

// TODO: make this work with m_str natively
void spl_line_eval (void)
{
        m_str *line;
        q_pop (elines, (void **)&line);

        char *cmdline = line->str;
        char *cmdmask = line->mask;

        size_t cmdlen = strlen(cmdline);

        char *buf = cmdline;
        char *nbuf = masked_strchr(buf, cmdmask, ';');

        while (buf != NULL && buf - cmdline < cmdlen && *buf != '\0') {
                if (nbuf != NULL) {
                        *nbuf = '\0';
                } else {
                        nbuf = buf + strlen(buf);
                }
                if (buf == '\0') continue;

                ptrdiff_t bdiff = buf - cmdline;
                ptrdiff_t nbdiff = nbuf - buf;

                char *ncmd = malloc ((nbdiff+1) * sizeof(char));
                strcpy (ncmd, buf);

                char *nmask = calloc (nbdiff+1, sizeof(char));
                memcpy (nmask, cmdmask + bdiff, nbdiff + 1);

                buf = nbuf + 1;
                if (buf - cmdline < cmdlen) {
                        nbuf = masked_strchr(buf, cmdmask+nbdiff, ';');
                }

                m_str *nms = malloc(sizeof(m_str));
                nms->str = ncmd;
                nms->mask = nmask;
                q_push(ejob_res, nms);
                q_push(ejobs, spl_pipe_eval);
        }

        ms_free (line);
}
