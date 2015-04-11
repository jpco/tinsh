#ifndef JPSH_EVAL_H
#define JPSH_EVAL_H

/**
 * Evaluates the command line and executes it as necessary.
 * NOTE: frees cmdline!!!
 */
void eval (char *cmdline);

// Frees the current eval variables
// NOTE: only if the program is to exit during execution!
void free_ceval (void);

#endif
