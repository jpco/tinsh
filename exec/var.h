#ifndef JPSH_EVAL_VAR_H
#define JPSH_EVAL_VAR_H

#include "../types/job.h"

void var_eval(job_j *job);

// A masked version of get_var which will find variables, pull them,
// mask the values, insert both back into the original string, then
// split the new string.
int devar(m_str *line, char ***nstrs, char ***nmask, int *strc);

#endif
