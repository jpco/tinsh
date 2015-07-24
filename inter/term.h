#ifndef JPSH_TERM_H
#define JPSH_TERM_H

/**
 * Function to get the current terminal width.
 * 
 * Returns -1 on error, the current width on success.
 */
int term_width();

/**
 * Function to get the current cursor position.
 * Assumes we are currently during the linebuffer
 * period, not during evaluation/execution. (i.e.,
 * assumes ICANON, IECHO, etc. are OFF)
 *
 * Returns 0 on success, nonzero otherwise. On error,
 * should not affect the value at *row or *col.
 */
int cursor_pos (int *row, int *col);

/**
 * Initializes the terminal.
 */
void term_init();

/**
 * Exits the terminal.
 */
void term_exit();

/*
 * Pre-evaluation function for setting up environment before
 * parsing & executing a command. (Generally, for un-fscking
 * up things important for the shell which mess with normal
 * execution)
 * Returns:
 *  - 0 on success, ready to evaluate & execute
 *  - non-zero otherwise.
 */
int term_prep();

/**
 * Re-establishes the shell environment for the linebuffer.
 */
void term_unprep();

#endif
