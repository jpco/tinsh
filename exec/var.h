#ifndef JPSH_EVAL_VAR_H
#define JPSH_EVAL_VAR_H

#include "../types/job.h"
#include "../types/job_queue.h"

void var_eval(job_j *job);

m_str *devar(char *name);

#endif
