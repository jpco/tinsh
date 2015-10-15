#ifndef TINSH_POSIX_TYPES_H
#define TINSH_POSIX_TYPES_H

#include <unistd.h>
#include <termios.h>

typedef struct process
{
    struct process *next;
    char **argv;
    pid_t pid;
    char completed;
    char stopped;
    int status;
} process;

typedef struct job
{
    struct job *next;
    char *command;
    process *first;
    pid_t pgid;
    char notified;
    struct termios tmodes;
    int stdin, stdout, stderr;
} job;

job *first_job; // = NULL;

// Finds a job given by the given pgid.
job *find_job (pid_t pgid);

// Returns true iff all processes in the given job have stopped.
int job_is_stopped (job *j);

// Returns true iff all processes in the given job have completed.
int job_is_completed (job *j);

// Frees a job and all processes inside of it.
void free_job (job *j);

#endif
