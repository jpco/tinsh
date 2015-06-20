#ifndef _JPSH_EVAL_UTILS_H_
#define _JPSH_EVAL_UTILS_H_

void rm_element (char **argv, char **argm, int idx, int *argc);
void add_element (char **argv, char **argm, char *na, char *nm, int idx,
                int *argc);
char *masked_strchr(const char *s, const char *m, char c);
void masked_trim_str(const char *s, const char *m, char **ns, char **nm);
void spl_cmd (const char *s, const char *m, char ***argv, char ***argm,
                int *argc);
/**
 * NOTE:
 *  - WILL thrash arguments
 *
 * Removes the pointed-to character from the string to
 * which it belongs.
 */
void arm_char (char *line, size_t len);

char *mask_str (char *cmdline);

#endif // _JPSH_EVAL_UTILS_H_
