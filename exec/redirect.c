#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

// local includes
#include "../types/job.h"
#include "../types/m_str.h"
#include "../types/stack.h"

#include "../util/debug.h"

// self-include
#include "redirect.h"

int setup_redirects (job_j *job)
{
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

        stack *bak = s_make();
        while (s_len(job->rd_stack) > 0) {
                rd_j *redir;
                s_pop (job->rd_stack, (void **)&redir);
                s_push (bak, redir);

                int other_fd = -1;

                // TODO: implement these
                if (redir->mode == RD_FIFO || redir->mode == RD_LIT ||
                        redir->mode == RD_PRE || redir->mode == RD_REV) {
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
                        other_fd = open (ms_strip(redir->name), flags, 0644);
                        if (other_fd == -1) {
                                print_err_wno ("Could not open file.",
                                        errno);
                                return -1;
                        }
                }

                if (redir->loc_fd == -2) {      // stderr & stdout
                        dup2 (other_fd, STDOUT_FILENO);
                        dup2 (other_fd, STDERR_FILENO);
                } else {
                        dup2 (other_fd, redir->loc_fd);
                }
        }
        while (s_len(bak) > 0) {
                rd_j *redir;
                s_pop (bak, (void **)&redir);
                s_push (job->rd_stack, redir);
        }
        free (bak);

        return 0;
}
