#ifndef JPSH_EXEC_H
#define JPSH_EXEC_H
#define NUM_BUILTINS 13

const char *builtins[NUM_BUILTINS];

/**
 * Kills the child process.
 * Returns 0 if the child was successfully killed, nonzero otherwise.
 */
int sigchild (int signo);

/**
 * Runs a subshell and returns the stdout output of the
 * subshell.
 */
char *subshell (char *cmd);

/**
 * Executes the builtin functions within jpsh.
 *
 * Arguments:
 *  - argc: the number of args
 *  - argv: the arguments
 *
 * Returns:
 *  - 0 if there is no builtin with this name
 *  - 1 if the builtin successfully executed
 *  - 2 if there was an error
 */
void try_exec (int argc, const char **argv, int bg);

/**
 * Checks if "cmd" can be executed, given the current
 * PATH and builtins. 
 *
 * Returns 2 for a builtin, 1 for an executable file,
 * 0 for nothing executable.
 *
 * NOTE: Probably not 100% accurate.
 *
 * WARNING!! Don't use directly prior to executing!!
 * Can lead to nasty vulnerabilities and bugs!!
 */
int chk_exec (const char *cmd);

#endif
