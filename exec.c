#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

// local includes
#include "cd.h"
#include "defs.h"
#include "eval.h"
#include "str.h"
#include "env.h"
#include "debug.h"
#include "eval.h"
#include "color.h"

// self include
#include "exec.h"

static int pid;
const char *builtins[NUM_BUILTINS] = {"exit", "cd", "pwd",
        "lsvars", "lsalias", "set", "setenv", "unset",
        "unenv", "alias", "unalias", "color", NULL};

int sigchild (int signo)
{
        if (pid == 0) return 1;
        else {
                if ((kill (pid, signo)) < 0) {
                        print_err_wno ("Could not kill child", errno);
                        return 2;
                } else {
                        return 0;
                }
        }
}

int chk_exec (const char *cmd)
{
        if (*cmd == '\0') return 0;
        int i;
        for (i = 0; builtins[i] != NULL; i++) {
                if (olstrcmp(cmd, builtins[i])) return 2;
        }

        if (strchr (cmd, '/')) { // absolute path
                if (opendir(cmd) == NULL) {
                        return (1 + access (cmd, X_OK));
                } else {
                        return 0;
                }
        }
        
        // relative path
        char *path = getenv ("PATH");
        if (path == NULL)
                path = "/bin:/usr/bin";
        int len = strlen (cmd);
        int pathlen = strlen (path);
        char *cpath = path;
        char *buf = path;

        while ((buf = strchr (cpath, ':')) || 1) {
                if (buf) *buf = '\0';
                char *curr_cmd;
                if (buf == cpath) { // initial :, so current dir
                        curr_cmd = vcombine_str ('/', 2,
                                        getenv ("PWD"), cmd);
                } else if ((buf && *(buf-1) != '/') ||
                           (cpath[strlen(cpath)-1] != '/')) {
                        // then need to add '/'
                        curr_cmd = vcombine_str ('/', 2, cpath, cmd);
                } else {
                        curr_cmd = vcombine_str ('\0', 2, cpath, cmd);
                }

                if (!access (curr_cmd, X_OK)) {
                        free (curr_cmd);
                        if (buf) *buf = ':';
                        return 1;
                }

                free (curr_cmd);
                if (buf) *buf = ':';
                if (buf) {
                        cpath = buf + 1;
                } else {
                        break;
                }
        }

        return 0;
}

int builtin (int argc, const char **argv, int bg)
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

void printjob (int argc, const char **argv, int bg)
{
        char *db = get_var ("__jpsh_debug");
        if (db == NULL) return;
        else free (db);
        printf ("\e[0;35m");
        if (bg) printf ("(background) ");
        printf ("[%s] ", argv[0]);
        int i;
        for (i = 1; i < argc; i++) {
                printf("%s ", argv[i]);
        }
        printf("\e[0m\n");
}

char *subshell (char *cmd)
{
        int fds[2];
        pipe (fds);

        pid = fork();
        if (pid < 0) print_err_wno ("Fork error.", errno);
        else if (pid == 0) {
                close (fds[0]);
                dup2 (fds[1], STDOUT_FILENO);
                eval (cmd);
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

void try_exec (int argc, const char **argv, int bg)
{
        printjob (argc, argv, bg);

        if (!builtin (argc, argv, bg)) {
                int success = 0;
                int err = 0;
                pid = fork();
                if (pid < 0) print_err_wno ("Fork error.", errno);
                else if (pid == 0) {
                        success = execvpe (argv[0], argv, environ);
                        err = errno;
                        if (err == 2) {
                                print_err ("Command not found.");
                        } else {
                                print_err_wno ("Execution error.", err);
                        }

                        // clean up memory!
                        free_ceval();
                }
                if (!bg || !success) {
                        int status = 0;
                        if (waitpid (pid, &status, 0) < 0) {
                                int err = errno;
                                if (err != 10) {
                                        dbg_print_err_wno ("Waitpid error.",errno);
                                }
                                exit (1);
                        }
                }
                pid = 0;
        }
}
