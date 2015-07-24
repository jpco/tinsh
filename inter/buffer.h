#ifndef JPSH_LINEBUFFER_H
#define JPSH_LINEBUFFER_H

/**
 * NOTE:
 *  - returned char* is allocated in the heap!
 *
 * The main line-buffer loop. This is what is in charge in between each
 * line eval.
 */
char *line_loop();

#endif
