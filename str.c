#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "str.h"

int starts_with(const char *str, const char *pre)
{
        size_t lenpre = strlen(pre), lenstr = strlen(str);
        return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

int insert_str(char *str, char **buf, char *ins, int idx1, int idx2)
{
        int len = strlen(str);
        int inslen = strlen(ins);
        if (idx1 > len || idx2 > len) return -1;
        if (idx1 > idx2) return -1;

        *buf = calloc(idx1+inslen+len-idx2+1, sizeof(char));
       
//        printf("%d, %d, %d, %d\n", len, inslen, idx1, idx2);
        int i;
        for (i = 0; i < idx1; i++) {
                (*buf)[i] = str[i];
//                printf(".%c", nstr[i]);
        }
        for (i = idx1; i < idx1+inslen; i++) {
                (*buf)[i] = ins[i-idx1];
//                printf(",%c", nstr[i]);
        }
        for (i = idx1+inslen; i < 1+idx1+inslen+len-idx2; i++) {
                (*buf)[i] = str[idx2+i-inslen];
//                printf("/%c", nstr[i+idx1+inslen]);
        }

//        printf("'%s', '%s', '%s'\n", str, ins, *buf);
        return idx1+inslen;
}

int split_str(char *str, char **output, char delim, int rm_mul)
{
        int strl = strlen(str);
        char *buf = str;
        char *del_loc;
        int ct;

        str[strlen(str) - 1] = delim;

        if (rm_mul)
                while (*buf && (*buf == delim))
                        buf++;

        ct = 0;
        while (*buf != '\0' && (del_loc = strchr(buf, delim))) {
                output[ct++] = buf;
                *del_loc = '\0';
                buf = del_loc + 1;
                if (rm_mul) {
                        while (*buf && (*buf == delim)) {
                                buf++;
                        }
                }
        }
        output[ct] = NULL;

        return ct;
}
