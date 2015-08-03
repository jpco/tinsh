#include <stdlib.h>
#include <stdio.h>

// local includes
#include "../types/job_queue.h"
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

        job_j *celt;
        while ((celt = ll_iter_next (jq->cjob)) != NULL) {
                if (celt->argc > 0) {
                        exec_job (celt);
                }
                                
                if (celt->block != NULL) {
                        cscope = new_scope (cscope);
                        exec (celt->block);
                        cscope = leave_scope (cscope);
                }
        }

        free (jq->cjob);
        jq->cjob = NULL;
}
