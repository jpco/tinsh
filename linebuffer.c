#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "posix/putils.h"
#include "posix/ptypes.h"
#include "types/linkedlist.h"

#include "ll_utils.h"
#include "linebuffer.h"
#include "hist.h"

void prompt (void)
{
        char *cwd = getcwd(NULL, 0);
        printf ("%s$ ", cwd);
        free (cwd);
        fflush (stdout);
}

void get_cmd (char *cmdbuf)
{
	fgets (cmdbuf, CMD_MAX, stdin);
}

void line_loop (void)
{
    char *inbuf = malloc (CMD_MAX * sizeof (char));
    while (1) {
		prompt ();

        // get input
        memset (inbuf, 0, CMD_MAX);
		get_cmd (inbuf);
        if (!*inbuf) continue;

        char *cmd = strndup (inbuf, CMD_MAX);
        if (cmd[strlen(cmd) - 1] == '\n') {
            cmd[strlen(cmd) - 1] = 0;
        }

        if (!*cmd) continue;

        // TODO: fork behavior here based on line state
		// should we even be involving linkedlists? should do experiments
		// eval() should be in a different function/file

        linkedlist *cmd_ll = ll_make();

        // 1. word split
        word_split (cmd, cmd_ll);

        // 2. strong var/alias resolution
        // 3. command splitting/block gathering
        // 4. var resolution/shell expansion

        // create job/processes (this will end up the majority of the shell lol)
        job *cjob = calloc (sizeof (job), 1);
        cjob->command = cmd;
        cjob->stdin = STDIN_FILENO;
        cjob->stdout = STDOUT_FILENO;
        cjob->stderr = STDERR_FILENO;

        // 5. redirection
        // 6. execution
        cjob->first = calloc (sizeof (process), 1);
        cjob->first->argv = cmd_ll_to_array (cmd_ll);

        // if no '/' in bin, translate cmd_ll to bin path
        char retried = 0;
        if (!strchr (cjob->first->argv[0], '/')) {
            sym_t *execu;
resolve:
            execu = sym_resolve (cjob->first->argv[0], SYM_BINARY | SYM_BUILTIN);
            if (!execu) {
                if (!retried) {
                    rehash_bins();
                    retried = 1;
                    goto resolve;
                }
                fprintf (stderr, "Command '%s' not found.\n", cjob->first->argv[0]);
                continue;
            }
            cjob->first->wh_exec = execu;
        }

        ll_destroy (cmd_ll);

        // add job to job list
        if (!first_job) {
            first_job = cjob;
        } else {
            job *lj, *cj;
            for (cj = first_job; cj; cj = cj->next) lj = cj;
            lj->next = cjob;
        }

        // launch job
        launch_job (cjob, 1);
		hist_add (inbuf);
        // refresh jobs
        do_job_notification ();
    }

    free (inbuf);
}
