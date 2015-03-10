#include <stdlib.h>
#include <string.h>
#include "str.h"

int split_str(char *str, char **output, char delim)
{
        int strl = strlen(str);
        char *buf = str;
        char *del_loc;
        int ct;

        buf[strl - 1] = delim;

        while (*buf && (*buf == delim))
                buf++;

        ct = 0;
        while ((del_loc = strchr(buf, delim))) {
                output[ct++] = buf;
                *del_loc = '\0';
                buf = del_loc + 1;
                while (*buf && (*buf == delim)) {
                        buf++;
                }
        }
        output[ct] = NULL;

        return ct;
}
