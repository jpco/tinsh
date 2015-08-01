#include <stdlib.h>
#include <stdio.h>

// local includes
#include "../types/job_queue.h"
#include "../types/jq_elt.h"
#include "../types/scope.h"
#include "../types/job.h"
#include "../types/m_str.h"

#include "env.h"
#include "exec_job.h"

// self-include
#include "exec.h"

extern scope_j *cscope;

void exec (job_queue *jq)
{
        if (jq->cjob == NULL) {
                jq->cjob = ll_makeiter (jq->jobs);
        }

        jq_elt *celt;
        while ((celt = ll_iter_next (jq->cjob)) != NULL) {
                if (celt->is_block) {
                        cscope = new_scope (cscope);
                        while (bj_test (celt->dat.bl->bjob)) {
                                exec (celt->dat.bl->jobs);
                        }
                        cscope = leave_scope (cscope);
                } else {
                        size_t i;
                        for (i = 0; i < cscope->depth; i++) {
                                printf ("   ");
                        }

                        exec_job (celt->dat.job);
                }
        }

        free (jq->cjob);
        jq->cjob = NULL;
}
