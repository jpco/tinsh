#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>

#include "cd.h"
#include "def.h"

int cd(char *arg)
{
        char *pwd = getenv("PWD");

        char newwd[100];
        if (*arg == '/') {
                strcpy(newwd, arg);
        } else {
                snprintf(newwd, 100, "%s/%s", pwd, arg);
        }

        char newwd_c[PATH_MAX];
        realpath(newwd, newwd_c);

        if (setenv("PWD", newwd_c, 1) != 0) {
                int er = errno;
                printf("Error! %s\n", strerror(er));
                return 1;
        }
        if (chdir(newwd_c) != 0) {
                int er = errno;
                printf("Error (2)! %s\n", strerror(er));
                setenv("PWD", pwd, 1);
                return 1;
        }
        return 0;
}
