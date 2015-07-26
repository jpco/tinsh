#include <stdio.h>

#include "../types/job_queue.h"
#include "env.h"
#include "../types/scope.h"
#include "../types/job.h"
#include "../types/m_str.h"

#include "exec.h"

extern scope_j *cscope;

void exec (job_queue *jq)
{
        job_j *cjob = NULL;
        while ((cjob = jq_next (jq)) != NULL) {
                job_j *pjob = cjob;
                int i;
                for (i = 0; i < pjob->argc; i++) {
                        printf ("'");
                        ms_print (pjob->argv[i], 0);
                        printf ("' ");
                }
                while ((pjob = pjob->p_next) != NULL) {
                        printf ("| ");
                        for (i = 0; i < pjob->argc; i++) {
                                printf ("'");
                                ms_print (pjob->argv[i], 0);
                                printf ("' ");
                        }
                }
                printf ("\n");
        }
}
