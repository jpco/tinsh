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

#include "env.h"

// self-include
#include "var.h"

// TODO: rewrite this function. this function is bad.
void var_eval (job_j *job)
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

                arg = job->argv[i];

                // normal (foo) vars
                m_str *lparen = arg;
                if (!mbuf_strchr (lparen, '(')) continue;

                char *rparen = lparen->str;
                if ((rparen = ms_strchr(lparen, ')')) == NULL) {
                        dbg_print_err ("Mismatched parentheses.");
                        continue;
                }
                m_str *ms_rparen = ms_advance (lparen, 1+rparen-(lparen->str));
                *(ms_strchr (arg, ')')) = 0;
                ms_updatelen (arg);

                *(lparen->str) = '\0';
                *rparen = '\0';

                m_str *value;
                if ((value = devar (lparen->str+1)) != NULL) {
                } else if ((value = ms_mask (getenv (lparen->str+1))) != NULL) {
                        m_str *nvalue = ms_dup (value);
                        value = nvalue;
                }
                ms_updatelen (lparen);
                m_str *nwd;
                if (value != NULL) {
                        nwd = ms_vcombine (0, 3, arg, value, ms_rparen);
                } else {
                        nwd = ms_vcombine (0, 2, arg, ms_rparen);
                }
                m_str **nargs = ms_spl_cmd (nwd);
                rm_element (job->argv, i, &(job->argc));
                int j;
                for (j = 0; nargs[j] != NULL; j++) {
                        add_element (job->argv, nargs[j], i+j, &(job->argc));
                }
                i--;
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
        return NULL;
}
