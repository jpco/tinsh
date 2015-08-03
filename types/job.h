#ifndef JPSH_TYPES_JOB_H
#define JPSH_TYPES_JOB_H

#include <stddef.h>
#include "m_str.h"
#include "stack.h"
#include "job_queue.h"

typedef struct job_queue_str job_queue;

typedef enum {RD_FIFO, RD_APP, RD_PRE, RD_REV, RD_OW, RD_LIT, RD_RD, RD_REP} rdm;

typedef struct rd_struct {
        int loc_fd;
        rdm mode;       // type of redirection to be done
        int fd;
        m_str *name;
} rd_j;

typedef struct job_str {
        m_str **argv; // array
        size_t argc;
        char bg;

        struct job_str *p_prev;
        struct job_str *p_next;

        int *p_in;      // used during the exec stage
        int *p_out;

        stack *rd_stack;

        // if this job is a block job this will not be null
        job_queue *block;
} job_j;

job_j *job_form(m_str *line, job_j *prev_job);

#endif
