#ifndef JPSH_STR_H
#define JPSH_STR_H

int starts_with(const char *str, const char *pre);
int insert_str(char *str, char **buf, char *ins, int idx1, int idx2);
int split_str(char *str, char **output, char delim, int rm_mul);
int split_strn(char *str, char **output, char delim, int rm_mul);

#endif
