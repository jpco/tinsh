// local includes
#include "../types/queue.h"
#include "../types/m_str.h"
#include "spl_pipe.h"

// self-include
#include "spl_block.h"

extern queue *ejobs;
extern queue *elines;

void spl_block (m_str *line);

int spl_block_symbol (m_str *line, char sym)
{
        m_str **spl = ms_split (line, sym);
        if (spl != NULL) {
                ms_free (line);
                spl_block (spl[0]);

                char symstr[2] = { 0 };
                symstr[0] = sym;

                q_push (ejobs, spl_pipe_eval);
                q_push (elines, ms_mask(symstr));

                spl_block (spl[1]);

                return 1;
        } else {
                return 0;
        }
}

void spl_block (m_str *line)
{
        if (spl_block_symbol (line, ':')) return;
        if (spl_block_symbol (line, '{')) return;
        if (spl_block_symbol (line, '}')) return;

        q_push (ejobs, spl_pipe_eval);
        q_push (elines, line);
}

void spl_block_eval (void)
{
        m_str *line = NULL;
        q_pop (elines, (void **)&line);

        spl_block (line);
}
