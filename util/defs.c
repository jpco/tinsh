// self-include
#include <stdio.h>

#include "defs.h"
#include "vector.h"

char *flow_builtins[NUM_FLOW_BIS] = {"if", "for", "cfor", "while", "else", "with"};
char *cmd_builtins[NUM_CMD_BIS] = {"cd", "pwd", "lsvars", "lsalias", "set", "setenv", "unset", "unenv", "alias", "unalias", "color"};

char **bis = NULL;

char **get_builtins() {
        if (!bis) bis = (char **)combine ((void **)flow_builtins, (void **)cmd_builtins, NUM_FLOW_BIS, NUM_CMD_BIS);

        return bis;
}

const char separators[NUM_SEPARATORS] = {' ', '\'', ':', '{', '}', '|', '&', '>', '<', ';', '`', '(', '~', '\0'};

int is_separator (char c)
{
        int i;
        for (i = 0; i < NUM_SEPARATORS; i++) {
                if (c == separators[i]) return 1;
        }
        return 0;
}
