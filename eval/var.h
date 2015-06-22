#ifndef JPSH_EVAL_VAR_H
#define JPSH_EVAL_VAR_H

#include "../defs.h"

void var_eval(job_t *job);

// A masked version of get_var which will find variables, pull them,
// mask the values, insert both back into the original string, then
// split the new string.
int devar (char *str, char *mask, char ***nstrs, char ***nmask, int *strc);

#endif
