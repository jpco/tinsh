#include <stdlib.h>
#include <string.h>

// local includes
#include "queue.h"
#include "job_queue.h"
#include "block_job.h"

// self-include
#include "block.h"

block_j *block_form(queue *lines)
{
        block_j *new_bl = malloc(sizeof(block_j));

        // block job
        m_str *fln;
        q_pop(lines, (void **)&fln);
        char *div;
        if ((div = ms_strchr(fln, '{')) == NULL)
                div = ms_strchr(fln, ':');

        m_str *bj_cpy = malloc(sizeof(m_str));
        char tmpdiv = *div;
        *div = '\0';
        bj_cpy->str = strdup(fln->str);
        bj_cpy->mask = malloc(strlen(bj_cpy->str)*sizeof(char));
        memcpy(bj_cpy->mask, fln->mask, strlen(bj_cpy->str));
        ms_updatelen(bj_cpy);
        *div = tmpdiv;

        fln->str += ms_len(bj_cpy) + 1;
        fln->mask += ms_len(bj_cpy) + 1;
        ms_updatelen(fln);

        ms_trim(&fln);

        new_bl->bjob = bj_make(bj_cpy);

        if (fln->len > 0) {
                q_push(lines, fln);
        }

        // job_queue
        if (ms_strchr(fln, '{') != NULL) { // multi-line
                new_bl->jobs = jq_make(lines);
        } else { // single-line
                new_bl->jobs = jq_singleton(lines);
        }

        return new_bl;
}
