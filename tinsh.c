#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "posix/putils.h"
#include "posix/ptypes.h"

#define CMD_MAX 100

char **split_str (char *in)
{
//    fprintf (stderr, "cmd: %s\n", in);
    int numelts = 2;
    char *indup = in;
    while ((indup = strchr (indup, ' '))) { numelts++; indup++; }

    char **argv = malloc (numelts * sizeof (char *));
    argv[numelts - 1] = NULL;
   
    char *lindup = in;
    indup = in;
    int i;
//    fprintf (stderr, "argv: ");
    for (i = 0; (indup = strchr (indup, ' ')); i++) {
        *indup = 0;
        argv[i] = strdup (lindup);
        *indup = ' ';
        lindup = ++indup;
//        fprintf (stderr, "[%s] ", argv[i]);
    }
    argv[i] = strdup (lindup);
//    fprintf (stderr, "[%s] ", argv[i]);

//    fprintf (stderr, "\n");

    return argv;
}

int main (void)
{
    // initialize
    // parse arguments
    init_shell ();
    // read input/listen for input

    char *inbuf = malloc (CMD_MAX * sizeof (char));
    while (1) {
        // get input
        memset (inbuf, 0, CMD_MAX);
        fgets (inbuf, CMD_MAX, stdin);
        char *cmd = strndup (inbuf, CMD_MAX);
        cmd[strlen(cmd) - 1] = 0;   // kill newline

        // create job/processes (this will be the majority of the shell lol)
        job *cjob = calloc (sizeof (job), 1);
        cjob->command = cmd;
        cjob->stdin = STDIN_FILENO;
        cjob->stdout = STDOUT_FILENO;
        cjob->stderr = STDERR_FILENO;

        cjob->first = calloc (sizeof (process), 1);
        cjob->first->argv = split_str (cmd);

        // add job to job list
        if (!first_job) {
            first_job = cjob;
        } else {
            job *lj, *cj;
            for (cj = first_job; cj; cj = cj->next) lj = cj;
            lj->next = cjob;
        }

        // launch job
        launch_job (cjob, 0);
        // refresh jobs
        do_job_notification ();
    }
}
