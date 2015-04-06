#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// local includes
#include "defs.h"

// self include
#include "str.h"

/**
 * Returns 1 if passed strings are the same,
 * otherwise 0. Returns 0 as soon as it encounters a mismatched
 * char.
 */
int olstrcmp (const char *str1, const char *str2)
{
        int i;
        for (i = 0; str1[i] != '\0'; i++)
                if (str1[i] != str2[i]) return 0;
        return (str2[i] == '\0');
}

/**
 * NOTES:
 *  - WILL thrash arguments
 *
 * Removes the pointed-to character from the string to
 * which it belongs.
 */
void rm_char (char *line)
{
        int i;
        int len = strlen(line);
        for (i = 0; i < len; i++) {
                line[i] = line[i+1];
        }
}

/**
 * NOTES:
 *  - will not thrash arguments!
 *  - returned char** is newly allocated on the heap!
 *      - to free: free (retval[0]); free (retval);
 *      - sorry it's weird
 *
 * Returns a pointer to a NULL-terminated char* array,
 * containing all non-empty substrings of the argument
 * line which are separated by the passed delimiter
 * (not including the delimiter itself).
 *
 * Arguments:
 *  - line: the line to be split.
 *  - delim: the delimiter for splitting.
 */
char **split_str (char *line, char delim)
{
        char *nline = malloc ((1+strlen(line))*sizeof(char));
        strcpy (nline, line);
        line = nline;

        char **retval = calloc (MAX_SPLIT, sizeof(char *));

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

/**
 * NOTES:
 *  - will not thrash arguments!
 *  - returned char* is newly allocated on the heap!
 *
 * Returns a trimmed version of the passed string, with
 * starting whitespace (' ', '\t') and trailing whitespace
 * (' ', '\t', '\n') removed.
 *
 * Arguments:
 *  - line: the string to be trimmed
 * Returns:
 *  - a pointer to a new string, trimmed
 */
char *trim_str (char *line)
{
        char *oline = line;
        while (*line == ' ' || *line == '\t') rm_char (line);

        if (*line == '\0') return "";
        char *nline = malloc ((1 + strlen(line)) * sizeof(char));
        strcpy(nline, line);

        int i;
        for (i = strlen(line)-1; i >= 0 && (nline[i] == '\n' || 
                                 nline[i] == ' '); i--)
                nline[i] = '\0';

        return nline;
}

/**
 * NOTES:
 *  - Will not thrash arguments!
 *  - Returned char* is newly allocated on the heap!
 *
 * Combines an array of strings into one string, putting
 * the delim in between each string (or if delim=='\0',
 * putting no delim between each string).
 *
 * Arguments:
 *  - strs: the array of strings to combine.
 *  - len: the length of strs.
 *  - delim: the delimiter to insert between each string (or '\0'
 *  for no insertion)
 *
 * Returns: a pointer to the (heap-allocated) combined string.
 */
char *combine_str(char **strs, int arrlen, char delim)
{
        int len = 0;
        int i;
        for (i = 0; i < arrlen; i++) {
                len += strlen(strs[i]);
        }

        char *comb;
        if (delim == '\0') comb = calloc (len+1, sizeof(char));
        else comb = malloc (arrlen+len * sizeof(char));

        for (i = 0; i < arrlen; i++) {
                strcat (comb, strs[i]);
                if (delim != '\0')
                        strcat (comb, &delim);
        }
        return comb;
}

/*
 * NOTES:
 *  - will not thrash arguments!
 *  - returned char* is newly allocated on the heap!
 *
 * Variadic wrapper for the above function.
 * Arguments:
 *  - char delim: same as combine_str.
 *  - int ct: number of strings to be concatenated.
 *  - ...: strings to be concatenated.
 *
 * Returns:
 *  - same as combine_str.
 */
char *vcombine_str (char delim, int ct, ...)
{
        va_list argl;
        char *strarr[ct];

        va_start (argl, ct);
        int i;
        for (i = 0; i < ct; i++)
                strarr[i] = va_arg(argl, char *);

        return combine_str (strarr, ct, delim);
}
