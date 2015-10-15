#include <unistd.h>
#include <stdlib.h>

// self-include
#include "ptypes.h"

// Finds the job with the given pgid.
job *find_job (pid_t pgid)
{
    job *j;

    for (j = first_job; j; j = j->next) {
        if (j->pgid == pgid)
            return j;
    }

    return NULL;
}

// Returns true iff all processes in the job have stopped or completed.
int job_is_stopped (job *j)
{
    process *p;
    
    for (p = j->first; p; p = p->next) {
        if (!p->completed && !p->stopped)
            return 0;
    }

    return 1;
}

// Returns true iff all processes in the job have completed.
int job_is_completed (job *j)
{
    process *p;

    for (p = j->first; p; p = p->next) {
        if (!p->completed)
            return 0;
    }

    return 1;
}

void free_job (job *j)
{
    // TODO: free command
    // free (j->command);

    process *p;
    process *pnext;
    for (p = j->first; p; p = pnext) {
        pnext = p->next;
        // TODO: free argv
        free (p);
    }

    free (j);
}
