#include <stdio.h>

#include "../types/job_queue.h"
#include "env.h"
#include "../types/scope.h"
#include "../types/job.h"
#include "../types/m_str.h"

#include "exec.h"

extern scope_j *cscope;

void exec (job_queue *jq)
{
//        job_j *cjob = NULL;
        jq_next (jq);
}
