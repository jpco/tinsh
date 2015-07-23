#ifndef JPSH_TYPES_SCOPE_H
#define JPSH_TYPES_SCOPE_H

#include "hashtable.h"

typedef struct scope_str {
        struct scope_str *parent;
        hashtable *vars;
        hashtable *fns;
} scope_j;

// p is parent scope
scope_j *new_scope(scope_j *p);
// returned is new base scope
scope_j *leave_scope(scope_j *curr);

#endif
