#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "ptypes.h"
#include "putils.h"
#include "../builtins/builtins.h"

extern char **environ;

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

// Initialize the shell in a POSIX-pleasing way
void posix_init (void)
{
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty (shell_terminal);

    if (shell_is_interactive) {
        // wait until we are fg
        while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ()))
            kill (- shell_pgid, SIGTTIN);

        // ignore interactive & job-control signals.
//        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);

        // this should not be ignored; we need to get this signal from children
//        signal (SIGCHLD, SIG_IGN);

        // put ourselves in in a new process group.
        shell_pgid = getpid ();
        if (setpgid (shell_pgid, shell_pgid) < 0) {
            perror ("Couldn't put shell in its own process group");
            exit (1);
        }

        // take the terminal for our own
        tcsetpgrp (shell_terminal, shell_pgid);

        // save default terminal attributes
        tcgetattr (shell_terminal, &shell_tmodes);
    }
}

// Launch a new process (called right after forking; never returns)
void launch_process (process *p, pid_t pgid,
                     int infile, int outfile, int errfile,
                     int foreground)
{
    pid_t pid;

    if (shell_is_interactive) {
        // put the process in the process group and give the new process group the terminal (if applicable)
        // must be done by both shell and subprocesses for race condition reasons (?)
        pid = getpid ();
        if (pgid == 0) pgid = pid;
        setpgid (pid, pgid);
        if (foreground)
            tcsetpgrp (shell_terminal, pgid);

        // reset job control signal handling
        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);

        // Set up redirects
        if (infile != STDIN_FILENO) {
            dup2 (infile, STDIN_FILENO);
            close (infile);
        }
        if (outfile != STDOUT_FILENO) {
            dup2 (outfile, STDOUT_FILENO);
            close (outfile);
        }
        if (errfile != STDERR_FILENO) {
            dup2 (errfile, STDERR_FILENO);
            close (errfile);
        }

        // Execute the new process; make sure we exit.
        execve (p->argv[0], p->argv, environ);
        perror ("execve");

        exit (1);
    }
}

void launch_job (job *j, int foreground)
{
    process *p;
    pid_t pid;
    int procpipe[2], infile, outfile;

    infile = j->stdin;
    for (p = j->first; p; p = p->next) {
        // set up pipes
        if (p->next) {
            if (pipe (procpipe) < 0) {
                perror ("pipe");
                exit (1);
            }
            outfile = procpipe[1];
        } else {
            outfile = j->stdout;
        }

        if (p->wh_exec->type == SYM_BINARY) {
            // fork the child processes
            pid = fork ();
            if (pid == 0) {
                // this is the child proc
                launch_process (p, j->pgid, infile, outfile, j->stderr, foreground);
            } else if (pid < 0) {
                // something broke
                perror ("fork");
                exit (1);
            } else {
                // this is the parent proc
                p->pid = pid;
                if (shell_is_interactive) {
                    if (!j->pgid)
                        j->pgid = pid;
                    setpgid (pid, j->pgid);
                }
            }

            if (infile != j->stdin)
                close (infile);
            if (outfile != j->stdout)
                close (outfile);
            infile = procpipe[0];
        } else if (p->wh_exec->type == SYM_BUILTIN) {
            // launch builtin in a new thread, don't fork
            launch_builtin (p, infile, outfile, j->stderr, foreground);
        }
    }

    // format_job_info (j, "launched");

    if (!shell_is_interactive) {
        wait_for_job (j);
    } else if (foreground) {
        put_job_in_foreground (j, 0);
    } else {
        put_job_in_background (j, 0);
    }
}

void put_job_in_foreground (job *j, int cont)
{
    // actually put the job in the foreground.
    tcsetpgrp (shell_terminal, j->pgid);

    // give the job a continue signal, if necessary.
    if (cont) {
        tcsetattr (shell_terminal, TCSADRAIN, &j->tmodes);
        if (kill (- j->pgid, SIGCONT) < 0)
            perror ("kill (SIGCONT)");
    }

    // wait for it to report
    wait_for_job (j);

    // put the shell back in the foreground
    tcsetpgrp (shell_terminal, shell_pgid);

    // restore the shell's terminal modes
    tcgetattr (shell_terminal, &j->tmodes);
    tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes);
}

void put_job_in_background (job *j, int cont)
{
    if (cont)
        if (kill (- j->pgid, SIGCONT) < 0)
            perror ("kill (SIGCONT)");
}

int mark_process_status (pid_t pid, int status)
{
    job *j;
    process *p;

    if (pid > 0) {
        // update the record for the next process
        for (j = first_job; j; j = j->next) {
            for (p = j->first; p; p = p->next) {
                if (p->pid == pid) {
                    p->status = status;
                    if (WIFSTOPPED (status)) {
                        p->stopped = 1;
                    } else {
                        p->completed = 1;
                        if (WIFSIGNALED (status)) {
                            fprintf (stderr, "%d: Terminated by signal %d.\n",
                                    (int) pid, WTERMSIG (p->status));
                        }
                    }
                    return 0;
                }
            }
        }
        fprintf (stderr, "No child process %d.\n", pid);
        return -1;
    } else if (pid == 0 || errno == ECHILD) {
        // no processes ready to report
        return -1;
    } else {
        perror ("waitpid");
        return -1;
    }
}

void update_status (void)
{
    int status = 0;
    pid_t pid;
    int mps_ret;

    do {
        pid = waitpid (-1, &status, WUNTRACED|WNOHANG);
    } while (!(mps_ret = mark_process_status (pid, status)));
}

void wait_for_job (job *j)
{
    int status;
    pid_t pid;

    do {
        pid = waitpid (WAIT_ANY, &status, WUNTRACED);
    } while (!mark_process_status (pid, status)
            && !job_is_stopped (j)
            && !job_is_completed (j));

    job *jcurr = NULL, *jlast = NULL, *jnext = NULL;
    for (jcurr = first_job; jcurr; jcurr = jnext) {
        jnext = jcurr->next;
        if (jcurr != j) {
            jlast = jcurr;
            continue;
        }

        if (jlast) {
            jlast->next = jnext;
        } else {
            first_job = jnext;
        }

        // TODO: free this job at the appropriate time
        // free_job (j);
        break;
    }
}

void format_job_info (job *j, const char *status)
{
    fprintf (stderr, "%ld (%s): %s\n", (long)j->pgid, status, j->command);
}

void do_job_notification (void)
{
    job *j, *jlast, *jnext;

    // update status information for child processes.
    update_status ();

    jlast = NULL;
    jnext = NULL;
    for (j = first_job; j; j = jnext) {
        jnext = j->next;

        if (job_is_completed (j)) {
            // All processes completed, so notify user && remove job
            format_job_info (j, "completed");
            if (jlast) {
                jlast->next = jnext;
            } else {
                first_job = jnext;
            }
            free_job (j);
        } else if (job_is_stopped (j) && !j->notified) {
            // Job is stopped, so tell user if that hasn't been done already
            format_job_info (j, "stopped");
            j->notified = 3;
            jlast = j;
        } else {
            // Do nothing
            // format_job_info (j, "running");
            jlast = j;
        }
    }
}

void mark_job_as_running (job *j)
{
    process *p;

    for (p = j->first; p; p = p->next) {
        p->stopped = 0;
    }
    j->notified = 0;
}

void continue_job (job *j, int foreground)
{
    mark_job_as_running (j);

    if (foreground) put_job_in_foreground (j, 1);
    else put_job_in_background (j, 1);
}
