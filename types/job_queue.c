#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// local includes
#include "m_str.h"
#include "fn.h"
#include "hashtable.h"
#include "stack.h"
#include "scope.h"
#include "linkedlist.h"

#include "../exec/env.h"

#include "../util/defs.h"
#include "../util/str.h"

// self-include
#include "job_queue.h"

extern scope_j *cscope;

void jq_load_chain (job_queue *jq, job_j *jq_job);

job_queue *jq_make (queue *lines)
{
        job_queue *njq = malloc(sizeof(job_queue));
        njq->jobs = ll_make();
        njq->cjob = NULL;

        m_str *line;
        char pipe = 0;
        job_j *prev_job = NULL;
        while (q_len (lines) > 0) {
                q_pop (lines, (void **)&line);

                if (line == PIPE_MARKER) {
                        pipe = 1;
                } else {
                        if (line == NULL || line->str == NULL) continue;

                        if (olstrcmp (line->str, "{")) {
                                ms_free (line);
                                line = NULL;
                                prev_job->block = jq_make (lines);
                                pipe = 0;
                        } else if (olstrcmp (line->str, ":")) {
                                ms_free (line);
                                line = NULL;
                                prev_job->block = jq_singleton (lines);
                                pipe = 0;
                        } else if (olstrcmp (line->str, "}")) {
                                ms_free (line);
                                break;
                        }

                        if (!pipe) {
                                jq_load_chain (njq, prev_job);
                                prev_job = NULL;
                        } 

                        if (line != NULL) {
                                prev_job = job_form (line, prev_job);
                        }
                        pipe = 0;
                }
        }
        jq_load_chain (njq, prev_job);
        return njq;
}

job_queue *jq_singleton (queue *lines) { return NULL; }

void jq_load_chain (job_queue *jq, job_j *jq_job)
{
        if (jq_job == NULL) return;

        while (jq_job->p_prev != NULL) {
                jq_job = jq_job->p_prev;
        }
        ll_append (jq->jobs, jq_job);
}
