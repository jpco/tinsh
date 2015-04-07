#ifndef JPSH_HIST_H
#define JPSH_HIST_H

void hist_add (char *line);
char *hist_up(void);
char *hist_down(void);
void free_hist(void);

#endif
