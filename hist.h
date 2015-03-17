#ifndef JPSH_HIST_H
#define JPSH_HIST_H

#define HIST_SIZE 3000

void hist_add(char *in);
char *hist_mv(char in);
void hist_free();

#endif
