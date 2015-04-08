#ifndef JPSH_TERM_H
#define JPSH_TERM_H

int cursor_pos (int *row, int *col);

void term_init();
void term_exit();

int term_prep();
void term_unprep();

#endif
