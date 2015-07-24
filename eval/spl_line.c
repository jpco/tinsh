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
        m_str *line = NULL;
        q_pop (elines, (void **)&line);

        if (line == NULL) return;
        ms_updatelen(line);
        size_t cmdlen = ms_len(line);

        char *buf = line->str;
        char *nbuf = ms_strchr(line, ';');

        while (buf != NULL && buf - line->str < cmdlen && *buf != '\0') {
                if (nbuf != NULL) {
                        *nbuf = '\0';
                } else {
                        nbuf = buf + strlen(buf);
                }
                if (buf == '\0') continue;

                m_str *nline = ms_dup_at(line, nbuf);

                buf = nbuf + 1;
                if (buf - line->str < cmdlen) {
                        m_str *bufms = ms_dup_at(line, buf);
                        nbuf = ms_strchr(bufms, ';');
                        ms_free (bufms);
                }

                q_push(elines, nline);
                q_push(ejobs, spl_pipe_eval);
        }

        ms_free (line);
}
