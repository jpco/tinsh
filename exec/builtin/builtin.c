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
#include "flow.h"

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

// TODO: move these two around
int func_set (job_j *job)
{
        return func_settype (job, DEF);
}

int func_settype (job_j *job, vartype deftype)
{
        const m_str **argv = (const m_str **)(job->argv + 1);
        size_t argc = job->argc - 1;

        if (argc == 0) {
                print_err ("Too few args.\n");
                return 2;
        } else {
                vartype v_type = deftype;
                char adv = 0;
                if (olstrcmp(ms_strip((m_str *)argv[0]), "env")) {
                        v_type = ENV;
                        adv = 1;
                } else if (olstrcmp(ms_strip((m_str *)argv[0]), "global")) {
                        v_type = GLOBAL;
                        adv = 1;
                } else if (olstrcmp(ms_strip((m_str *)argv[0]), "local")) {
                        v_type = LOCAL;
                        adv = 1;
                }
                if (adv) {
                        argv++;
                        argc--;
                }

                m_str *keynval = ms_combine(argv, argc, ' ');

                if (ms_strchr(keynval, '=') != NULL) {
                        m_str **key_val = ms_split (keynval, '=');

                        ms_trim (&key_val[0]);
                        ms_trim (&key_val[1]);
                        if (key_val[0] != NULL) {
                                set_type_var (ms_strip(key_val[0]),
                                        ms_strip(key_val[1]), v_type);
                                ms_free (key_val[0]);
                                ms_free (key_val[1]);
                                free (key_val);
                        } else {
                                print_err ("Malformed 'set' syntax.");
                        }
                } else {
                        print_err ("No value to set to.");
                }
                ms_free (keynval);
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

        // TODO: fix this
        if (olstrcmp (f_arg, "exit")) {
                cfunc = func_exit;
        } else if (olstrcmp (f_arg, "cd")) {
                cfunc = func_cd;
        } else if (olstrcmp (f_arg, "pwd")) {
                cfunc = func_pwd;
        } else if (olstrcmp (f_arg, "set")) {
                cfunc = func_set;
        } else if (olstrcmp (f_arg, "unset")) {
                cfunc = func_unset;
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
