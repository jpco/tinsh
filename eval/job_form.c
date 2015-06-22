#include <stdlib.h>
#include <string.h>

// local includes
#include "queue.h"
#include "var.h"
#include "eval_utils.h"
#include "../str.h"
#include "../debug.h"
#include "../defs.h"

// self-include
#include "job_form.h"

extern queue *ejob_res;
extern queue *ejobs;
extern queue *jobs;

static job_t *prev_job;

void job_form (void)
{
        char *line;
        char *mask;
        char *job_out;
        q_pop(ejob_res, (void **)&line);
        q_pop(ejob_res, (void **)&mask);
        q_pop(ejob_res, (void **)&job_out);

        char *nline = NULL;
        char *nmask = NULL;
        masked_trim_str (line, mask, &nline, &nmask);
        free (line);
        free (mask);

        if (*nline == '\0') {
                free (nline);
                free (nmask);
                return;
        }

        job_t *job = malloc(sizeof(job_t));

        // defaults
        job->p_in = NULL;
        job->p_out = NULL;
        job->p_prev = NULL;
        job->p_next = NULL;
        job->file_in = NULL;
        job->file_out = NULL;
        job->bg = 0;

        spl_cmd (nline, nmask, &(job->argv), &(job->argm), &(job->argc));
        free (nline);
        free (nmask);

        // Background job?
        if (olstrcmp(job->argv[job->argc-1], "&")) {
                job->bg = 1;
                rm_element (job->argv, job->argm, (job->argc)-1,
                                &(job->argc));
        }

        // File redirection
        int i;
        for (i = 0; i < job->argc; i++) {
                if (olstrcmp(job->argv[i], ">") && !(*(job->argm[i]))) {
                        rm_element (job->argv, job->argm, i, &(job->argc));

                        if (i == job->argc) {      // they messed up
                                print_err ("Missing redirect destination.");
                                continue;
                        }

                        char *fname = malloc(strlen(job->argv[i])+1);
                        strcpy (fname, job->argv[i]);
                        rm_element (job->argv, job->argm, i, &(job->argc));
                        i -= 2;

                        job->file_out = fname;

                } else if (olstrcmp(job->argv[i], "<")
                                && !(*(job->argm[i]))) {
                        rm_element (job->argv, job->argm, i, &(job->argc));

                        if (i == job->argc) {      // they messed up
                                print_err ("Missing redirect source.");
                                continue;
                        }

                        char *fname = malloc(strlen(job->argv[i])+1);
                        strcpy (fname, job->argv[i]);
                        rm_element (job->argv, job->argm, i, &(job->argc));
                        i -= 2;

                        job->file_in = fname;
                }
        }

        // Piped from previous job?
        if (prev_job != NULL) {
                job->p_prev = prev_job;
                job->p_prev->p_next = job;
        }

        // Pipe to next job?
        if (job_out != NULL) {
                prev_job = job;
        } else {
                prev_job = NULL;
        }

        job->job_fn = var_eval;

        q_push (jobs, job);
}
