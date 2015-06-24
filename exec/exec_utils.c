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
#include "../eval/stack.h"

#include "../defs.h"
#include "../debug.h"
#include "../str.h"
#include "../var.h"

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

        stack *lo_copy = s_make();
        while (s_len(job->rd_stack) != 0) {
                rd_t *redir;
                s_pop(job->rd_stack, (void **)&redir);
                fprintf (stderr, " - Redirect\n");
                fprintf (stderr, " - - local fd: %d\n",
                         redir->loc_fd);
                switch (redir->mode) {
                        case RD_FIFO:
                                fprintf (stderr, " - - type: FIFO\n");
                                break;
                        case RD_APP:
                                fprintf (stderr, " - - type: append\n");
                                break;
                        case RD_PRE:
                                fprintf (stderr, " - - type: prepend\n");
                                break;
                        case RD_REV:
                                fprintf (stderr, " - - type: reverse\n");
                                break;
                        case RD_OW:
                                fprintf (stderr, " - - type: overwrite\n");
                                break;
                        case RD_REP:
                                fprintf (stderr, " - - type: replicate\n");
                                break;
                        case RD_LIT:
                                fprintf (stderr, " - - type: literal\n");
                                break;
                        case RD_RD:
                                fprintf (stderr, " - - type: read\n");
                                break;
                }
                if (redir->mode == RD_RD || redir->mode == RD_REP) {
                        fprintf (stderr, " - - fd: %d\n", redir->fd);
                } else {
                        fprintf (stderr, " - - name: %s\n", redir->name);
                }

                s_push(lo_copy, redir);
        }
        while (s_len(lo_copy) > 0) {
                rd_t *redir;
                s_pop(lo_copy, (void **)&redir);
                s_push(job->rd_stack, redir);
        }
        free (lo_copy);
}

int setup_redirects (job_t *job)
{
        // Piping. File redirection comes afterwards since it supersedes.
        if (job->p_in != NULL) {
                close (job->p_in[1]);
                dup2 (job->p_in[0], STDIN_FILENO);

                free (job->p_in);
                job->p_in = NULL;
        }
        if (job->p_out != NULL) {
                close (job->p_out[0]);
                dup2 (job->p_out[1], STDOUT_FILENO);
        }

        while (s_len(job->rd_stack) > 0) {
                rd_t *redir;
                s_pop (job->rd_stack, (void **)&redir);

                int other_fd = -1;

                // TODO: implement these
                if (redir->mode == RD_FIFO || redir->mode == RD_LIT ||
                        redir->mode == RD_PRE || redir->mode == RD_REV) {
                        free (redir->name);
                        free (redir);
                        return 0;
                }
                
                if (redir->mode == RD_REP) {
                        other_fd = redir->fd;
                } else {
                        // time to open a file
                        int flags = 0;
                        if (redir->mode == RD_APP) {
                                flags = O_CREAT | O_WRONLY | O_APPEND;
                        } else if (redir->mode == RD_OW) {
                                flags = O_CREAT | O_WRONLY | O_TRUNC;
                        } else if (redir->mode == RD_RD) {
                                flags = O_RDONLY;
                        }

                        // TODO: should probably not have "0644" hardcoded in
                        other_fd = open (redir->name, flags, 0644);
                        if (other_fd == -1) {
                                print_err_wno ("Could not open file.",
                                        errno);
                                free (redir->name);
                                free (redir);
                                return -1;
                        }
                }

                if (redir->loc_fd == -2) {      // stderr & stdout
                        dup2 (other_fd, STDOUT_FILENO);
                        dup2 (other_fd, STDERR_FILENO);
                } else {
                        dup2 (other_fd, redir->loc_fd);
                }

                free (redir->name);
                free (redir);
        }

        return 0;
}
