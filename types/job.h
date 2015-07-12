#ifndef JPSH_TYPES_JOB_H
#define JPSH_TYPES_JOB_H

#include <stddef.h>

typedef struct {
        m_str *argv;
        size_t argc;
} job_j;

#endif
