#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

// local includes
#include "inter/hist.h"
#include "inter/term.h"
#include "inter/buffer.h"

#include "eval/eval.h"

#include "types/var.h"
#include "types/queue.h"

#include "exec/exec.h"
#include "exec/env.h"

#include "util/str.h"
#include "util/debug.h"
#include "util/defs.h"

// don't need self-include for main

int debug_line_no = 0;

// Very basic signal handler.
// TODO: this should handle more signals
/* void sigint_handler (int signo)
{
        sigchild (signo);
} */

/**
 * Parses a file of impsh commands. Since there's no control flow or
 * anything, it's not super useful.
 * 
 * Arg: the path of the file to open.
 *
 * TODO: when this gets more complex, separate into its own file.
 */
void parse_file (char *fstr)
{
        FILE *fp = fopen (fstr, "r");
        int err = errno;
        if (fp == NULL) {
                print_err_wno ("Error opening file.", err);
                return;
        }

        size_t n = MAX_LINE;
        char *line = malloc ((1+n) * sizeof(char));
        queue *lines = q_make();

        int read = getline (&line, &n, fp);
        for (; read >= 0; read = getline (&line, &n, fp)) {
                q_push (lines, line);
                line = malloc ((1+n) * sizeof(char));
        }
        fclose (fp);

        job_queue *jq = eval (lines);
        exec (jq);
}

typedef struct {
        char *cmd;
        char *config;
        char *file;
        char **fargs;
} args_t;

/*
 * Parses arguments into impsh.
 * Arguments:
 *  - argc = main argc
 *  - argv = main argv
 *  - cmd  = a pointer which will have the cmd for main to evaluate, or
 *           NULL otherwise
 * Returns:
 *  - a combination of the following masks:
 *  - binary 1 indicates that the default env file needs to be parsed
 *  - binary 10 indicates that the program is to be interactive
 */
args_t parse_args(int argc, char **argv)
{
        args_t retval;
        retval.cmd = NULL;
        retval.config = NULL;
        retval.file = NULL;

        int i;

        char *fargs[argc];
        int fargc = 0;
        fargs[0] = NULL;

        char fmode = 0;
        for (i = 1; i < argc; i++) {
                char *carg = argv[i];
                if (olstrcmp(carg, "-e")) {
                        if (i < argc - 1)
                                retval.cmd = argv[++i];
                } else if (olstrcmp(carg, "-c")) {
                        if (i < argc - 1)
                                retval.config = argv[++i];
                } else {
                        if (!fmode) {
                                retval.file = carg;
                                fmode = 1;
                        } else {
                                fargs[fargc++] = carg;
                                fargs[fargc] = NULL;
                        }
                }
        }

        retval.fargs = malloc(sizeof(fargs));
        memcpy (retval.fargs, fargs, sizeof(fargs));

        return retval;
}

/*
 * The main method.
 * Arguments -- impsh [options] [command | file]
 *  -c ./configfile: uses ./configfile as the config file
 *  -e [command]: executes the command and exits.
 *  [file]: executes the file line-by-line and exits.
 * Returns:
 *  - 0 without errors (no errors implemented yet)
 */
int main (int argc, char **argv)
{
//        signal(SIGINT, sigint_handler);
        args_t args = parse_args(argc, argv);

        if (args.config != NULL) {
                init_envp(args.config);
        } else {
                init_env();
        }

        atexit (free_env);

        if (args.file != NULL) {
                int i;
                for (i = 0; args.fargs[i] != NULL; i++) {
                        char idx[5];
                        snprintf (idx, 5, "_%d", i+1);
                        set_var (idx, args.fargs[i]);
                }
                char idx[4];
                snprintf (idx, 4, "%d", i);
                set_var ("_n", idx);
                set_var ("_", args.file);

                parse_file (args.file);
                return 0;
        } else if (args.cmd != NULL) {
                queue *smq = q_make();
                q_push(smq, args.cmd);
                job_queue *jq = eval (smq);
                exec (jq);
                return 0;
        }

        term_init();
        atexit (free_hist);
        atexit (term_exit);

        // interactivity
        while (1) {
                char *line = line_loop();

                if (*line != '\0' && term_prep() == 0) {
                        // TODO: this will need to understand unresolved
                        // blocks/lines
                        queue *smq = q_make();
                        q_push (smq, line);
                        job_queue *jq = eval (smq);
                        exec (jq);
                }
                term_unprep();
        }

        return 0;
}
