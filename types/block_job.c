#include <stdlib.h>
#include <string.h>

#include "m_str.h"

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

block_job *bj_make (m_str *stmt)
{
        block_job *new_bj = malloc(sizeof(block_job));
        
        if (ms_startswith (stmt, "if ")) {
                new_bj->type = BJ_IF;
        } else if (ms_startswith (stmt, "for ")) {
                new_bj->type = BJ_FOR;
        } else if (ms_startswith (stmt, "cfor ")) {
                new_bj->type = BJ_CFOR;
        } else if (ms_startswith (stmt, "while ")) {
                new_bj->type = BJ_WHILE;
        } else if (ms_startswith (stmt, "with ")) {
                new_bj->type = BJ_WITH;
        } else if (ms_startswith (stmt, "else ")) {
                new_bj->type = BJ_ELSE;
        } else {
                new_bj->type = BJ_ENTER;
        }

        if (new_bj->type != BJ_ENTER) {
                m_str *new_ms = malloc(sizeof(m_str));
                char *pt = ms_strchr(stmt, ' ');
                new_ms->str = strdup(pt+1);
                new_ms->mask = calloc(strlen(new_ms->str), sizeof(char));
                memcpy(new_ms->mask, stmt->mask+(pt-(stmt->str))+1,
                        strlen(new_ms->str));

                new_bj->stmt = new_ms;
        } else {
                new_bj->stmt = ms_dup (stmt);
        }

        return new_bj;
}
