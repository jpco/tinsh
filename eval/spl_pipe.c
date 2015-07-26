#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

// local includes
#include "../types/m_str.h"
#include "../types/queue.h"

// self-include
#include "spl_pipe.h"

extern queue *elines;
extern queue *ejobs;

// TODO: make this work with m_str natively
void spl_pipe_eval (void)
{
        m_str *line;
        q_pop(elines, (void **)&line);

        if (line == NULL) {
                return;
        }
        char *buf = line->str;
        char *nbuf = ms_strchr(line, '|');

        size_t cmdlen = ms_len(line);

        char lflag = 0;

        while (buf != NULL && buf - line->str < cmdlen && *buf != '\0') {
                if (nbuf != NULL) {
                        *nbuf = '\0';
                } else {
                        nbuf = buf + strlen(buf);
                        lflag = 1;
                }
                if (buf == '\0') {
                        continue;
                }

                m_str *nline = ms_dup_at(line, buf);

                buf = nbuf + 1;
                if (!lflag) {
                        m_str *bufms = ms_dup_at(line, buf);
                        nbuf = ms_strchr(bufms, '|');
                        ms_free (bufms);
                }
                q_push(elines, nline);
        }

        ms_free (line);
}
