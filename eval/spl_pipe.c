#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

// local includes
#include "../types/m_str.h"
#include "../types/queue.h"

#include "../util/defs.h"

// self-include
#include "spl_pipe.h"

extern queue *elines;
extern queue *ejobs;

void spl_pipe_eval (void)
{
        m_str *line = NULL;
        q_pop (elines, (void **)&line);

        m_str **pipe_split;
        while ((pipe_split = ms_split(line, '|')) != NULL) {
                ms_free (line);
                q_push (elines, pipe_split[0]);
                q_push (elines, PIPE_MARKER);
                line = pipe_split[1];
        }
        q_push (elines, line);
}
