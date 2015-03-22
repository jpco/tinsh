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

        *buf = calloc(idx1+inslen+len-idx2, sizeof(char));
       
        int i;
        for (i = 0; i < idx1; i++) {
                (*buf)[i] = str[i];
        }
        for (i = idx1; i < idx1+inslen; i++) {
                (*buf)[i] = ins[i-idx1];
        }
        for (i = 0; i < len-idx2; i++) {
                (*buf)[i+idx1+inslen] = str[i+idx2];
        }

        return idx1+inslen;
}

void rm_first_char(char *str)
{
        int i;
        for (i = 0; i < strlen(str); i++) {
                str[i] = str[i+1];
        }
}

int split_str(char *str, char **output, char delim, int rm_mul)
{
        // buf keeps track of start of current word
        // tm_buf keeps track of current word after escaped spaces
        int strl = strlen(str);
        char *buf = str;
        char *tm_buf = buf;
        char *del_loc;
        int ct;

        str[strlen(str) - 1] = delim;

        if (rm_mul)
                while (*buf && (*buf == delim))
                        buf++;

        ct = 0;
        tm_buf = buf;
        while (*buf != '\0' && (del_loc = strchr(tm_buf, delim))) {
                if (*(del_loc - 1) == '\\') {
                        rm_first_char(del_loc-1);
                        tm_buf = del_loc;
                        continue;
                }
                output[ct++] = buf;
                *del_loc = '\0';
                buf = del_loc + 1;
                if (rm_mul) {
                        while (*buf && (*buf == delim)) {
                                buf++;
                        }
                }
                tm_buf = buf;
        }
        output[ct] = NULL;

        return ct;
}
