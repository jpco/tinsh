#ifndef JPSH_TYPES_JOB_H
#define JPSH_TYPES_JOB_H

#include <stddef.h>
#include "m_str.h"
#include "stack.h"

typedef struct job_str {
        m_str *argv; // array
        size_t argc;
        char bg;

        struct job_str *p_prev;
        struct job_str *p_next;

        stack *rd_stack;
} job_j;

#endif
