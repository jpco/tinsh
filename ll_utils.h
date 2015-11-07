#ifndef TINSH_LL_UTILS_H
#define TINSH_LL_UTILS_H

#include "types/linkedlist.h"

char *masked_strchr (char *str, char delim);

void word_split (char *in, linkedlist *ll);

char **cmd_ll_to_array (linkedlist *ll);

#endif
