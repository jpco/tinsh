

// self-include
#include "block_job.h"

// TODO: finish this
char bj_test (block_job *bj)
{
        switch (bj->type) {
                case BJ_CONT:
                        return 0;
/*                case BJ_IF:
                case BJ_WHILE:
                case BJ_FOR:
                case BJ_CFOR: */
                default:        // BJ_WITH
                        bj->type = BJ_CONT;
                        return 1;
        }
}
