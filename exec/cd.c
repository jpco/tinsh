#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>

// local includes
#include "../defs.h"
#include "../debug.h"

// self-include
#include "cd.h"

int cd (const char *arg)
{
        // "cd with no args" is handled elsewhere.
        if (arg == NULL || *arg == '\0') return 1;
        char *pwd = getenv("PWD");

        // Builds the (rough) path of the passed argument.
        char newwd[PATH_MAX];
        if (*arg == '/') {
                strcpy(newwd, arg);
        } else {
                if (pwd == NULL) {
                        print_err ("PWD undefined; could not cd.");
                        return 1;
                }
                snprintf(newwd, PATH_MAX, "%s/%s", pwd, arg);
        }

        // Canonicalizes the path.
        char newwd_c[PATH_MAX];
        if (!realpath(newwd, newwd_c)) {
                int err = errno;
                print_err_wno ("Could not get realpath", err);
                return 1;
        }

        // Need to change the environment variable of PWD, AS WELL AS
        // the internal cwd dir.
        if (setenv("PWD", newwd_c, 1) != 0) {
                int er = errno;
                print_err_wno ("Could not set new PWD value", er);
                return 1;
        }
        if (chdir(newwd_c) != 0) {
                int er = errno;
                print_err_wno ("Could not internally change cwd", er);
                setenv("PWD", pwd, 1);
                return 1;
        }

        // Done.
        return 0;
}
