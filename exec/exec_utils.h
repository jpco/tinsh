#ifndef JPSH_EXEC_UTILS_H
#define JPSH_EXEC_UTILS_H

// The number of builtins (currently)
// implemented.
#define NUM_BUILTINS 13

// Need this for job_t
#include "../defs.h"

// The builtins (defined in exec_utils.c and exec.c)
const char *builtins[NUM_BUILTINS];
// The pid currently being executed.
extern int pid;

// Sends a signal to the child process.
int sigchild (int signo);

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

/**
 * Prints the current job, including piping and redirection
 * information.
 */
void printjob (job_t *job);

/**
 * Sets up the redirects and such based on the information in
 * job.
 */
int setup_redirects (job_t *job);

#endif
