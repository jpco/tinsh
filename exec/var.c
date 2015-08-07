#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// local includes
#include "../types/m_str.h"
#include "../types/job.h"
#include "../types/var.h"

#include "../util/str.h"
#include "../util/debug.h"
#include "../util/vector.h"

#include "subshell.h"
#include "env.h"

// self-include
#include "var.h"

void tilde_eval (m_str **argv, size_t argc);

void var_eval (job_j *job)
{
        job->argc = devar_argv(job->argv, job->argc);
}

void devar_arg (m_str **arg)
{
        m_str **args = ms_spl_cmd (*arg);
        size_t argc;
        for (argc = 0; args[argc] != NULL; argc++);

        argc = devar_argv (args, argc);

        *arg = ms_combine ((const m_str **)args, argc, ' ');

        int i;
        for (i = 0; i < argc; i++) {
                ms_free (args[i]);
                free (args);
        }
}

size_t devar_argv (m_str **argv, size_t argc)
{
        tilde_eval (argv, argc);

        int i;
        for (i = 0; i < argc; i++) {
                m_str *arg = ms_dup(argv[i]);

                m_str **left_spl = ms_split (arg, '(');
                if (left_spl == NULL) continue;

                ms_free (arg);

                char matched = 1;
                char *varptr = left_spl[1]->str;
                int len = strlen(varptr);
                int j;
                for (j = 0; matched > 0 && j < len; j++) {
                        varptr++;
                        if (left_spl[1]->mask[j]) continue;
                        if (left_spl[1]->str[j] == '(') matched++;
                        else if (left_spl[1]->str[j] == ')') matched--;
                }
                if (matched > 0) {
                        print_err ("Missing ')'.");
                } else {
                        *(varptr-1) = 0;
                }
                m_str *right = ms_advance (left_spl[1],
                                        varptr-left_spl[1]->str);
                ms_updatelen (left_spl[1]);
                ms_updatelen (right);

                m_str *value;
                if ((value = devar (ms_strip(left_spl[1]))) == NULL) {
                        value = ms_mask (getenv (ms_strip(left_spl[1])));
                }
                ms_free (left_spl[1]);

                m_str *nwd = ms_vcombine (0, 3, left_spl[0], value, right);

                ms_free (left_spl[0]);
                ms_free (right);

                m_str **nargs = ms_spl_cmd (nwd);
                rm_element (argv, i, &argc);
                for (j = 0; nargs[j] != NULL; j++) {
                        add_element (argv, nargs[j], i+j, &argc);
                }
                i--;
        }

        return argc;
}

void tilde_eval (m_str **argv, size_t argc)
{
        int i;
        for (i = 0; i < argc; i++) {
                m_str *arg = ms_dup(argv[i]);
                // ~
                if (ms_strchr (arg, '~')) {
                        if (devar ("__tin_home")) {
                                m_str *home_ptr = ms_dup (arg);
                                m_str *home_val = devar ("__tin_home");

                                while (mbuf_strchr (home_ptr, '~')) {
                                        home_ptr = ms_advance (home_ptr, 1);
                                        if (ms_strchr (arg, '~')) {
                                                *(ms_strchr (arg, '~')) = '\0';
                                        }
                                        ms_updatelen (arg);
                                        ms_updatelen (home_ptr);
                                        m_str *nwd = ms_vcombine (0, 3,
                                                        arg,
                                                        home_val,
                                                        home_ptr);

                                        rm_element (argv, i, &argc);
                                        add_element (argv, nwd, i, &argc);
                                }
                        }
                }
                ms_free (arg);
        }
}

m_str *devar(char *name)
{
        var_j *varval = get_var (name);
        if (varval != NULL) {
                if (!varval->is_fn) {
                        return varval->v.value;
                }
        } else {
                char *envval;
                if ((envval = getenv (name)) != NULL) {
                        return ms_mask (envval);
                }
        }

        // eval/exec time
        // TODO: function handling goes here        

        m_str *ret = ms_mask (subexec (name));
        while (ms_strchr(ret, '\n') != NULL) {
                *(ms_strchr(ret, '\n')) = ' ';
                ms_updatelen(ret);
        }
        return ret;
}
