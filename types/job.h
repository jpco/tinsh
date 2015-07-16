#ifndef JPSH_TYPES_JOB_H
#define JPSH_TYPES_JOB_H

#include <stddef.h>
#include "m_str.h"
#include "stack.h"

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

        stack *rd_stack;
} job_j;

job_j *job_form(m_str *line);

#endif
