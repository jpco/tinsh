#ifndef JPSH_DEBUG_H
#define JPSH_DEBUG_H

// Defined by impsh.c, used by *print_err* to report
// line numbers on scripts.
extern int debug_line_no;

/**
 * Prints a message to stderr. The latter method adds
 * the errno, as well as the output of strerror(errno).
 */
void print_err (const char *errmsg);
void print_err_wno (const char *errmsg, int err);

/**
 * Identical to the prior two functions, except these
 * do not print unless __imp_debug is set.
 */
void dbg_print_err (const char *errmsg);
void dbg_print_err_wno (const char *errmsg, int err);

#endif
