#include <string.h>
#include <stdio.h>

#include "posix/ptypes.h"
#include "posix/putils.h"

int exec (job *cjob, int fg)
{
    process *cproc;
    for (cproc = cjob->first; cproc; cproc = cproc->next) {
        // if no '/' in bin, translate cmd_ll to bin path
        char retried = 0;
        if (!strchr (cproc->argv[0], '/')) {
            sym_t *execu;
    resolve:
            execu = sym_resolve (cproc->argv[0], SYM_BINARY | SYM_BUILTIN);
            if (!execu) {
                if (!retried) {
                    rehash_bins();
                    retried = 1;
                    goto resolve;
                }
                fprintf (stderr, "Command '%s' not found.\n", cproc->argv[0]);
                return -1;
            }
            cproc->wh_exec = execu;
        }
    }

    // add job to job list
    if (!first_job) {
        first_job = cjob;
    } else {
        job *lj, *cj;
        for (cj = first_job; cj; cj = cj->next) lj = cj;
        lj->next = cjob;
    }

    // launch job
    launch_job (cjob, fg);
    // refresh jobs
    do_job_notification ();

    return 0;
}
