#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../posix/putils.h"
#include "../posix/ptypes.h"
#include "../types/linkedlist.h"

#include "hist.h"
#include "../ll_utils.h"

#include "buffer.h"

void prompt (void)
{
        char *cwd = getcwd(NULL, 0);
        printf ("%s$ ", cwd);
        free (cwd);
        fflush (stdout);
}

void get_cmd (char *cmdbuf)
{
	fgets (cmdbuf, CMD_MAX, stdin);
}
