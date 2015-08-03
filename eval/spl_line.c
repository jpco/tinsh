#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

// local includes
#include "../types/m_str.h"
#include "../types/queue.h"
#include "spl_block.h"

// self-include
#include "spl_line.h"

extern queue *elines;
extern queue *ejobs;

// TODO: make this work with m_str natively
void spl_line_eval (void)
{
        m_str *line = NULL;
        q_pop (elines, (void **)&line);

        m_str **line_split;
        while ((line_split = ms_split(line, ';')) != NULL) {
                ms_free (line);
                q_push (ejobs, spl_block_eval);
                q_push (elines, line_split[0]);
                line = line_split[1];
        }
        q_push (ejobs, spl_block_eval);
        q_push (elines, line);
}
