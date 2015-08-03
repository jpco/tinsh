#ifndef JPSH_TYPES_JOB_QUEUE_H
#define JPSH_TYPES_JOB_QUEUE_H

#include "linkedlist.h"
#include "job.h"
#include "queue.h"

typedef struct job_str job_j;

typedef struct job_queue_str {
        linkedlist *jobs;  // of job_js
        ll_iter *cjob;
} job_queue;

struct block_str;

job_queue *jq_make(queue *lines);
job_queue *jq_singleton(queue *lines);
void jq_add_job(job_queue *jq, job_j *job);
void jq_add_block(struct job_queue_str *jq, struct block_str *block);

job_j *jq_next(job_queue *jq);

#endif
