#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <linux/limits.h>

#include "exec.h"
#include "debug.h"
#include "def.h"

int try_exec(char **argv, int no_exec)
{
        if (no_exec) {
                if (access(argv[0], X_OK) == 0) return 0;
        } else {
                if (execve(argv[0], argv, environ) >= 0) return 0;
        }
        FILE *fp;
        char path[PATH_MAX] = "";
        strcat(path, "/usr/bin/which \"");
        strcat(path, argv[0]);
        strcat(path, "\" 2> /dev/null");
        fp = popen (path, "r");

        if (fp == NULL) {
                if (!no_exec) printf("error finding path\n");
                return -1;
        }

        if (fgets (path, PATH_MAX, fp) != NULL) {
                path[strlen(path) - 1] = '\0';
        } else {
                if (!no_exec) printf("cmd '%s' not found.\n", argv[0]);
                pclose(fp);
                return -1;
        }

        int status = pclose(fp);
        if (status == -1) {
                if (!no_exec) printf("error finding path (2)\n");
                return -1;
        }

        if (!no_exec) printpath(path);
        if (no_exec || execve(path, argv, environ) >= 0) return 0;
        else return -1;
}
