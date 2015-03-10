#include<stdio.h>
#include "debug.h"
#include "def.h"

void printjob (int bg, char **argv)
{
        int i;

        printf("\e[0;35m");
        if(bg) printf("(background) ");
        printf("[%s] ", argv[0]);
        for (i = 1; argv[i] != NULL; i++) {
                printf("%s ", argv[i]);
        }
        printf("\e[0m\n");
}

void printpath (char *path)
{
        printf("\e[0;32m");
        printf("<%s>", path);
        printf("\e[0m\n");
}
