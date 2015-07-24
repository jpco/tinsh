#ifndef JPSH_TYPES_BLOCK_H
#define JPSH_TYPES_BLOCK_H

#include "block_job.h"
#include "job_queue.h"
#include "queue.h"

typedef struct block_str {
        block_job *bjob;
        struct job_queue_str *jobs;
} block_j;

block_j *block_form(queue *lines);

#endif
