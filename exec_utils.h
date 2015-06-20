#ifndef _JPSH_EXEC_UTILS_H_
#define _JPSH_EXEC_UTILS_H_

#define NUM_BUILTINS 13

#include "defs.h"

const char *builtins[NUM_BUILTINS];
extern int pid;

int sigchild (int signo);
int chk_exec (const char *cmd);
void printjob (job_t *job);
int setup_redirects (job_t *job);

#endif
