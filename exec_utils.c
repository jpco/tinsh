#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

// local includes
#include "defs.h"
#include "debug.h"
#include "str.h"
#include "env.h"

// self-include
#include "exec_utils.h"

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

void printjob (job_t *job)
{
        char *db = get_var ("__jpsh_debug");
        if (db == NULL) return;
        else free (db);

        fprintf (stderr, " ==== JOB ====\n");
        fprintf (stderr, "[%s] ", job->argv[0]);
        int i;
        for (i = 1; i < job->argc; i++) {
                fprintf (stderr, "%s ", job->argv[i]);
        }
        fprintf (stderr, "\n");
        if (job->bg) {
                fprintf (stderr, " - Background\n");
        }
        if (job->p_in != NULL) {
                fprintf (stderr, " - In from %d\n", job->p_in[0]);
        }
        if (job->p_out != NULL) {
                fprintf (stderr, " - Out to %d\n", job->p_out[1]);
        }
        if (job->file_in != NULL) {
                fprintf (stderr, " - File in: %s\n", job->file_in);
        }
        if (job->file_out != NULL) {
                fprintf (stderr, " - File out: %s\n", job->file_out);
        }
}

int setup_redirects (job_t *job)
{
        int cin = -1;
        int cout = -1;

        // Piping. File redirection comes afterwards since it supersedes.
        if (job->p_in != NULL) {
                close (job->p_in[1]);
                dup2 (job->p_in[0], STDIN_FILENO);
                cin = job->p_in[0];

                free (job->p_in);
        }
        if (job->p_out != NULL) {
                close (job->p_out[0]);
                dup2 (job->p_out[1], STDOUT_FILENO);
                cout = job->p_out[0];
        }

        // File redirection.
        if (job->file_in != NULL) {
                int in_fd = open (job->file_in, O_RDONLY);
                if (in_fd == -1) {
                        print_err_wno ("Could not open file for reading.",
                                        errno);
                        if (cin != -1) close (cin);
                        if (cout != -1) close (cout);
                        return -1;
                }
                if (cin != -1) close (cin);
                dup2 (in_fd, STDIN_FILENO);
                cin = in_fd;

                free (job->file_in);
        }
        if (job->file_out != NULL) {
                int out_fd = open (job->file_out,
                                   O_CREAT | O_WRONLY, 0644);
                if (out_fd == -1) {
                        print_err_wno ("Could not open file for writing.",
                                        errno);
                        if (cin != -1) close (cin);
                        if (cout != -1) close (cout);
                        return -1;
                }
                if (cout != -1) close (cout);
                dup2 (out_fd, STDOUT_FILENO);
                cout = out_fd;

                free (job->file_out);
        }

        return 0;
}
