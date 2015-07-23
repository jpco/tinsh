#include <stdlib.h>
#include <string.h>

// local includes
#include "../types/m_str.h"
#include "../types/queue.h"
#include "../util/str.h"
#include "../util/debug.h"

#include "comment.h"

// self-include
#include "mask.h"

extern queue *ejobs;
extern queue *elines;

void mask_eval (void)
{
        queue *mlines = q_make();
        while (q_len (elines) > 0) {
                char *line;
                q_pop (elines, (void **)&line);

                char *lbs = strrchr(line, '\\');
                while (lbs != NULL && lbs+1 == '\0') {
                        if (q_len (elines) == 0) {
                                print_err ("Backslash at end of line"
                                        " with no line to follow.");
                                return;
                        }
                        char *nextline;
                        q_pop (elines, (void **)&nextline);
                        line = vcombine_str (0, 2, line, nextline);

                        lbs = strrchr (line, '\\');
                }
                m_str *mline = ms_mask (line);
                free (line);

                q_push (mlines, mline);
        }

        q_destroy (elines);
        elines = mlines;

        q_push (ejobs, comment_eval);
}
