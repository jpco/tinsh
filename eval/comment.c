#include <stdio.h>
#include <errno.h>

// local includes
#include "../types/queue.h"
#include "../types/m_str.h"
#include "spl_line.h"

// self-include
#include "comment.h"

extern queue *ejobs;
extern queue *elines;

void comment_eval (void)
{
        queue *nqueue = q_make();
        if (elines == NULL) return;
        while (q_len (elines) > 0) {
                m_str *line = NULL;
                q_pop (elines, (void **)&line);

                m_str **comment_split = ms_split (line, '#');

                if (comment_split == NULL) {
                        q_push (nqueue, line);
                } else {
                        ms_free (line);
                        ms_free (comment_split[1]);
                        q_push (elines, comment_split[0]);
                }
        }

        while (q_len (nqueue) != 0) {
                m_str *line;
                q_pop (nqueue, (void **)&line);
                q_push (elines, line);
                q_push (ejobs, spl_line_eval);
        }

        q_destroy (nqueue);
}
