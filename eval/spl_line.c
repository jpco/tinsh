#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

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

        m_str *msbuf = line;
        m_str *nmsbuf = malloc(sizeof(m_str));
        nmsbuf->str = msbuf->str;
        nmsbuf->mask = msbuf->mask;
        nmsbuf->len = msbuf->len;
        int nline_exists = mbuf_strchr (nmsbuf, ';');

        while (msbuf != NULL) {
                if (nline_exists) {
                        *(nmsbuf->str) = '\0';
                }
                m_str *nline = ms_dup (msbuf);
                q_push (elines, nline);
                q_push (ejobs, spl_pipe_eval);

                if (!nline_exists) {
                        msbuf = NULL;
                } else {
                        msbuf->str = nmsbuf->str + 1;
                        msbuf->mask = nmsbuf->mask + 1;
                        ms_updatelen (msbuf);
                        nline_exists = mbuf_strchr (nmsbuf, ';');
                }
        }
        free (nmsbuf);
}
