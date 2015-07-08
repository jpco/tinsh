#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

// local includes
#include "exec/cd.h"
#include "exec/exec_utils.h"
#include "exec/func.h"

#include "eval/stack.h"

#include "inter/color.h"

#include "defs.h"
#include "eval.h"
#include "str.h"
#include "var.h"
#include "debug.h"

// self include
#include "exec.h"

int pid;
static job_t *cchain;

void free_cchain (void)
{
        job_t *cjob = cchain;
        while (cjob != NULL) {
                int i;
                for (i = 0; i < cjob->argc; i++) {
                        free (cjob->argv[i]);
                        free (cjob->argm[i]);
                }
                free (cjob->argv);
                free (cjob->argm);
                free (cjob->p_in);
                free (cjob->p_prev);

                while (s_len(cjob->rd_stack) > 0) {
                        rd_t *loc_rd;
                        s_pop(cjob->rd_stack, (void **)&loc_rd);
                        free (loc_rd->name);
                        free (loc_rd);
                }
                free (cjob->rd_stack);

                if (cjob->p_next == NULL) {
                        free (cjob);
                        cjob = NULL;
                        cchain = NULL;
                } else {
                        cjob = cjob->p_next;
                }
        }
}

char *subshell (char *cmd, char *mask)
{
        cmd = strdup (cmd);

        int fds[2];
        if (pipe (fds) < 0) {
                print_err_wno ("Could not set up pipe (subshell)",
                                errno);
                return NULL;
        }

        pid = fork();
        if (pid < 0) {
                print_err_wno ("Fork error.", errno);
                return NULL;
        } else if (pid == 0) {
                do {
                        close (fds[0]);
                } while (errno == EINTR);

                dup2 (fds[1], STDOUT_FILENO);
                eval_m (cmd, mask);

                do {
                        close (fds[1]);
                } while (errno == EINTR);
                exit (0);
        } else {
                close (fds[1]);
                int status = 0;
                if (waitpid (pid, &status, 0) < 0) {
                        dbg_print_err_wno ("Waitpid error.", errno);
                        exit (1);
                }

                char *output = calloc((SUBSH_LEN+1), sizeof(char));
                if (output == NULL) {
                        print_err_wno ("Calloc failure (subshell)", errno);
                        return NULL;
                }

                errno = 0;
                while (read (fds[0], output, SUBSH_LEN) > 0);
                if (errno != 0) {
                        print_err_wno ("Read failure (subshell)", errno);
                }

                close (fds[0]);
                return output;
        }
}

void fork_exec (job_t *job)
{
        int redir = setup_redirects (job);

        if (redir == -1) {
                exit (1);
        }

        execvpe ((char *)job->argv[0],
                        (char **)job->argv,
                        (char **)environ);

        if (errno == 2) {
                print_err ("Command not found.");
        } else {
                print_err_wno ("Could not execute command.", errno);
        }

        exit(1);
}

void try_exec (job_t *job)
{
        if (job->argc == 0) return;

        const char **argv = (const char **)job->argv;

        if (job->p_prev == NULL) {
                job_t *cjob = job;
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
        }
        atexit (free_cchain);

        printjob (job);

        int dup_in = dup (STDIN_FILENO);
        int dup_out = dup (STDOUT_FILENO);

        if (!func(job->argc, argv)) {
                pid = fork();
                if (pid < 0) print_err_wno ("fork() error.", errno);
                else if (pid == 0) fork_exec(job);
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

        if (job->p_next == NULL) {
                free_cchain();
        }

        pid = 0;
}
