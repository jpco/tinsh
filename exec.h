#ifndef JPSH_EXEC_H
#define JPSH_EXEC_H

#include "defs.h"
#include "exec/exec_utils.h"

/**
 * Kills the child process.
 * Returns 0 if the child was successfully killed, nonzero otherwise.
 */
int sigchild (int signo);

/**
 * Runs a subshell and returns the stdout output of the
 * subshell.
 */
char *subshell (char *cmd, char *mask);

void try_exec (job_t *job);

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
