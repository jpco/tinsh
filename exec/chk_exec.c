#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

// local includes
#include "../util/defs.h"
#include "../util/str.h"

// self-include
#include "chk_exec.h"

int chk_exec (const char *cmd)
{
        if (*cmd == '\0') return 0;
        int i;
        for (i = 0; builtins[i] != NULL; i++) {
                if (olstrcmp(cmd, builtins[i])) return 2;
        }

        if (strchr (cmd, '/')) { // absolute path
                if (opendir(cmd) == NULL) {
                        return (1 + access (cmd, X_OK));
                } else {
                        return 0;
                }
        }
        
        // relative path
        char *path = getenv ("PATH");
        if (path == NULL)
                path = "/bin:/usr/bin";
        char *cpath = path;
        char *buf = path;

        while ((buf = strchr (cpath, ':')) || 1) {
                if (buf) *buf = '\0';
                char *curr_cmd;
                if (buf == cpath) { // initial :, so current dir
                        curr_cmd = vcombine_str ('/', 2,
                                        getenv ("PWD"), cmd);
                } else if ((buf && *(buf-1) != '/') ||
                           (cpath[strlen(cpath)-1] != '/')) {
                        // then need to add '/'
                        curr_cmd = vcombine_str ('/', 2, cpath, cmd);
                } else {
                        curr_cmd = vcombine_str ('\0', 2, cpath, cmd);
                }

                if (!access (curr_cmd, X_OK)) {
                        free (curr_cmd);
                        if (buf) *buf = ':';
                        return 1;
                }

                free (curr_cmd);
                if (buf) *buf = ':';
                if (buf) {
                        cpath = buf + 1;
                } else {
                        break;
                }
        }

        return 0;
}

