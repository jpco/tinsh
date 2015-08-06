#ifndef JPSH_EVAL_VAR_H
#define JPSH_EVAL_VAR_H

#include "../types/job.h"
#include "../types/job_queue.h"

void var_eval(job_j *job);

size_t devar_argv (m_str **argv, size_t argc);

void devar_arg (m_str **arg);

m_str *devar(char *name);

#endif
