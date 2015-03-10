#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtin.h"
#include "debug.h"
#include "parseline.h"
#include "def.h"
#include "eval.h"
#include "exec.h"

void eval (char *cmdline)
{
        char *argv[MAX_ARGS];
        int bg = parseline(cmdline, argv);
        pid_t pid;

        if (argv[0] == NULL) return;

        if (DEBUG) printjob (bg, argv);

        if (!builtin(argv)) {
                pid = fork();
                int success;
                if (pid < 0) {
                        printf("Fork error\n");
                } else if (pid == 0) {
                        success = try_exec(argv);
                }

                if (!bg || !success) {
                        int status;
                        if (waitpid(pid, &status, 0) < 0) {
//                              printf("waitpid error\n");
                                exit(1);
                        }
                }
        }
        return;
}
