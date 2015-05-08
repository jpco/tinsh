#ifndef JPCO_COMPL_H
#define JPCO_COMPL_H

/**
 * Note: end is the last char in the word, NOT the first char
 * after the word!
 *
 * returned char* is newly allocated!
 */
char *l_compl (char *line, char *start, char *end);

/**
 * returned char* is newly allocated!
 */
char *w_compl (char *wd, int first);

#endif
