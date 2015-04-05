#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// terminal attribute includes
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

// local includes
#include "defs.h"
#include "eval.h"
#include "env.h"
#include "str.h"
#include "linebuffer.h"

// don't need self-include for main

// term config files
static int fd;
static struct termios config;
static tcflag_t o_stty;
static tcflag_t m_stty;


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
                printf ("jpsh: Error interpreting file: %s\n", strerror(err));
                return;
        }

        size_t n = MAX_LINE;
        char *rline = malloc ((1+n)*sizeof(char));
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
                printf(" - arg %s\n", carg);
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
 * Pre-evaluation function for setting up environment before
 * parsing & executing a command. (Generally, for un-fscking
 * up things important for the shell which mess with normal
 * execution)
 * Argument:
 *  - line: the cmdline which is to be evaluated
 * Returns:
 *  - 0 on success, ready to evaluate & execute
 *  - non-zero otherwise.
 */
int pre_eval (char *line)
{
        config.c_lflag = o_stty;
        if (tcsetattr(fd, TCSAFLUSH, &config) < 0) {
                printf("tcsetattr failed\n");
                return -1;
        }

        return 0;
}

/*
 * Post-evaluation function for re-establishing shell environment.
 */
void post_eval (char *line)
{
//        free (line);

        config.c_lflag = m_stty;
        if (tcsetattr(fd, TCSAFLUSH, &config) < 0) {
                printf("tcsetattr failed\n");
                exit (1);
        }
}

/**
 * Prepares the shell for interaction
 */
void init_shell (void)
{
        // turn off canonical mode
        const char *device = "/dev/tty";
        fd = open (device, O_RDWR);
        int err = errno;
        if (fd == -1) {
                printf ("Failed to set shell attributes (1): %s\n",
                                strerror(err));
                exit(1);
        }
        int retcode = tcgetattr(fd, &config);
        err = errno;
        if (retcode < 0) {
                printf ("Failed to set shell attributes (2): %s\n",
                                strerror(err));
                exit(1);
        }

        o_stty = config.c_lflag;
        config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
        m_stty = config.c_lflag;

        retcode = tcsetattr(fd, TCSAFLUSH, &config);
        err = errno;
        if (retcode < 0) {
                printf ("Failed to set shell attributes (3): %s\n",
                                strerror(err));
                exit (1);
        }
}

void pre_exit (void)
{
        config.c_lflag = o_stty;
        if (tcsetattr(fd, TCSAFLUSH, &config) < 0)
                printf("tcsetattr failed\n");

        close (fd);
        free_env();
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
        if (exec != NULL) { // exec!
                // pre_eval?????!!!!
                eval (exec);
        }
        if (!(argcode & 2)) return 0;

        init_shell();
        atexit (pre_exit);
        if (signal (SIGINT, sigint_handler) == SIG_ERR) {
                printf ("Can't catch SIGINT\n");
        }

        // interactivity
        while (1) {
                char *line = line_loop();

                if (*line != '\0' && !pre_eval (line)) {
                        eval (line);
                }
                post_eval (line);
        }

        return 0;
}
