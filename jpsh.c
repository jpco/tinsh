#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

// local includes
#include "defs.h"
#include "eval.h"
#include "exec.h"
#include "env.h"
#include "str.h"
#include "linebuffer.h"
#include "hist.h"
#include "term.h"
#include "debug.h"

// don't need self-include for main

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
        char *rline = malloc ((1+n)*sizeof(char));
        if (rline == NULL && errno == ENOMEM) {
                print_err ("Could not allocate memory to parse file.");
                return;
        }
        char *line;
        int read = getline (&rline, &n, fp);
        for (; read >= 0; read = getline (&rline, &n, fp)) {
                line = trim_str (rline);
                if (*line != '#' && *line != '\0')
                        eval (line);
                free (line);
        }
}

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
int parse_args(int argc, char **argv, char **cmd)
{
        int retval = 3;
        int i;
        for (i = 1; i < argc; i++) {
                char *carg = argv[i];
                if (strcmp(carg, "-e") == 0) {
                        // maybe pre_eval?
                        if (i < argc - 1)
                                *cmd = argv[++i];
                        retval &= ~2;
                } else if (strcmp(carg, "-c") == 0) {
                        if (i < argc - 1)
                                init_envp (argv[++i]);
                        retval &= ~1;
                } else {
                        parse_file (carg);
                        retval &= ~2;
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
        int argcode = 3;
        char *exec = NULL;
        if (argc > 1) {
                argcode = parse_args(argc, argv, &exec);
        }
        if (argcode & 1) {
                init_env();
        }
        if (exec != NULL) {
                eval (exec);
        }
        if (!(argcode & 2)) return 0;

        term_init();
        atexit (free_env);
        atexit (free_hist);
        atexit (term_exit);
        if (signal (SIGINT, sigint_handler) == SIG_ERR) {
                print_err ("Cannot catch SIGINT.");
        }

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
