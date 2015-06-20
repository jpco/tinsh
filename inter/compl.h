#ifndef JPCO_COMPL_H
#define JPCO_COMPL_H

/**
 * Returns the passed line with the word demarcated by start
 * and end completed to the best of its ability.
 *
 *  - Will not complete if there are conflicting files/commands,
 *    but if there are SOME letters for certain, will complete
 *    those.
 *  - Completes files and commands. Support for variables will come
 *    later.
 *
 * Note: end is the last char in the word, NOT the first char
 * after the word!
 *
 * returned char* is newly allocated!
 */
char *l_compl (char *line, char *start, char *end);

/**
 * Completes a word. Behavior depends on if the word is the 'first'
 * in the line; if it is, makes attempts at elements of (PATH); if
 * not, does not.
 *
 * returned char* is newly allocated!
 */
char *w_compl (char *wd, int first);

#endif
