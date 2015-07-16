#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "m_str.h"
#include "../util/vector.h"

// self-include
#include "job.h"

static job_j *prev_job;

int redirect (job_j *job, int i)
{
        int retval = i;

        char *o_rdwd = strdup(job->argv[i]->str);
        char *rdwd = o_rdwd;
        rm_element (job->argv, i, &(job->argc));
        retval--;

        rd_j *redir = malloc(sizeof(rd_j));
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
                                return i;
                        }
                        redir->name = ms_dup(job->argv[i]);
                        rm_element (job->argv, i, &(job->argc));
                        retval--;
                } else if (*rdwd == '-') {      // file
                        free (o_rdwd);
                        redir->mode = RD_RD;

                        if (i == job->argc) {
                                print_err ("Missing redirect source.");
                                free (redir);
                                return i;
                        }
                        redir->name = ms_dup(job->argv[i]);
                        rm_element (job->argv, i, &(job->argc));
                        retval--;
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
                                return i;
                        }
                        redir->name = ms_dup(job->argv[i]);
                        rm_element (job->argv, i, &(job->argc));
                        retval--;
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
                                return i;
                        }
                        redir->name = ms_dup(job->argv[i]);
                        rm_element (job->argv, i, &(job->argc));
                        retval--;
                } else {        // fd replacement
                        redir->mode = RD_REP;
                        redir->fd = last - '0';
                }
        }

        s_push (job->rd_stack, redir);
        return retval;
}


job_j *job_form (m_str *line)
{
        // for memory safety
        m_str *ol = line;

        ms_trim (&line);
        if (ms_len(line) == 0) {
                free (ol);
                return NULL;
        }

        job_j *job = malloc(sizeof(job_j));
        if (job == NULL) {
                free (ol);
                return NULL;
        }

        // defaults
        job->bg = 0;
        job->p_prev = NULL;
        job->p_next = NULL;
        job->rd_stack = s_make();

        job->argv = ms_spl_cmd (line);
        int i;
        for (i = 0; job->argv[i] != NULL; i++);
        job->argc = i;

        // Background job?
        if (ms_strcmp(job->argv[job->argc-1], "&")) {
                job->bg = 1;
                rm_element(job->argv, (job->argc)-1,
                        &(job->argc));
        }

        // File redirection
        int i;
        for (i = 0; i < job->argc; i++) {
                char *cstr = job->argv[i]->str;
                char *cmask = job->argv[i]->mask;

                char fc = *cstr;
                char lc = cstr[strlen(cstr)-1];
                char blc = cstr[strlen(cstr)-2];

                int j;
                int rsum = 0;
                for (j = 0; j < strlen(cstr); j++) {
                        rsum += cmask[j];
                }

                if (!rsum) {
                        if (fc == '<' && (lc == '-' || lc == '~')) {
                                i = redirect (job, i--);
                        }
                        if ((fc == '-' || fc == '~') &&
                                (lc == '>' || blc == '>')) {
                                i = redirect (job, i--);
                        }
                }
        }

        // pipe from previous job?
        if (prev_job != NULL) {
                job->p_prev = prev_job;
                job->p_prev->p_next = job;
        }

        // pipe to next job?
        if (job_out != NULL) {
                prev_job = job;
        } else {
                prev_job = NULL;
        }

        return job;
}
