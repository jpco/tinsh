#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// local includes
#include "block.h"
#include "block_job.h"
#include "m_str.h"
#include "fn.h"
#include "hashtable.h"
#include "stack.h"
#include "scope.h"
#include "linkedlist.h"

#include "../exec/env.h"

#include "../util/defs.h"

// self-include
#include "job_queue.h"
// (kind of self-include)
#include "jq_elt.h"

extern scope_j *cscope;

void jq_add_block (job_queue *jq, block_j *block);
void jq_add_job (job_queue *jq, job_j *job);

job_queue *jq_make (queue *lines)
{
        job_queue *njq = malloc(sizeof(job_queue));
        njq->jobs = ll_make();

        m_str *cline;
        char pipe = 0;
        job_j *ljob = NULL;
        char end_now = 0;
        while (q_len (lines) > 0) {
                q_pop (lines, (void **)&cline);
                if (cline == PIPE_MARKER) {
                        pipe = 1;
                } else {
                        if (ms_strchr(cline, '}') != NULL) {
                                m_str *bl_line = ms_dup (cline);
                                *(ms_strchr (bl_line, '}')) = 0;
                                ms_updatelen (bl_line);
                                mbuf_strchr (cline, '}');
                                ll_prepend ((linkedlist *)lines,
                                        ms_advance (cline, 1));
                                cline = bl_line;

                                end_now = 1;
                        }

                        if (!pipe) {
                                // add previous pipe chain to the job queue
                                job_j *pjob = ljob;
                                if (pjob) {
                                        while (pjob->p_prev != NULL) {
                                                pjob = pjob->p_prev;
                                        }

                                        jq_add_job (njq, pjob);
                                }

                                ljob = NULL;
                        }

                        // here is where we handle blocks
                        if (ms_strchr (cline, '{') != NULL ||
                                ms_strchr (cline, ':') != NULL) {
                                // nasty
                                ll_prepend ((linkedlist *)lines, cline);
                                block_j *cblock = block_form(lines);
                                jq_add_block (njq, cblock);
                        } else {
                                ljob = job_form (cline, ljob);
                        }
                        pipe = 0;
                }
                if (end_now) break;
        }

        job_j *pjob = ljob;
        if (pjob) {
                while (pjob->p_prev != NULL) {
                        pjob = pjob->p_prev;
                }
                jq_add_job (njq, pjob);
        }

        njq->cjob = NULL;
        return njq;
}

job_queue *jq_singleton (queue *lines)
{
        job_queue *njq = malloc(sizeof(job_queue));
        njq->jobs = ll_make();

        m_str *cline;
        job_j *ljob = NULL;
        do {
                q_pop (lines, (void **)&cline);

                // here is where we handle blocks
                if (ms_strchr (cline, '{') != NULL ||
                        ms_strchr (cline, ':') != NULL) {
                        // nasty
                        ll_prepend ((linkedlist *)lines, cline);
                        block_j *cblock = block_form(lines);
                        jq_add_block (njq, cblock);
                } else {
                        ljob = job_form (cline, ljob);
                }
                q_pop (lines, (void **)&cline);

        } while (cline == PIPE_MARKER);
        ll_prepend ((linkedlist *)lines, cline);

        job_j *pjob = ljob;
        if (pjob) {
                while (pjob->p_prev != NULL) {
                        pjob = pjob->p_prev;
                }
                jq_add_job (njq, pjob);
        }

        njq->cjob = NULL;
        return njq;
}

void jq_add_block (job_queue *jq, block_j *block)
{
        jq_elt *nelt = malloc(sizeof(jq_elt));
        nelt->is_block = 1;
        nelt->dat.bl = block;
        ll_append((linkedlist *)jq->jobs, nelt);
}

void jq_add_job (job_queue *jq, job_j *job)
{
        jq_elt *nelt = malloc(sizeof(jq_elt));
        nelt->is_block = 0;
        nelt->dat.job = job;
        ll_append((linkedlist *)jq->jobs, nelt);
}
