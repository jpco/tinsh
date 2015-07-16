#ifndef JPSH_TYPES_BLOCK_H
#define JPSH_TYPES_BLOCK_H

#include "block_job.h"
#include "job_queue.h"

typedef struct {
        block_job *bjob;
        job_queue *jobs;
} block_j;

block_j *block_form(queue *lines);

#endif
