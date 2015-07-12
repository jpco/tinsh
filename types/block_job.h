#ifndef JPSH_TYPES_BLOCK_JOB_H
#define JPSH_TYPES_BLOCK_JOB_H

#include "m_str.h"

typedef enum {
        BJ_ENTER,
        BJ_CONT,
        BJ_IF,
        BJ_FOR,
        BJ_CFOR,
        BJ_WHILE,
        BJ_WITH
} bj_type;

typedef struct {
        bj_type type;
        m_str *stmt;
} block_job;

// returns 1 if we are to enter the block,
// 0 otherwise
char bj_test (block_job *bj);

#endif
