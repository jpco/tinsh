#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hist.h"

static char *hist_ar[HIST_SIZE];
static int hist_len;
static int loc;

void hist_add(char *in)
{
        if (*in == '\0') return;
        int inlen = strlen(in);
        char *histcpy = malloc((1+inlen) * sizeof(char));
        strcpy(histcpy, in);

        if (hist_len < HIST_SIZE) {
                hist_ar[hist_len++] = histcpy;
        } else {
                free (hist_ar[0]);
                int i;
                for (i = 0; i < HIST_SIZE-1; i++) {
                        hist_ar[i] = hist_ar[i+1];
                }
                hist_ar[HIST_SIZE] = histcpy;
        }
        loc = hist_len;
}

char *hist_mv(char in)
{
//        return "cats";
        if (in == 'A') { // up
                if (loc > 0) loc--;
                return hist_ar[loc];
        } else if (in == 'B') { // down
                if (loc < hist_len) loc++;
                if (loc == hist_len) return "";
                else return hist_ar[loc];
        }
        return "";
}

void hist_free()
{
        int i;
        for (i = 0; i < hist_len; i++) {
                free (hist_ar[i]);
        }
}
