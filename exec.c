#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

// local includes
#include "cd.h"
#include "defs.h"
#include "eval.h"
#include "str.h"
#include "env.h"
#include "debug.h"
#include "eval.h"
#include "color.h"
#include "exec_utils.h"

// self include
#include "exec.h"

int pid;

int builtin (int argc, const char **argv)
{
        if (strchr(argv[0], '/')) return 0;

        if (olstrcmp (argv[0], "exit")) {
                atexit (free_ceval);
                exit (0);
        } else if (olstrcmp (argv[0], "cd")) {
                if (argv[1] == NULL) { // going HOME
                        if (cd (getenv("HOME")) > 0) return 2;
                }
                if (cd (argv[1]) > 0) return 2;
        } else if (olstrcmp (argv[0], "pwd")) {
                printf("%s\n", getenv ("PWD"));
        } else if (olstrcmp (argv[0], "lsvars")) {
                ls_vars();
        } else if (olstrcmp (argv[0], "lsalias")) {
                ls_alias();
        } else if (olstrcmp (argv[0], "set")) {
                if (argc == 1) {
                        print_err ("Too few args.\n");
                } else {
                        char *keynval = combine_str(argv+1, argc-1, '\0');
                        if (strchr(keynval, '=') == NULL) {
                                set_var (keynval, NULL);
                        } else {
                                char **key_val = split_str (keynval, '=');
                                set_var (key_val[0], key_val[1]);
                        }
                }
        } else if (olstrcmp (argv[0], "setenv")) {
                if (argc == 1) {
                        print_err ("Too few args.\n");
                } else {
                        char *keynval = combine_str(argv+1, argc-1, '\0');
                        if (strchr(keynval, '=') == NULL) {
                                print_err ("Malformed \"setenv\".");
                        } else {
                                char **key_val = split_str (keynval, '=');
                                setenv (key_val[0], key_val[1], 1);
                        }
                }
        } else if (olstrcmp (argv[0], "unset")) {
                if (argc < 2) {
                        print_err ("Too few args.\n");
                } else {
                        unset_var (argv[1]);
                }
        } else if (olstrcmp (argv[0], "unenv")) {
                if (argc < 2) {
                        print_err ("Too few args.\n");
                } else {
                        unsetenv (argv[1]);
                }
        } else if (olstrcmp (argv[0], "alias")) {
                if (argc < 3) {
                        print_err ("Too few args.\n");
                } else {
                        set_alias (argv[1], argv[2]);
                }
        } else if (olstrcmp (argv[0], "unalias")) {
                if (argc < 2) {
                        print_err ("Too few args.\n");
                } else {
                        unset_alias (argv[1]);
                }
        } else if (olstrcmp (argv[0], "color")) {
                if (argc < 2) {
                        print_err ("Too few args.\n");
                } else {
                        color_line (argc-1, argv+1);
                }
        } else {
                return 0;
        }

        return 1;
}

char *subshell (char *cmd, char *mask)
{
        // just use popen
        int fds[2];
        pipe (fds);

        pid = fork();
        if (pid < 0) print_err_wno ("Fork error.", errno);
        else if (pid == 0) {
                close (fds[0]);
                dup2 (fds[1], STDOUT_FILENO);
                eval_m (cmd, mask);
                close (fds[1]);
                exit (0);
        } else {
                close (fds[1]);
                int status = 0;
                if (waitpid (pid, &status, 0) < 0) {
                        dbg_print_err_wno ("Waitpid error.", errno);
                        exit (1);
                }

                char *output = malloc((SUBSH_LEN+1) * sizeof(char));
                while (read (fds[0], output, SUBSH_LEN) > 0);

                return output;
        }
        return NULL;
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

        print_err_wno ("Could not execute command.", errno);

        exit(1);
}

void try_exec (job_t *job)
{
        const char **argv = (const char **)job->argv;

        if (job->p_prev == NULL) {
                job_t *cjob = job;
                while (cjob->p_next != NULL) {
                        int *fds = malloc(2 * sizeof(int));
                        pipe (fds);

                        cjob->p_out = fds;
                        cjob->p_next->p_in = fds;

                        cjob = cjob->p_next;
                }
        }

        printjob (job);

        int dup_in = dup (STDIN_FILENO);
        int dup_out = dup (STDOUT_FILENO);

        if (!builtin(job->argc, argv)) {

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
                free (job->p_in);
        }

        free (job->file_in);
        free (job->file_out);

        dup2 (dup_in, STDIN_FILENO);
        dup2 (dup_out, STDOUT_FILENO);
        close (dup_in);
        close (dup_out);

        pid = 0;
}
