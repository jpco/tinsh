#ifndef JPSH_EXEC_ENV_H
#define JPSH_EXEC_ENV_H

#include "../types/scope.h"

hashtable *alias_tab;
scope_j *gscope;
scope_j *cscope;  // defines full scope stack within scope type as well

#endif
