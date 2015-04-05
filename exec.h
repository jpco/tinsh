#ifndef JPSH_EXEC_H
#define JPSH_EXEC_H

int sigchild (int signo);
void try_exec (int argc, char **argv, int bg);

#endif
