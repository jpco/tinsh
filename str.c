#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

// local includes
#include "defs.h"
#include "debug.h"

// self include
#include "str.h"

int olstrcmp (const char *str1, const char *str2)
{
        int i;
        for (i = 0; str1[i] != '\0'; i++)
                if (str1[i] != str2[i]) return 0;
        return (str2[i] == '\0');
}

void rm_char (char *line)
{
        int i;
        int len = strlen(line);
        for (i = 0; i < len; i++) {
                line[i] = line[i+1];
        }
}

char **split_str (char *line, char delim)
{
        char *nline = malloc ((1+strlen(line))*sizeof(char));
        if (errno == ENOMEM) {
                print_err ("Could not malloc while splitting string.");
                return NULL;
        }
        strcpy (nline, line);
        line = nline;

        errno = 0;
        char **retval = calloc (MAX_SPLIT, sizeof(char *));
        if (errno == ENOMEM) {
                print_err ("Could not calloc while splitting string.");
                return NULL;
        }

        char *buf = line;
        char *sbuf = buf;
        char *nbuf = buf;
        int i;
        for (i=0; (nbuf = strchr(buf, delim)) != NULL; i++) {
                if (nbuf != line && *(nbuf-1) == '\\') {
                        rm_char (nbuf-1);
                        buf = nbuf;
                        i--;
                        continue;
                }
                // buf to nbuf is range
                if (nbuf != sbuf) {
                        retval[i] = sbuf;
                        *nbuf = '\0';
                }
                buf = nbuf+1;
                sbuf = buf;
        }
        // last segment
        if (sbuf != '\0')
                retval[i] = sbuf;

        return retval;
}

char *trim_str (const char *line)
{
        const char *oline = line;
        while (*line == ' ' || *line == '\t') line++;

        if (*line == '\0') return "";
        char *nline = malloc ((1 + strlen(line)) * sizeof(char));
        strcpy(nline, line);

        int i;
        for (i = strlen(line)-1; i >= 0 && (nline[i] == '\n' || 
                                 nline[i] == ' '); i--)
                nline[i] = '\0';

        return nline;
}

char *combine_str(const char **strs, int arrlen, char delim)
{
        int len = 0;
        int i;
        for (i = 0; i < arrlen; i++) {
                len += strlen(strs[i]);
        }

        char *comb;
        if (delim == '\0') comb = calloc (len+1, sizeof(char));
        else comb = malloc (arrlen+len * sizeof(char));
        if (errno == ENOMEM) {
                print_err ("Could not malloc to combine str");
                return NULL;
        }

        for (i = 0; i < arrlen; i++) {
                strcat (comb, strs[i]);
                if (delim != '\0')
                        strcat (comb, &delim);
        }
        return comb;
}

char *vcombine_str (char delim, int ct, ...)
{
        va_list argl;
        const char *strarr[ct];

        va_start (argl, ct);
        int i;
        for (i = 0; i < ct; i++)
                strarr[i] = va_arg(argl, const char *);

        return combine_str (strarr, ct, delim);
}
