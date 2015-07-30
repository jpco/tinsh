#include <stdlib.h>
#include <stdio.h>

#include "../types/job_queue.h"
#include "../types/jq_elt.h"
#include "../types/scope.h"
#include "../types/job.h"
#include "../types/m_str.h"

#include "env.h"
#include "exec.h"

extern scope_j *cscope;

void exec (job_queue *jq)
{
//        printf ("entering exec\n");
        if (jq->cjob == NULL) {
                jq->cjob = ll_makeiter (jq->jobs);
        }

        jq_elt *celt;
        while ((celt = ll_iter_next (jq->cjob)) != NULL) {
                if (celt->is_block) {
                        cscope = new_scope (cscope);
//                        printf (" :: entering\n");
//                        ms_print (celt->dat.bl->bjob->stmt, 1);
                        exec (celt->dat.bl->jobs);
                        cscope = leave_scope (cscope);
                } else {
                        // TODO: print scope-depth tabs
                        size_t i;
                        for (i = 0; i < cscope->depth; i++) {
                                printf ("   ");
                        }

                        // print pipe string
                        job_j *cjob = celt->dat.job;
                        while (1) {
                                size_t i;
                                for (i = 0; i < cjob->argc; i++) {
                                        printf ("'");
                                        ms_print (cjob->argv[i], 0);
                                        printf ("' ");
                                }
                                
                                if (cjob->p_next != NULL) {
                                        cjob = cjob->p_next;
                                        printf ("| ");
                                } else {
                                        printf ("\n");
                                        break;
                                }
                        }
                }
        }

        free (jq->cjob);
        jq->cjob = NULL;

//        printf (" :: exiting\n");
}
