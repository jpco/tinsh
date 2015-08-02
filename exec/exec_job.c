#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

// local includes
#include "../types/job.h"
#include "../types/m_str.h"

#include "../util/debug.h"
#include "../util/vector.h"

#include "redirect.h"
#include "var.h"
#include "env.h"

#include "builtin/builtin.h"

// self-include
#include "exec_job.h"

static int pid;
static job_j *cchain;

extern char **environ;

int fork_exec (job_j *job)
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
                return 1;
        } else {
                return 2;
        }

        exit (1);
}

void exec_single_job (job_j *job)
{
        int dup_in = dup (STDIN_FILENO);
        int dup_out = dup (STDOUT_FILENO);

        var_eval (job);

        // aliasing
        m_str *vval = NULL;
        m_str *newval = NULL;
        int newmask = 0;
        do {
                newval = NULL;
                newmask = 0;

                var_j *newcmd;
                if ((newcmd = get_var (ms_strip (job->argv[0]))) != NULL) {
                        if (!newcmd->is_fn) {
                                newval = newcmd->v.value;
                        }
                }
                if (newval != NULL) {
                        vval = newval;
                        m_str **cmd = ms_spl_cmd (newval);
                        // prevent infinite recursion
                        if (ms_ustrcmp(cmd[0], job->argv[0])) newmask = 1;

                        rm_element (job->argv, 0, &(job->argc));
                        int i;
                        for (i = 0; cmd[i] != NULL; i++) {
                                add_element (job->argv, cmd[i], i,
                                        &(job->argc));
                        }
                        for (i = 0; i < ms_len(job->argv[0]); i++) {
                                newmask += job->argv[0]->mask[i];
                        }
                }
        } while (newval != NULL && newmask == 0);

        if (get_var ("__imp_debug")) {
                int i;
                printf("[");
                ms_print (job->argv[0], 0);
                printf("] ");
                for (i = 1; i < job->argc; i++) {
                        ms_print (job->argv[i], 0);
                        printf (" ");
                }
                printf ("\n");
        }

        // function calls

        if (!builtin(job)) {
                pid = fork();
                if (pid < 0) print_err_wno ("fork() error.", errno);
                else if (pid == 0) {
                        int err = fork_exec (job);
                        if (err == 1) {
                                if (vval != NULL) {
                                        printf ("%s\n", ms_strip (vval));
                                } else {
                                        print_err ("Var/command not found.");
                                }
                        } else {
                                print_err ("Could not execute command.");
                        }
                } else {
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
