#ifndef JPSH_TYPES_SCOPE_H
#define JPSH_TYPES_SCOPE_H

#include "hashtable.h"

typedef struct {
        scope_j *parent;
        hashtable *vars;
} scope_j;

void new_scope();
void leave_scope();

#endif
