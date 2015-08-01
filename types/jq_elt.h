#ifndef JPSH_TYPES_JQ_ELT_H
#define JPSH_TYPES_JQ_ELT_H

#include "block.h"
#include "job.h"

typedef struct jq_elt_str {
        char is_block;
        union {
                block_j *bl;
                job_j *job;
        } dat;
} jq_elt;

#endif
