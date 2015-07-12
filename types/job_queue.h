#ifndef JPSH_TYPES_JOB_QUEUE_H
#define JPSH_TYPES_JOB_QUEUE_H

#include "linkedlist.h"

struct jq_elt;

typedef struct {
        linkedlist *jobs;  // of struct jq_elts;
} job_queue;

#endif
