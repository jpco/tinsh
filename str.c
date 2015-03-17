#include <stdlib.h>
#include <string.h>
#include "str.h"

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
