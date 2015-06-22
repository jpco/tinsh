#ifndef JPSH_EVAL_MASK_H
#define JPSH_EVAL_MASK_H

// The mask_eval function used in the evaluation queue.
void mask_eval();

// Generates a mask for a string, and removes the masking characters in
// the string. Thrashes cmdline.
char *mask_str (char *cmdline);

// A quick way to encode mask data into a string. Does not thrash args.
char *unmask_str (char *str, char *mask);


#endif
