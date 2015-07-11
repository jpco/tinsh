#ifndef JPSH_JOB_H
#define JPSH_JOB_H

#include "stack.h"

/**
 * The struct describing a job.
 *
 * Fields are
 *  - argv: obvious.
 *  - argc: obvious.
 *  - bg: whether or not the job is to run in the background.
 *  - p_in: the pipe in (not set until the exec phase)
 *  - p_out: the pipe out (not set until the exec phase)
 *  - p_prev: the job piping into this one. Set during eval phase.
 *  - p_next: the job this one pipes into. Set during eval phase.
 *  - file_in: the file to act as stdin.
 *  - file_out: the file to act as stdout.
 *  - job_fn: used during the eval phase.
 */
typedef struct job_struct {
        // the usual argument parameters.
        char **argv;
        char **argm;
        int argc;

        // whether the job is in the background.
        int bg;

        // for piping, the file descriptors in & out
        // used during exec stage
        int *p_in;
        int *p_out;
        
        // for piping, the prev. and next job
        // used during eval stage
        struct job_struct *p_prev;
        struct job_struct *p_next;

        // the file redirections to take place.
        stack *rd_stack;

        // used in the eval process
        void (*job_fn)(struct job_struct *);
} job_t;

#endif
