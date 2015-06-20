#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

// local includes
#include "inter/hist.h"
#include "inter/term.h"

#include "defs.h"
#include "eval.h"
#include "exec.h"
#include "env.h"
#include "var.h"
#include "str.h"
#include "linebuffer.h"
#include "debug.h"

// don't need self-include for main

int debug_line_no = 0;

// Very basic signal handler.
// TODO: this should handle more signals
void sigint_handler (int signo)
{
        sigchild (signo);
}

/**
 * Parses a file of jpsh commands. Since there's no control flow or
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
                print_err_wno ("Error interpreting file.", err);
                return;
        }

        size_t n = MAX_LINE;
        char *line = malloc ((1+n) * sizeof(char));
        if (line == NULL && errno == ENOMEM) {
                print_err ("Could not allocate memory to parse file.");
                return;
        }
        int read = getline (&line, &n, fp);
        for (; read >= 0; read = getline (&line, &n, fp)) {
                debug_line_no++;
                eval (line);
                free (line);
                line = malloc ((1+n) * sizeof(char));
        }
        free (line);
}

typedef struct {
        char *cmd;
        char *config;
        char *file;
} args_t;

/*
 * Parses arguments into jpsh.
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
        for (i = 1; i < argc; i++) {
                char *carg = argv[i];
                if (olstrcmp(carg, "-e")) {
                        if (i < argc - 1)
                                retval.cmd = argv[++i];
                } else if (olstrcmp(carg, "-c")) {
                        if (i < argc - 1)
                                retval.config = argv[++i];
                } else {
                        retval.file = carg;
                }
        }
        return retval;
}

/*
 * The main method.
 * Arguments -- jpsh [options] [command | file]
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
        if (args.file != NULL) {
                parse_file (args.file);
                return 0;
        } else if (args.cmd != NULL) {
                eval (args.cmd);
                return 0;
        }

        term_init();
        atexit (free_env);
        atexit (free_hist);
        atexit (term_exit);

        // interactivity
        while (1) {
                char *line = line_loop();

                if (*line != '\0' && !term_prep()) {
                        eval (line);
                }
                term_unprep();
        }

        return 0;
}
