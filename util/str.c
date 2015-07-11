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

int startswith (const char *str1, const char *pre)
{
        int i;
        for (i = 0; pre[i] != '\0'; i++)
                if (str1[i] != pre[i]) return 0;
        return 1;
}

char rm_char (char *line)
{
        char tr = *line;
        int i;
        int len = strlen(line);
        for (i = 0; i < len; i++) {
                line[i] = line[i+1];
        }

        return tr;
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
        while (*line == ' ' || *line == '\t') line++;

        if (*line == '\0') {
                char *nline = calloc(1, sizeof(char));
                return nline;
        }
        char *nline = malloc ((1 + strlen(line)) * sizeof(char));
        strcpy(nline, line);

        int i;
        for (i = strlen(line)-1; i >= 0 && (nline[i] == '\n' || 
                                 nline[i] == ' '); i--) {
                if (i >= 1 && nline[i-1] == '\\') break;
                nline[i] = '\0';
        }

        return nline;
}

char *combine_str(const char **strs, int arrlen, char delim)
{
        int len = 0;
        int i;
        for (i = 0; i < arrlen; i++) {
                if (strs[i] == NULL || *strs[i] == '\0') continue;
                len += strlen(strs[i]);
        }

        char *comb = calloc (len+1, sizeof(char));
        if (delim != '\0') {
                free (comb);
                comb = calloc ((arrlen+len+1), sizeof(char));
        }
        if (errno == ENOMEM) {
                print_err ("Could not malloc to combine str");
                return NULL;
        }

        int ctotal = 0;
        for (i = 0; i < arrlen; i++) {
                if (strs[i] == NULL) continue;
                int j;
                for (j = 0; strs[i][j] != '\0'; j++) {
                        comb[ctotal+j] = strs[i][j];
                }
                ctotal += j;
                if (delim != '\0' && i < arrlen-1) {
                        comb[ctotal++] = delim;
                }
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
