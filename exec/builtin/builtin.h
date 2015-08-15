#ifndef JPSH_EXEC_BUILTIN_BUILTIN_H
#define JPSH_EXEC_BUILTIN_BUILTIN_H

#include "../../types/job.h"
#include "../env.h"

int func_set (job_j *job);
int func_settype (job_j *job, vartype v_type);
int builtin (job_j *job);

#endif
