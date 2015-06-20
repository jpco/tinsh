#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// local includes
#include "../exec/exec_utils.h"

#include "../defs.h"
#include "../str.h"

// self-include
#include "color.h"

/**
 * ===============
 * TODO: Rewrite this to have anything to do with the
 * current eval system!!
 * ===============
 */

void color_wd (const char *wd, int first) {
        if (first) {
                int ex = chk_exec(wd);
                if (ex > 0) {
                        printf("\e[1;32m%s\e[0m ", wd);
                        return;
                }
        }
        if (!access(wd, F_OK)) {
                printf("\e[0;36m%s\e[0m ", wd);
        } else {
                printf("%s ", wd);
        }
}

void color_line (int argc, const char **argv)
{
        int i;
        if (argc == 0) return;

        for (i = 0; i < argc; i++) {
                color_wd (argv[i], (i == 0 ? 1 : 0));
        }
        printf("\n");
}

void color_line_s (const char *line)
{
        char *nline = malloc((strlen(line)+1) * sizeof(char));
        strcpy(nline, line);
        char *nnline = trim_str(nline);
        free (nline);
        nline = nnline;

        char **spl_line = split_str (nline, ' ');

        int len;
        for (len = 0; spl_line[len] != NULL; len++);
        color_line (len, (const char **)spl_line);

        free (spl_line[0]);
        free (spl_line);
        if (nline != NULL) free (nline);
}
