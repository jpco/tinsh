#ifndef JPSH_TYPES_JOB_QUEUE_H
#define JPSH_TYPES_JOB_QUEUE_H

#include "linkedlist.h"
#include "block.h"
#include "job.h"

typedef struct {
        linkedlist *jobs;  // of struct jq_elts;
} job_queue;

job_queue *jq_make(queue *jobs);
job_queue *jq_singleton(job_j *job);
void jq_add_block(job_queue *jq, block_j *block);
void jq_add_job(job_queue *jq, job_j *job);

#endif
