#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// local includes #include "defs.h" #include "str.h"
#include "env.h"
#include "str.h"
#include "defs.h"
#include "debug.h"

// self-include
#include "eval.h"

static char **jobs;
static char *job;
static char **argv;

void eval (char *cmdline)
{
/*
 * Parsing:
 * 0) [execute subjobs]
 * 1) split jobs
 *    "cat (mydir)/foo | ll" => "cat (mydir)/foo" | "ll"
 * 2) expand aliases
 *    "cat (mydir)/foo" | "ll" => "cat (mydir)/foo" | "ls -l"
 * 3) evaluate vars
 *    "cat (mydir)/foo" | "ls -l" => "cat /bin/foo" | "ls -l"
 * 4) pull apart args
 *    "cat /bin/foo" | "ls -l" => "cat" "/bin/foo" | "ls" "-l"
 * 5) execute (w/ piping/redirection)
 *
 * TODO: ADD SUBJOB SUPPORT
 */

        if (*cmdline == '\0') return;

        // STEP 1: Split jobs
        jobs = split_str (cmdline, '|');
        free (cmdline);
        if (jobs == NULL) return;
        int big_i;
        for (big_i=0; jobs[big_i] != NULL; big_i++) {
                job = trim_str (jobs[big_i]);
                if (job == NULL) return;
                if (*job == '\0') {
                        continue;
                }

                // STEP 2: expand aliases
                char *jbuf;
                if ((jbuf = strchr (job, ' ')) != NULL)
                        *jbuf = '\0';

                if (has_alias (job)) {
                        char *ncmd = get_alias (job);
                        if (ncmd == NULL && errno == ENOMEM) {
                                print_err ("malloc failure");
                                return;
                        }
                        char *njob = "";
                        if (jbuf != NULL) {
                                *jbuf = ' ';
                                njob = vcombine_str ('\0', 2, ncmd, jbuf);
                                if (njob == NULL && errno == ENOMEM) {
                                        print_err ("malloc failure");
                                        return;
                                }
                        } else {
                                njob = ncmd;
                        }
                        free (job);
                        job = njob;
                } else {
                        if (jbuf != NULL)
                                *jbuf = ' ';
                }

                // STEP 3: expand vars
                // prologue: ~
                jbuf = job;
                char *cbuf = jbuf;
                char *tilde = get_var ("__jpsh_~home");
                if (tilde != NULL) {
                        while ((jbuf = strchr (cbuf, '~')) != NULL) {
                                if (jbuf > job && *(jbuf-1) == '\\') {
                                        rm_char (jbuf-1);
                                        cbuf = jbuf;
                                        continue;
                                }
                                cbuf = jbuf+1;
                                *jbuf = '\0';
                                char *njob = vcombine_str (0, 3, job,
                                                tilde, cbuf+1);
                                if (njob == NULL && errno == ENOMEM) {
                                        print_err ("malloc failure");
                                        return;
                                }
                                jbuf = njob + (jbuf - job);
                                free (job);
                                job = njob;
                                cbuf = jbuf;
                        }
                }

                // main part.
                jbuf = job;
                cbuf = jbuf;
                while ((jbuf = strchr (cbuf, '(')) != NULL) {
                        if ((cbuf = strchr (jbuf, ')')) == NULL)
                                break;
                        if (jbuf > job && *(jbuf-1) == '\\') {
                                rm_char (jbuf-1);
                                cbuf = jbuf;
                                continue;
                        }
                        *cbuf = '\0';
                        char *val;
                        if ((val = get_var (jbuf+1)) == NULL)
                                val = getenv (jbuf+1);
                        if (val == NULL) val = "";

                        *jbuf = '\0';
                        char *njob = vcombine_str ('\0', 3,
                                        job, val, cbuf+1);
                        if (njob == NULL && errno == ENOMEM) {
                                print_err ("malloc failure");
                                return;
                        }
                        int jbdiff = jbuf - job;
                        jbuf = njob + jbdiff;
                        free (job);
                        job = njob;
                        cbuf = jbuf;
                }

                // STEP 4: separate args
                // TODO: backslash-escaping works but "" doesn't
                argv = split_str (job, ' ');
                if (argv == NULL && errno == ENOMEM) {
                        print_err ("malloc failure");
                        return;
                }

                // STEP 5: execute (w/ piping/redirection)
                int argl;
                for (argl = 0; argv[argl] != NULL; argl++);
                if (olstrcmp(argv[argl-1], "&")) {
                        free (argv[argl-1]);
                        argv[--argl] = NULL;
                        try_exec (argl, argv, 1);
                } else  try_exec (argl, argv, 0);
                free (argv[0]);
                free (argv);
                free (job);
        }
        free (jobs[0]);
        free (jobs);
}

void free_ceval ()
{
        free (jobs[0]);
        free (jobs);
        free (job);
        free (argv[0]);
        free (argv);
}
