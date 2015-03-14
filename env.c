#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "str.h"

void default_jpsh_env()
{
        setenv("SHELL", "/home/jpco/bin/jpsh", 1);
}

void jpsh_env()
{
        FILE *ef;
        ef = fopen("/home/jpco/.jpshrc", "r");
        if (ef == NULL) {
                printf("could not find ~/.jpshrc, using defaults...");
                default_jpsh_env();
                return;
        }

        char *line;
        while (fgets (line, 500, ef) != NULL) {
                char *spline[2];
                split_str(line, spline, '=');
                setenv(spline[0], spline[1], 1);
        }
}
