#ifndef JPSH_TYPES_FN_H
#define JPSH_TYPES_FN_H

#include "arglist.h"
#include "job_queue.h"
#include "queue.h"

typedef struct {
        char *name;
        arglist_j *args;
        job_queue *jq;
} fn_j;

fn_j *fn_form(queue *lines);

#endif
