#ifndef JPSH_TYPES_ARGLIST_H
#define JPSH_TYPES_ARGLIST_H

#include "linkedlist.h"

typedef struct {
        char *name;
        char greedy;
} arg_j;

typedef struct {
        linkedlist *args;  // of arg_j elts
} arglist_j;

#endif
