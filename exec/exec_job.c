#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

// local includes
#include "../types/job.h"
#include "../types/m_str.h"

#include "../util/debug.h"

#include "redirect.h"
#include "var.h"
#include "env.h"

#include "builtin/builtin.h"

// self-include
#include "exec_job.h"

static int pid;
static job_j *cchain;

extern char **environ;

void fork_exec (job_j *job)
{
        int redir = setup_redirects (job);
        if (redir == -1) {
                exit (1);
        }

        char **args = calloc (job->argc+1, sizeof(char *));
        int i;
        int j = 0;
        for (i = 0; i < job->argc; i++) {
                if (job->argv[i]->str == NULL || ms_len (job->argv[i]) == 0) continue;
                args[j++] = job->argv[i]->str;
        }
        args[j] = NULL;
        execvpe (args[0], args, environ);

        if (errno == 2) {
                print_err ("Command not found.");
        } else {
                print_err_wno ("Could not execute command.", errno);
        }

        exit (1);
}

void exec_single_job (job_j *job)
{
        int dup_in = dup (STDIN_FILENO);
        int dup_out = dup (STDOUT_FILENO);

        var_eval (job);

        if (has_var ("__jpsh_debug")) {
                int i;
                for (i = 0; i < job->argc; i++) {
                        ms_print (job->argv[i], 0);
                        printf (" ");
                }
                printf ("\n");
        }

        if (!builtin(job)) {
                pid = fork();
                if (pid < 0) print_err_wno ("fork() error.", errno);
                else if (pid == 0) fork_exec (job);
                else {
                        int status = 0;
                        if (waitpid (pid, &status, 0) < 0) {
                                print_err_wno ("waitpid() error.", errno);
                        }
                        if (job->p_in != NULL)
                                close (job->p_in[0]);
                        if (job->p_out != NULL)
                                close (job->p_out[1]);
                }
        }

        if (job->p_in != NULL) {
                close (job->p_in[0]);
                close (job->p_in[1]);
        }

        dup2 (dup_in, STDIN_FILENO);
        dup2 (dup_out, STDOUT_FILENO);
        close (dup_in);
        close (dup_out);
}

void exec_job (job_j *job)
{
        if (job == NULL || job->argc == 0) return;

        // set up piping redirects
        job_j *cjob = job;
        while (cjob->p_next != NULL) {
                int *fds = malloc(2 * sizeof(int));
                if (fds == NULL && errno == ENOMEM) {
                        print_err_wno ("Malloc failure (try_exec)",
                                        errno);
                        return;
                }
                pipe (fds);
                
                cjob->p_out = fds;
                cjob->p_next->p_in = fds;

                cjob = cjob->p_next;
        }
        cchain = job;

        for (; job != NULL; job = job->p_next) {
                exec_single_job (job);
        }

        pid = 0;
}
