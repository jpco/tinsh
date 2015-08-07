#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

// local includes
#include "../types/job_queue.h"
#include "../types/queue.h"

#include "../eval/eval.h"

#include "../util/debug.h"
#include "../util/defs.h"

#include "env.h"
#include "exec.h"

// self-include
#include "subshell.h"

static int pid;

char *subexec (char *cmd)
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
                
                queue *c = q_make();
                q_push (c, cmd);
                job_queue *jq = eval (c);
                set_var ("__tin_ignorenf", "true");
                var_j *db = get_var ("__tin_debug");
                unset_var ("__tin_debug");
                exec (jq);
                if (db) set_var ("__tin_debug", "true");

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

        return NULL;
}
