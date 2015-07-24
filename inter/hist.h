#ifndef JPSH_HIST_H
#define JPSH_HIST_H

/**
 * Adds an entry to the history. If the current bottom
 * element is the same as the passed line, does not
 * add the entry. Also moves the index to the bottom.
 */
void hist_add (const char *line);

/**
 * NOTE:
 *  - returned char* is newly allocated on the heap!
 *
 * Moves the current history index up, and returns the
 * current index. If at the top of the history, stays
 * at the same index.
 *
 * Cannot hurt hist (I think)...
 */
char *hist_up(void);

/**
 * NOTE:
 *  - returned char* is newly allocated on the heap!
 *
 * Moves the current history index down, and returns
 * the current index. If we are at the bottom of the
 * history, returns "".
 *
 * Cannot hurt hist (I think)...
 */
char *hist_down(void);

/**
 * Frees the history from memory.
 */
void free_hist(void);

#endif
