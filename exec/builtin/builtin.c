#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// local includes
#include "../../types/job.h"
#include "../../types/m_str.h"

#include "../../util/str.h"
#include "../../util/debug.h"

#include "../../inter/color.h"

#include "../env.h"
#include "../redirect.h"

#include "cd.h"

// self-include
#include "builtin.h"

int func_exit (job_j *job)
{
        exit (0);
        return 0xBEEFCAFE;
}

int func_pwd (job_j *job)
{
        printf ("%s\n", getenv ("PWD"));
        return 1;
}

int func_lsvars (job_j *job)
{
        ls_vars();
        return 1;
}

int func_lsalias (job_j *job)
{
        ls_alias();
        return 1;
}

int func_set (job_j *job)
{
        const char **argv = (const char **)job->argv;
        int argc = job->argc;

        if (argc == 1) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                char *keynval = combine_str(argv+1, argc-1, '\0');
                if (strchr(keynval, '=') == NULL) {
                        set_var (keynval, NULL);
                } else {
                        char **key_val = split_str (keynval, '=');

                        if (key_val[0] != NULL) {
                                set_var (key_val[0], key_val[1]);
                                free (key_val[0]);
                                free (key_val);
                        } else {
                                print_err ("Malformed 'set' syntax.");
                        }
                }
                free (keynval);
        }

        return 1;
}

int func_setenv (job_j *job)
{
        const char **argv = (const char **)job->argv;
        int argc = job->argc;

        if (argc == 1) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                char *keynval = combine_str(argv+1, argc-1, '\0');
                if (strchr(keynval, '=') == NULL) {
                        print_err ("Malformed \"setenv\".");
                } else {
                        char **key_val = split_str (keynval, '=');
                        free (keynval);

                        setenv (key_val[0], key_val[1], 1);
                }
        }

        return 1;
}

int func_unset (job_j *job)
{
        if (job->argc < 2) {
                print_err ("Too few args.\n");
        } else {
                int i;
                for (i = 1; i < job->argc; i++) {
                        unset_var (ms_strip(job->argv[i]));
                }
        }
        
        return 1;
}

int func_unenv (job_j *job)
{
        if (job->argc < 2) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                int i;
                for (i = 1; i < job->argc; i++) {
                        unsetenv (ms_strip(job->argv[i]));
                }
        }

        return 1;
}

int func_alias (job_j *job)
{
        if (job->argc < 3) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                set_msalias (ms_strip(job->argv[1]), job->argv[2]);
        }

        return 1;
}

int func_unalias (job_j *job)
{
        if (job->argc < 2) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                unset_alias (ms_strip(job->argv[1]));
        }

        return 1;
}

int func_color (job_j *job)
{
        if (job->argc < 2) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                color_line (job->argc-1, (const char **)job->argv+1);
        }

        return 1;
}


int builtin (job_j *job)
{
        char *f_arg = ms_strip (job->argv[0]);
        int (*cfunc)(job_j *) = NULL;

        if (strchr(f_arg, '/')) return 0;

        if (olstrcmp (f_arg, "exit")) {
                cfunc = func_exit;
        } else if (olstrcmp (f_arg, "cd")) {
                cfunc = func_cd;
        } else if (olstrcmp (f_arg, "pwd")) {
                cfunc = func_pwd;
        } else if (olstrcmp (f_arg, "lsvars")) {
                cfunc = func_lsvars;
        } else if (olstrcmp (f_arg, "lsalias")) {
                cfunc = func_lsalias;
        } else if (olstrcmp (f_arg, "set")) {
                cfunc = func_set;
        } else if (olstrcmp (f_arg, "setenv")) {
                cfunc = func_setenv;
        } else if (olstrcmp (f_arg, "unset")) {
                cfunc = func_unset;
        } else if (olstrcmp (f_arg, "unenv")) {
                cfunc = func_unenv;
        } else if (olstrcmp (f_arg, "alias")) {
                cfunc = func_alias;
        } else if (olstrcmp (f_arg, "unalias")) {
                cfunc = func_unalias;
        } else if (olstrcmp (f_arg, "color")) {
                cfunc = func_color;
        }

        if (cfunc != NULL) {
                setup_redirects (job);
                return cfunc (job);
        } else {
                return 0;
        }
}
