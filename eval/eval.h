#ifndef JPSH_EVAL_EVAL_H
#define JPSH_EVAL_EVAL_H

#include "../types/job_queue.h"

/**
 * Evaluates a block of lines and
 * converts it into a job queue.
 */
job_queue *eval (queue *cmd);

#endif
