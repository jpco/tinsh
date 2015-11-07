#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "posix/putils.h"
#include "posix/ptypes.h"
#include "types/linkedlist.h"

#include "symtable.h"
#include "ll_utils.h"

#define CMD_MAX 100

typedef enum {
    LINE_NORMAL,    // normal parsing
    LINE_COMMENT,   // we're in a comment; ignore unless it's time to leave the comment
    LINE_CONTINUE,  // newline has been escaped; append current line to buffer line & execute

    LINE_BGATHER,   // need to gather a line/block for buffer line
    LINE_IF_FALSE,  // good to jump into 'else'
    // More states here as syntax gets more 'fancy'
} line_state_t;

line_state_t lstate = LINE_NORMAL;
static char *linebuffer;

void parse_args (int argc, char **argv)
{
}

int main (int argc, char **argv)
{
    // initialize POSIX sorts of things
    posix_init ();

    // initialize symbol table
    create_symtable();

    // parse arguments
    parse_args (argc, argv);

    // read config file

    // read input/listen for input
    char *inbuf = malloc (CMD_MAX * sizeof (char));
    while (1) {
        // print prompt
        char *cwd = getcwd(NULL, 0);
        printf ("%s$ ", cwd);
        free (cwd);
        fflush (stdout);

        // get input
        memset (inbuf, 0, CMD_MAX);
        fgets (inbuf, CMD_MAX, stdin);
        if (!*inbuf) continue;

        char *cmd = strndup (inbuf, CMD_MAX);
        if (cmd[strlen(cmd) - 1] == '\n') {
            cmd[strlen(cmd) - 1] = 0;
        }

        if (!*cmd) continue;

        // TODO: fork behavior here based on line state

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
            sym_t *binpath;
resolve:
            binpath = sym_resolve (cjob->first->argv[0]);
            if (!binpath) {
                if (!retried) {
                    rehash_bins();
                    retried = 1;
                    goto resolve;
                }
                fprintf (stderr, "Command '%s' not found.\n", cjob->first->argv[0]);
                continue;
            }
            cjob->first->argv[0] = binpath->value;
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
        // refresh jobs
        do_job_notification ();
    }

    free (inbuf);
}
