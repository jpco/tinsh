#include <stdlib.h>
#include <string.h>

// local includes
#include "eval/queue.h"
#include "eval/eval_utils.h"
#include "eval/subsh.h"
#include "eval/mask.h"
#include "eval/comment.h"
#include "eval/spl_line.h"
#include "eval/job_form.h"
#include "eval/spl_pipe.h"
#include "eval/var.h"

#include "str.h"
#include "debug.h"
#include "exec.h"
#include "var.h"

// self-include
#include "eval.h"

queue *ejob_res;
queue *ejobs;
queue *jobs;

void mask_eval();
void comment_eval();
void spl_line_eval();
void spl_pipe_eval();
void job_form();
void var_eval();

void eval (char *cmd)
{
        eval_m (cmd, NULL);
}

void free_ceval (void)
{
        if (ejob_res != NULL) {
                while (q_len (ejob_res) > 0) {
                        void *elt;
                        q_pop(ejob_res, &elt);
                        free (elt);
                }
                free (ejob_res);
                ejob_res = NULL;
        }

        if (ejobs != NULL) {
                while (q_len (ejobs) > 0) {
                        void *elt;
                        q_pop(ejobs, &elt);
                        free (elt);
                }
                free (ejobs);
                ejobs = NULL;
        }

        if (jobs != NULL) {
                while (q_len (jobs) > 0) {
                        void *elt;
                        q_pop(jobs, &elt);
                        free (elt);
                }
                free (jobs);
                jobs = NULL;
        }
}

void eval_m (char *cmd, char *mask)
{
        ejob_res = q_make();
        ejobs = q_make();
        jobs = q_make();
        atexit (free_ceval);

        char *ncmd = malloc((strlen(cmd)+1) * sizeof(char));
        strcpy(ncmd, cmd);
        free (cmd);

        q_push(ejob_res, ncmd);
        if (mask == NULL) {
                q_push(ejobs, mask_eval);
        } else {
                char *nmask = calloc((strlen(ncmd)+1), sizeof(char));
                memcpy(nmask, mask, strlen(ncmd));

                q_push(ejob_res, nmask);
                q_push(ejobs, comment_eval);
        }

        while (q_len(ejobs) > 0) {
                void (*ejob)(void);
                q_pop(ejobs, (void **)&ejob);
                ejob();
        }

        while (q_len(jobs) > 0) {
                job_t *job;
                q_pop (jobs, (void **)&job);

                job->job_fn(job);
        }

        free_ceval();
}
