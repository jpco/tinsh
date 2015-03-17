#include <stdlib.h>

#include "parseline.h"
#include "def.h"
#include "str.h"

int parseline (char *cmdline, char **argv)
{
        int argc = split_str(cmdline, argv, ' ', 1);

        if (argc == 0)
                return 1;

        int bg;
        if ((bg = (*argv[argc-1] == '&')) != 0)
                argv[--argc] = NULL;

        return bg;
}

