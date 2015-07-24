#include <stdlib.h>
#include <string.h>

// local includes
#include "block.h"
#include "m_str.h"
#include "fn.h"
#include "hashtable.h"
#include "stack.h"
#include "scope.h"

#include "../exec/env.h"

// self-include
#include "job_queue.h"

extern scope_j *cscope;

typedef struct jq_elt_str {
        char is_block;
        union {
                block_j *bl;
                job_j *job;
        } dat;
} jq_elt;

void jq_add_block (job_queue *jq, block_j *block);
void jq_add_job (job_queue *jq, job_j *job);

job_queue *jq_make (queue *lines)
{
        job_queue *jq = malloc(sizeof(job_queue));
        if (jq == NULL) return NULL;
        jq->jobs = ll_make();
        if (jq->jobs == NULL) {
                free (jq);
                return NULL;
        }

        char end_now = 0;

        while (q_len(lines) > 0) {
                if (end_now) break;

                m_str *cline;
                q_pop (lines, (void **)&cline);

                if (ms_strchr(cline, '}') != NULL) {
                        char *div = ms_strchr(cline, '}');

                        m_str *line_cpy = malloc(sizeof(m_str));
                        *div = '\0';
                        line_cpy->str = strdup(cline->str);
                        line_cpy->mask = malloc(strlen(line_cpy->str)
                                         * sizeof(char));
                        memcpy(line_cpy->mask, cline->mask,
                                        strlen(line_cpy->str));
                        ms_updatelen(line_cpy);

                        cline->str += ms_len(line_cpy) + 1;
                        cline->mask += ms_len(line_cpy) + 1;
                        ms_updatelen(cline);
                        q_push(lines, cline);

                        cline = line_cpy;
                        end_now = 1;
                }

                if (ms_strchr(cline, '{') != NULL
                                || ms_strchr(cline, ':') != NULL) {
                        // block/fn!
                        if (ms_startswith(cline, "fn")) {
                                // fn!
                                fn_j *new_fn = fn_form(lines);
                                // TODO: check for null
                                ht_add(cscope->fns, new_fn->name, new_fn);
                        } else {
                                // block
                                block_j *new_bl = block_form(lines);
                                // TODO: check for null
                                jq_add_block(jq, new_bl);
                        }
                } else {
                        // job
                        job_j *new_job = job_form(cline);
                        // TODO: check for null
                        jq_add_job(jq, new_job);
                }
        }

        return jq;
}

job_queue *jq_singleton (queue *lines)
{
        job_queue *jq = malloc(sizeof(job_queue));
        if (jq == NULL) return NULL;
        jq->jobs = ll_make();
        if (jq->jobs == NULL) {
                free (jq);
                return NULL;
        }

        m_str *cline;
        q_pop (lines, (void **)&cline);

        if (ms_strchr(cline, '{') != NULL
                        || ms_strchr(cline, ':') != NULL) {
                // block/fn!
                if (ms_startswith(cline, "fn")) {
                        // fn!
                        fn_j *new_fn = fn_form(lines);
                        // TODO: check for null
                        ht_add(cscope->fns, new_fn->name, new_fn);
                } else {
                        // block
                        block_j *new_bl = block_form(lines);
                        // TODO: check for null
                        jq_add_block(jq, new_bl);
                }
        } else {
                // job
                job_j *new_job = job_form(cline);
                // TODO: check for null
                jq_add_job(jq, new_job);
        }

        return jq;
}

void jq_add_block (job_queue *jq, block_j *block)
{
        jq_elt *nelt = malloc(sizeof(jq_elt));
        nelt->is_block = 1;
        nelt->dat.bl = block;
        ll_append((linkedlist *)jq, nelt);
}

void jq_add_job (job_queue *jq, job_j *job)
{
        jq_elt *nelt = malloc(sizeof(jq_elt));
        nelt->is_block = 0;
        nelt->dat.job = job;
        ll_append((linkedlist *)jq, nelt);
}
