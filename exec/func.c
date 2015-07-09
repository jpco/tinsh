#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// local includes
#include "cd.h"

#include "../inter/color.h"
#include "../eval/eval_utils.h"
#include "../exec/exec_utils.h"

#include "../eval.h"
#include "../defs.h"
#include "../str.h"
#include "../var.h"
#include "../debug.h"

// self-include
#include "func.h"

int func_exit (job_t *job)
{
        exit (0);
        return 0xBEEFCAFE;
}

int func_pwd (job_t *job)
{
        printf("%s\n", getenv ("PWD"));
        return 1;
}

int func_lsvars (job_t *job)
{
        ls_vars();
        return 1;
}

int func_lsalias (job_t *job)
{
        ls_alias();
        return 1;
}

int func_set (job_t *job)
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

int func_setenv (job_t *job)
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

int func_unset (job_t *job)
{
        if (job->argc < 2) {
                print_err ("Too few args.\n");
        } else {
                int i;
                for (i = 1; i < job->argc; i++) {
                        unset_var (job->argv[i]);
                }
        }
        
        return 1;
}

int func_unenv (job_t *job)
{
        if (job->argc < 2) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                int i;
                for (i = 1; i < job->argc; i++) {
                        unsetenv (job->argv[i]);
                }
        }

        return 1;
}

int func_alias (job_t *job)
{
        if (job->argc < 3) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                set_alias (job->argv[1], job->argv[2]);
        }

        return 1;
}

int func_unalias (job_t *job)
{
        if (job->argc < 2) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                unset_alias (job->argv[1]);
        }

        return 1;
}

int func_color (job_t *job)
{
        if (job->argc < 2) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                color_line (job->argc-1, (const char **)job->argv+1);
        }

        return 1;
}


int func (job_t *job)
{
        char **argv = job->argv;
        int (*cfunc)(job_t *) = NULL;

        if (strchr(argv[0], '/')) return 0;

        if (olstrcmp (argv[0], "exit")) {
                cfunc = func_exit;
        } else if (olstrcmp (argv[0], "cd")) {
                cfunc = func_cd;
        } else if (olstrcmp (argv[0], "pwd")) {
                cfunc = func_pwd;
        } else if (olstrcmp (argv[0], "lsvars")) {
                cfunc = func_lsvars;
        } else if (olstrcmp (argv[0], "lsalias")) {
                cfunc = func_lsalias;
        } else if (olstrcmp (argv[0], "set")) {
                cfunc = func_set;
        } else if (olstrcmp (argv[0], "setenv")) {
                cfunc = func_setenv;
        } else if (olstrcmp (argv[0], "unset")) {
                cfunc = func_unset;
        } else if (olstrcmp (argv[0], "unenv")) {
                cfunc = func_unenv;
        } else if (olstrcmp (argv[0], "alias")) {
                cfunc = func_alias;
        } else if (olstrcmp (argv[0], "unalias")) {
                cfunc = func_unalias;
        } else if (olstrcmp (argv[0], "color")) {
                cfunc = func_color;
        }

        if (cfunc != NULL) {
                setup_redirects (job);
                return cfunc (job);
        } else {
                return 0;
        }
}
