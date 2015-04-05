#ifndef JPSH_STR_H
#define JPSH_STR_H

char **split_str (char *line, char delim);
char *trim_str (char *line);
char *combine_str(char **strs, int len, char delim);
char *vcombine_str(char delim, int ct, ...);
void rm_char (char *line);

#endif
