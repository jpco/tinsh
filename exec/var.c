#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// local includes
#include "../types/m_str.h"
#include "../types/job.h"

#include "../util/str.h"
#include "../util/debug.h"
#include "../util/vector.h"

#include "env.h"

// self-include
#include "var.h"

void var_eval (job_j *job)
{
        int a1mask = 0;
        int i;
        for (i = 0; i < ms_len(job->argv[0])+1; i++) {
                a1mask += job->argv[0]->mask[i];
        }

        if (!a1mask) {
                m_str *alias = devar (ms_strip (job->argv[0]));

                if (alias != NULL) {
                        rm_element (job->argv, 0, &(job->argc));
                        m_str **spl_alias = ms_spl_cmd (alias);

                        int k;
                        for (k = 0; spl_alias[k] != NULL; k++) {
                                add_element (job->argv, spl_alias[k], k,
                                                &(job->argc));
                        }
                }
        }

        for (i = 0; i < job->argc; i++) {
                m_str *arg = job->argv[i];
                // ~
                if (ms_strchr (arg, '~')) {
                        if (devar ("__imp_~home")) {
                                m_str *home_ptr = ms_dup (arg);
                                m_str *home_val = devar ("__imp_~home");

                                while (mbuf_strchr (home_ptr, '~')) {
                                        home_ptr = ms_advance (home_ptr, 1);
                                        if (ms_strchr (arg, '~')) {
                                                *(ms_strchr (arg, '~')) = '\0';
                                        }
                                        m_str *nwd = ms_vcombine (0, 3,
                                                        arg,
                                                        home_val,
                                                        home_ptr);

                                        size_t len = job->argc;
                                        rm_element (job->argv, i, &len);
                                        add_element (job->argv, nwd, i, &len);
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

                *(lparen->str) = '\0';
                *rparen = '\0';
                size_t len = job->argc;

                m_str *value;
                if ((value = devar (lparen->str+1)) != NULL) {
                } else if ((value = ms_mask (getenv (lparen->str+1))) != NULL) {
                        m_str *nvalue = ms_dup (value);
                        value = nvalue;
                }
                m_str *nwd = NULL;
                if (value != NULL) {
                        nwd = ms_vcombine (0, 3, arg, value, ms_advance (lparen, rparen-(lparen->str)));
                } else {
                        nwd = ms_vcombine (0, 2, arg, ms_advance (lparen, (rparen-(lparen->str))));
                }

                if (nwd != NULL) {
                        m_str **spl_val = ms_spl_cmd (nwd);
                        ms_free (nwd);

                        int k;
                        for (k = 0; spl_val[k] != NULL; k++) {
                                add_element (job->argv,
                                                spl_val[k], k+i, &len);
                        }
                        i += k;
                }

                rm_element (job->argv, i, &len);
                i-=2;
                if (i < 0) i = 0;

                job->argc = len;
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
