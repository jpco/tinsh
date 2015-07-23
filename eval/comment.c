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
        while (q_len (elines) > 0) {
                m_str *line;
                q_pop (elines, (void **)&line);

                char *comment = ms_strchr (line, '#');
                if (comment != NULL) {
                        *comment = '\0';
                        ms_updatelen (line);
                }
                if (*(line->str) != '\0') {
                        q_push (nqueue, line);
                }
        }

        q_destroy (elines);
        elines = nqueue;

        q_push (ejobs, spl_line_eval);
}
