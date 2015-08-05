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

void tilde_eval (job_j *job);

// TODO: rewrite this function. this function is bad.
void var_eval (job_j *job)
{
        tilde_eval (job);

        int i;
        for (i = 0; i < job->argc; i++) {
                m_str *arg = job->argv[i];

                m_str **left_spl = ms_split (arg, '(');
                if (left_spl == NULL) continue;

                printf ("arg: ");
                ms_print (arg, 1);

                ms_free (arg);

                char matched = 1;
                char *varptr = left_spl[1]->str;
                int len = strlen(varptr);
                int j;
                for (j = 0; matched > 0 && j < len; j++) {
                        if (left_spl[1]->mask[j]) continue;
                        if (left_spl[1]->str[j] == '(') matched++;
                        else if (left_spl[1]->str[j] == ')') matched--;
                        varptr++;
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

                printf (" -- cut -- \n");
                printf ("0: ");
                ms_print (left_spl[0], 1);
                printf ("1: ");
                ms_print (left_spl[1], 1);
                printf ("2: ");
                ms_print (right, 1);

                m_str *value;
                if ((value = devar (ms_strip(left_spl[1]))) == NULL) {
                        value = ms_mask (getenv (ms_strip(left_spl[1])));
                }
                printf ("\n -- value --\n");
                ms_print (value, 1);
                ms_free (left_spl[1]);

                m_str *nwd = ms_vcombine (0, 3, left_spl[0], value, right);
                printf ("\n -- nwd --\n");
                ms_print (nwd, 1);

                ms_free (left_spl[0]);
                ms_free (right);

                ms_print (nwd, 1);
                m_str **nargs = ms_spl_cmd (nwd);
                rm_element (job->argv, i, &(job->argc));
                for (j = 0; nargs[j] != NULL; j++) {
                        add_element (job->argv, nargs[j], i+j, &(job->argc));
                }
                i--;
        }
}

void tilde_eval (job_j *job)
{
        int i;
        for (i = 0; i < job->argc; i++) {
                m_str *arg = job->argv[i];
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

                                        rm_element (job->argv, i, &(job->argc));
                                        add_element (job->argv, nwd, i, &(job->argc));
                                }
                        }
                }
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

        m_str *ret = ms_mask (subshell (name));
        if (ret->str[ret->len-1] == '\n' && !ret->mask[ret->len-1]) {
                ret->str[ret->len-1] = 0;
                ms_updatelen(ret);
        }
        return ret;
}
