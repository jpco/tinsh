#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// local includes
#include "queue.h"
#include "linkedlist.h"
#include "job_queue.h"
#include "block_job.h"
#include "m_str.h"

// self-include
#include "block.h"

block_j *block_form (queue *lines)
{
        m_str *cline;
        q_pop (lines, (void **)&cline);

        char sep = '{';
        if (ms_strchr (cline, '{') == NULL) sep = ':';

        m_str *bl_job = ms_dup (cline);
        *(ms_strchr (bl_job, sep)) = 0;
        ms_updatelen (bl_job);
        mbuf_strchr (cline, sep);
        ll_prepend ((linkedlist *)lines, ms_advance (cline, 1));

        block_j *nbl = malloc (sizeof(block_j));
        nbl->bjob = bj_make (bl_job);
        if (sep == '{') {
                nbl->jobs = jq_make (lines);
        } else {
                nbl->jobs = jq_singleton (lines);
        }

        return nbl;
}
