#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

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

void redirect (job_t *job, int i)
{
        // TODO: update  cmd splitting to account for syntax changes

        char *o_rdwd = vcombine_str(0, 1, job->argv[i]);
        char *rdwd = o_rdwd;
        rm_element (job->argv, job->argm, i, &(job->argc));

        rd_t *redir = malloc(sizeof(rd_t));
        // defaults
        redir->loc_fd = STDOUT_FILENO;
        redir->mode = RD_OW;
        redir->fd = -1;
        redir->name = NULL;

        if (*rdwd == '<') {
                // inbound redirect!
                redir->loc_fd = STDIN_FILENO;
                rdwd++;
                if (*rdwd == '<') {     // literal
                        redir->mode = RD_LIT;
                        free (o_rdwd);

                        if (i == job->argc) {
                                print_err ("Missing redirect literal.");
                                free (redir);
                                return;
                        }
                        redir->name = vcombine_str(0, 1, job->argv[i]);
                        rm_element (job->argv, job->argm, i, &(job->argc));
                } else if (*rdwd == '-') {      // file
                        free (o_rdwd);
                        redir->mode = RD_RD;

                        if (i == job->argc) {
                                print_err ("Missing redirect source.");
                                free (redir);
                                return;
                        }
                        redir->name = vcombine_str(0, 1, job->argv[i]);
                        rm_element (job->argv, job->argm, i, &(job->argc));
                } else {        // fifo!
                        redir->mode = RD_FIFO;
                        rdwd++;
                        if (*rdwd == '~') {
                                redir->fd = STDOUT_FILENO;
                        } else if (*rdwd == '&') {
                                redir->fd = -2;
                        } else {
                                redir->fd = *rdwd - '0';
                        }
                        free (o_rdwd);

                        if (i == job->argc) {
                                print_err ("Missing redirect command.");
                                free (redir);
                                return;
                        }
                        redir->name = vcombine_str(0, 1, job->argv[i]);
                        rm_element (job->argv, job->argm, i, &(job->argc));
                }
        } else {
                if (*rdwd == '~') redir->mode = RD_FIFO;
                rdwd++;

                if (*rdwd == '>') redir->loc_fd = STDOUT_FILENO;
                else if (*rdwd == '&') {
                        redir->loc_fd = -2;
                        rdwd++;
                } else {
                        redir->loc_fd = *rdwd - '0';
                        rdwd++;
                }
                rdwd++;

                char last = *rdwd;
                free (o_rdwd);

                if (last == '\0' || last == '+' || last == '*'
                                || last == '^') {     // out to file
                        if (last == '+') {
                                redir->mode = RD_APP;
                        } else if (last == '*') {
                                redir->mode = RD_PRE;
                        } else if (last == '^') {
                                redir->mode = RD_REV;
                        }

                        if (i == job->argc) {
                                print_err ("Missing redirect "
                                           "destination.");
                                free (redir);
                                return;
                        }
                        redir->name = vcombine_str(0, 1, job->argv[i]);
                        rm_element (job->argv, job->argm, i, &(job->argc));
                } else {        // fd replacement
                        redir->mode = RD_REP;
                        redir->fd = last - '0';
                }
        }

        q_push (job->rd_queue, redir);
}

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
        job->rd_queue = q_make();
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
                char fc = *(job->argv[i]);
                char lc = (job->argv[i])[strlen(job->argv[i])-1];
                char blc = (job->argv[i])[strlen(job->argv[i])-2];

                int j;
                int rsum = 0;
                for (j = 0; j < strlen(job->argv[i]); j++) {
                        rsum += (job->argm[i])[j];
                }

                if (!rsum) {
                        if (fc == '<' && (lc == '-' || lc == '~')) {
                                redirect (job, i);
                        }
                        if ((fc == '-' || fc == '~') && 
                                   (lc == '>' || blc == '>')) {
                                redirect (job, i);
                        }
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
