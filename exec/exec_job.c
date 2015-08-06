#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

// local includes
#include "../types/job.h"
#include "../types/m_str.h"
#include "../types/scope.h"

#include "../util/debug.h"
#include "../util/vector.h"
#include "../util/str.h"

#include "redirect.h"
#include "var.h"
#include "env.h"

#include "builtin/builtin.h"
#include "builtin/flow.h"

#include "exec.h"

// self-include
#include "exec_job.h"

static int pid;
static job_j *cchain;

extern char **environ;
extern scope_j *cscope;

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

        // flow builtins get evaluated here
        int fb_status = flow_builtin(job);
        if (fb_status) {
                if (fb_status == 1) {
                        // if && evaluated true

                } else if (fb_status == 2) {
                        // if && evaluated false

                }
                return;
        }

        var_eval (job);

        // debug printing
        if (get_var ("__tin_debug")) {
                int i;
                fprintf(stderr, "[%s] ", ms_strip (job->argv[0]));
                for (i = 1; i < job->argc; i++) {
                        fprintf (stderr, "%s ", ms_strip (job->argv[i]));
                }
                fprintf (stderr, "\n");
        }

        // TODO: function calls go here

        // builtins & forking go here
        if (!builtin(job)) {
                pid = fork();
                if (pid < 0) print_err_wno ("fork() error.", errno);
                else if (pid == 0) {
                        int err = fork_exec (job);
                        if (err == 1) {
                                if (vval != NULL || get_var ("__tin_ignorenf")) {
                                        // handle empty returns
                                        if (!ms_strip (vval)) {
                                                printf("\n");
                                        } else {
                                                printf("%s\n",
                                                        ms_strip(vval));
                                        }
                                        exit (0);
                                } else {
                                        fprintf (stderr, "For fn %s\n", job->argv[0]->str);
                                        print_err ("Var/command not found.");
                                }
                        } else {
                                print_err ("Could not execute command.");
                        }
                        exit (err);
                } else {
                        int status = 0;
                        if (waitpid (pid, &status, 0) < 0) {
                                print_err_wno ("waitpid() error.", errno);
                        }
                        if (job->p_in != NULL)
                                close (job->p_in[0]);
                        if (job->p_out != NULL)
                                close (job->p_out[1]);
                        char strstatus[4];
                        snprintf (strstatus, 3, "%d", status);
                        set_var ("_?", strstatus);
                }
        }

        if (job->block != NULL) {
                cscope = new_scope (cscope);
                exec (job->block);
                cscope = leave_scope (cscope);
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

job_j *job_dup (job_j *job);
void destroy_job (job_j *job);

void exec_job (job_j *orig_job)
{
        job_j *job = job_dup (orig_job);

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
                if (job->argc > 0) {
                        exec_single_job (job);
                }
        }

        pid = 0;

        destroy_job (job);
}

void destroy_job (job_j *job)
{
        while (job) {
                if (job->p_in) free (job->p_in);
                if (job->p_out) free (job->p_out);

                int i;
                for (i = 0; i < job->argc; i++) {
                        ms_free (job->argv[i]);
                }
                free (job->argv);
                job_j *fjob = job;
                job = job->p_next;

                free (fjob);
        }
}

job_j *job_dup (job_j *job)
{
        job_j *currdup = NULL;
        while (job) {
                job_j *newdup = malloc(sizeof(job_j));
                newdup->argc = job->argc;
                newdup->argv = malloc(sizeof(m_str *) * newdup->argc);
                int i;
                for (i = 0; i < newdup->argc; i++) {
                        newdup->argv[i] = ms_dup (job->argv[i]);
                }

                newdup->bg = job->bg;

                // have to make sure we get p_next pointing to duplicates
                // and not originals
                newdup->p_prev = currdup;
                newdup->p_next = NULL;
                if (newdup->p_prev) newdup->p_prev->p_next = newdup;

                // these don't need to be duplicated (hopefully)
                newdup->block = job->block;
                newdup->rd_stack = job->rd_stack;

                // initialize these
                newdup->p_in = NULL;
                newdup->p_out = NULL;

                currdup = newdup;
                job = job->p_next;
        }

        // back up to first job in chain
        while (currdup && currdup->p_prev) currdup = currdup->p_prev;

        return currdup;
}
