#ifndef JPSH_STR_H
#define JPSH_STR_H

#include <unistd.h>

/**
 * Returns 1 if passed strings are the same,
 * otherwise 0. Returns 0 as soon as it encounters a mismatched
 * char.
 */
int olstrcmp (const char *str1, const char *str2);

/**
 * Returns 1 if the first passed string starts with
 * the second passed string (not including the '\0',
 * of course.) Returns 0 as soon as it encounters a
 * mismatched char.
 */
int startswith (const char *str1, const char *pre);

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
char **split_str (char *line, char delim);

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
char *trim_str (const char *line);

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
char *combine_str(const char **strs, int len, char delim);

/*
 * NOTES:
 *  - will not thrash arguments!
 *  - returned char* is newly allocated on the heap!
 *
 * Variadic wrapper for the above function.
 * Arguments:
 *  - char delim: same as combine_str.
 *  - int ct: number of strings to be concatenated.
 *  - ...: char*s to be concatenated.
 *
 * Returns:
 *  - same as combine_str.
 */
char *vcombine_str(char delim, int ct, ...);

/**
 * NOTE:
 *  - WILL thrash arguments
 *
 * Removes the pointed-to character from the string to
 * which it belongs.
 */
void rm_char (char *line);

#endif
