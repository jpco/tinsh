#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <linux/limits.h>

#include "exec.h"
#include "debug.h"
#include "def.h"

int try_exec(char **argv)
{
        if (execve(argv[0], argv, environ) >= 0) return 0;
        char path[PATH_MAX] = "";

        FILE *fp;
        strcat(path, "/usr/bin/which ");
        strcat(path, argv[0]);
        strcat(path, " 2> /dev/null");
        fp = popen (path, "r");
        if (fp == NULL) {
                printf("error finding path\n");
                return -1;
        }

        if (fgets (path, PATH_MAX, fp) != NULL) {
                path[strlen(path) - 1] = '\0';
        } else {
                printf("cmd '%s' not found.\n", argv[0]);
                pclose(fp);
                return -1;
        }

        int status = pclose(fp);
        if (status == -1) {
                printf("error finding path (2)\n");
                return -1;
        }

        printpath(path);
        if (execve(path, argv, environ) >= 0) return 0;
        else return -1;
}
