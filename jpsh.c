#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "parseline.h"
#include "def.h"

char *in_m;

void prompt()
{
        printf("%d %s$ ", getpid(), getenv("PWD"));
}

void free_cmd()
{
        free (in_m);        
}

void init()
{
        setenv("SHELL", "/home/jpco/git/jpsh/jpsh", 1);
}

int main (void)
{
        char in[INPUT_MAX_LEN];

        init();

        while (1) {
                prompt();
                fgets(in, INPUT_MAX_LEN, stdin);
                if (feof(stdin)) return 0;

                in_m = malloc(strlen(in) * sizeof(char));
                strcpy (in_m, in);

                eval (in_m);
                free (in_m);
        }
        return 0;
}
