#ifndef TINSH_POSIX_UTILS_H
#define TINSH_POSIX_UTILS_H

#include "ptypes.h"

// initialize the shell in a POSIX-pleasing way
void posix_init ();
void launch_process (process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground);
void launch_job (job *j, int foreground);
void put_job_in_foreground (job *j, int cont);
void put_job_in_background (job *j, int cont);

int mark_process_status (pid_t pid, int status);
void update_status ();
void wait_for_job (job *j);
void format_job_info (job *j, const char *status);
void do_job_notification ();

void mark_job_as_running (job *j);
void continue_job (job *j, int foreground);

#endif
