#include <stdlib.h>
#include <string.h>

// local includes
#include "eval_utils.h"
#include "mask.h"
#include "../var.h"
#include "../str.h"
#include "../exec.h"
#include "../defs.h"
#include "../debug.h"

// self-include
#include "var.h"

void var_eval (job_t *job)
{
        int a1mask = 0;
        int i;
        for (i = 0; i < strlen(job->argv[0])+1; i++) {
                a1mask += job->argm[0][i];
        }

        if (!a1mask) {
                char *alias = get_alias (job->argv[0]);
                if (alias != NULL) {
                        rm_element (job->argv, job->argm, 0, &(job->argc));
                        char *a_mask = mask_str (alias);
                        char **a_argv;
                        char **a_argm;
                        int a_argc;
                        spl_cmd (alias, a_mask, &a_argv, &a_argm, &a_argc);
                        free (alias);
                        free (a_mask);

                        int k;
                        for (k = 0; k < a_argc; k++) {
                                add_element (job->argv, job->argm,
                                                a_argv[k], a_argm[k], k,
                                                &(job->argc));
                        }

                        free (a_argv);
                        free (a_argm);
                }
        }

        for (i = 0; i < job->argc; i++) {
                char *arg = job->argv[i];
                // ~
                if (has_var ("__jpsh_~home")) {
                        char *home_ptr;
                        char *home_val = get_var ("__jpsh_~home");
                        char *home_mask = mask_str (home_val);

                        while ((home_ptr = masked_strchr (arg,
                                        job->argm[i], '~')) != NULL) {
                                *home_ptr = '\0';
                                char *nwd = vcombine_str (0, 3,
                                                arg,
                                                home_val,
                                                home_ptr+1);
                                char *nmask = calloc(strlen(nwd)+1,
                                                        sizeof(char));
                                memcpy(nmask, job->argm[i], strlen(arg));
                                memcpy(nmask+strlen(arg), home_mask,
                                        strlen(home_val));
                                memcpy(nmask+strlen(arg)+strlen(home_val),
                                        job->argm[i]+(home_ptr-arg)+1,
                                        strlen(home_ptr+1));

                                int len = job->argc;
                                rm_element (job->argv, job->argm, i, &len);
                                add_element (job->argv, job->argm,
                                                nwd, nmask, i, &len);
                        }

                        free (home_val);
                        free (home_mask);
                }

                arg = job->argv[i];

                // normal (foo) vars
                char *lparen = masked_strchr (arg, job->argm[i], '(');
                if (lparen == NULL) continue;

                char *rparen = masked_strchr (lparen,
                                        job->argm[i] + (lparen - arg),
                                        ')');
                if (rparen == NULL) {
                        dbg_print_err ("Mismatched parenthesis.");
                        continue;
                }

                *lparen = '\0';
                *rparen = '\0';
                int len = job->argc;

                char *value;
                if ((value = get_var (lparen+1)) != NULL) {
                } else if ((value = getenv (lparen+1)) != NULL) {
                        char *nvalue = strdup (value);
                        value = nvalue;
                }
                char *nwd = NULL;
                char *nmask = NULL;
                if (value != NULL) {
                        char *valmask = mask_str(value);
                        nwd = vcombine_str(0, 3, arg, value, rparen+1);

                        nmask = calloc(strlen(nwd)+1, sizeof(char));
                        memcpy(nmask, job->argm[i], strlen(arg));
                        memcpy(nmask+strlen(arg), valmask, strlen(value));
                        memcpy(nmask+strlen(arg)+strlen(value),
                                        job->argm[i]+(rparen-arg)+1,
                                        strlen(rparen+1));

                        free (value);
                        free (valmask);
                } else {
                        nwd = vcombine_str(0, 2, arg, rparen+1);

                        nmask = calloc(strlen(nwd)+1, sizeof(char));
                        memcpy(nmask, job->argm[i], strlen(arg));
                        memcpy(nmask+strlen(arg), job->argm[i]+(rparen-arg)+1,
                                        strlen(rparen+1));
                }

                if (nwd != NULL) {
                        char **v_argv;
                        char **v_argm;
                        int v_argc;
                        spl_cmd (nwd, nmask, &v_argv, &v_argm, &v_argc);
                        free (nwd);
                        free (nmask);

                        int k;
                        for (k = 0; k < v_argc; k++) {
                                add_element (job->argv, job->argm,
                                                v_argv[k], v_argm[k], k+i,
                                                &len);
                        }
                        i += k;
                        free (v_argv);
                        free (v_argm);
                }

                rm_element (job->argv, job->argm, i, &len);
                i -= 2;

                job->argc = len;
        }

        try_exec (job);
}

int devar (char *str, char *mask, char ***nstrs, char ***nmasks, int *strc) {
        char *lparen = masked_strchr (str, mask, '(');
        if (lparen == NULL) return -1;

        char *rparen = masked_strchr (lparen,
                                mask + (lparen - str),
                                ')');
        if (rparen == NULL) {
                dbg_print_err ("Mismatched parenthesis.");
                return -1;
        }

        *lparen = '\0';
        *rparen = '\0';

        char *value;
        if ((value = get_var (lparen+1)) != NULL) {
        } else if ((value = getenv (lparen+1)) != NULL) {
                char *nvalue = malloc((strlen(value)+1)
                                * sizeof(char));
                strcpy (nvalue, value);
                value = nvalue;
        }
        char *nwd = NULL;
        char *nmask = NULL;
        if (value != NULL) {
                char *valmask = mask_str(value);
                nwd = vcombine_str(0, 3, str, value, rparen+1);

                nmask = calloc(strlen(nwd)+1, sizeof(char));
                memcpy(nmask, mask, strlen(str));
                memcpy(nmask+strlen(str), valmask, strlen(value));
                memcpy(nmask+strlen(str)+strlen(value),
                                mask+(rparen-str)+1,
                                strlen(rparen+1));
        } else {
                nwd = vcombine_str(0, 2, str, rparen+1);

                nmask = calloc(strlen(nwd)+1, sizeof(char));
                memcpy(nmask, mask, strlen(str));
                memcpy(nmask+strlen(str), mask+(rparen-str)+1,
                                strlen(rparen+1));
        }
        if (nwd != NULL) {
                spl_cmd (nwd, nmask, nstrs, nmasks, strc);
                free (nwd);
                return 0;
        } else {
                return -1;
        }
}
