#include<stdlib.h>
#include<stdio.h>

#include "builtin.h"
#include "def.h"
#include "cd.h"

int builtin(char **argv)
{
        if (strcmp(argv[0], "exit") == 0) {
                atexit(free_cmd);
                exit(0);
        } else if (strcmp(argv[0], "cd") == 0) {
                if (cd(argv[1]) > 0) {
                        return 2;
                } else {
                        return 1;
                }
        } else if(strcmp(argv[0], "environ") == 0) {
                int i;
                for (i = 0; environ[i] != NULL; i++)
                        printf("%s\n", environ[i]);
                return 1;
        } else if(strcmp(argv[0], "pwd") == 0) {
                printf("%s\n", getenv("PWD"));
                return 1;
        } else if(strcmp(argv[0], "setenv") == 0) {
                if(argv[1] == NULL || argv[2] == NULL) return 1;
                setenv(argv[1], argv[2], 1);
                return 1;
        }
        return 0;
}
