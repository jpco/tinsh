#ifndef JPCO_COMPL_H
#define JPCO_COMPL_H

#include "../util/defs.h"

extern char cpl_line[];
extern size_t cpl_idx;

/**
 * Fills cpl_line with the passed line completed to the best of its ability.
 *
 *  - Will not complete if there are conflicting files/commands,
 *    but if there are SOME letters for certain, will complete
 *    those.
 *  - Completes files and commands. Support for variables will come
 *    later.
 *
 * On success, returns 0. On failure, returns 1.  If 1 is returned, the
 * contents of cpl_line are undefined.
 *
 * Note: end is the last char in the word, NOT the first char
 * after the word!
 * 
 */
int l_compl (char *line, char *start, char *end);

/**
 * Completes a word. Behavior depends on if the word is the 'first'
 * in the line; if it is, makes attempts at elements of (PATH); if
 * not, does not.
 */
int w_compl (char *wd, int first);

#endif
