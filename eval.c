#include <stddef.h>
// #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// local includes
#include "eval/queue.h"
#include "eval/eval_utils.h"

#include "str.h"
#include "debug.h"
#include "exec.h"
#include "var.h"

// self-include
#include "eval.h"

static queue *ejob_res;
static queue *ejobs;
static queue *jobs;

void mask_eval();
void comment_eval();
void subsh_eval();
void spl_line_eval();
void spl_pipe_eval();
void job_form();
void var_eval();

void eval (char *cmd)
{
        eval_m (cmd, NULL);
}

void free_ceval (void)
{
        if (ejob_res != NULL) {
                while (q_len (ejob_res) > 0) {
                        void *elt;
                        q_pop(ejob_res, &elt);
                        free (elt);
                }
                free (ejob_res);
                ejob_res = NULL;
        }

        if (ejobs != NULL) {
                while (q_len (ejobs) > 0) {
                        void *elt;
                        q_pop(ejobs, &elt);
                        free (elt);
                }
                free (ejobs);
                ejobs = NULL;
        }

        if (jobs != NULL) {
                while (q_len (jobs) > 0) {
                        void *elt;
                        q_pop(jobs, &elt);
                        free (elt);
                }
                free (jobs);
                jobs = NULL;
        }
}

void eval_m (char *cmd, char *mask)
{
        ejob_res = q_make();
        ejobs = q_make();
        jobs = q_make();
        atexit (free_ceval);

        char *ncmd = malloc((strlen(cmd)+1) * sizeof(char));
        strcpy(ncmd, cmd);

        q_push(ejob_res, ncmd);
        if (mask == NULL) {
                q_push(ejobs, mask_eval);
        } else {
                char *nmask = calloc((strlen(ncmd)+1), sizeof(char));
                memcpy(nmask, mask, strlen(ncmd));

                q_push(ejob_res, nmask);
                q_push(ejobs, comment_eval);
        }

        while (q_len(ejobs) > 0) {
                void (*ejob)(void);
                q_pop(ejobs, (void **)&ejob);
                ejob();
        }

        while (q_len(jobs) > 0) {
                job_t *job;
                q_pop (jobs, (void **)&job);

                job->job_fn(job);
        }

        free_ceval();
}

void mask_eval (void)
{
        char *cmdline;
        q_pop(ejob_res, (void **)&cmdline);

        char *cmdmask = mask_str(cmdline);

        q_push(ejob_res, cmdline);
        q_push(ejob_res, cmdmask);
        q_push(ejobs, comment_eval);
}

void comment_eval (void)
{
        char *cmdline;
        char *cmdmask;
        q_pop(ejob_res, (void **)&cmdline);
        q_pop(ejob_res, (void **)&cmdmask);

        char *pt = masked_strchr(cmdline, cmdmask, '#');
        if (pt != NULL) {
                *pt = '\0';
        }

        q_push(ejob_res, cmdline);
        q_push(ejob_res, cmdmask);
        q_push(ejobs, subsh_eval);
}

void subsh_eval (void)
{
        char *cmdline;
        char *cmdmask;
        q_pop(ejob_res, (void **)&cmdline);
        q_pop(ejob_res, (void **)&cmdmask);

        char *buf = cmdline-1;
        while ((buf = masked_strchr(buf+1, cmdmask, '`'))) {
                *buf = '\0';
                ptrdiff_t diff = buf + 1 - cmdline;
                char *end = masked_strchr(buf+1, cmdmask+diff, '`');
                if (end == NULL) {
                        print_err ("Unmatched \"`\" in command.");
                        return;
                }
                *end = '\0';
                ptrdiff_t ediff = end + 1 - cmdline;
                char *ret = subshell(buf+1, cmdmask+diff);

                char *rmask = mask_str(ret);

                char *nbuf = vcombine_str(0, 3, cmdline, ret, end+1);

                char *nmask = calloc(strlen(nbuf)+1, sizeof(char));
                memcpy(nmask, cmdmask, strlen(cmdline));
                memcpy(nmask + strlen(cmdline), rmask, strlen(ret));
                memcpy(nmask + strlen(cmdline) + strlen(ret),
                                cmdmask+ediff+1, strlen(cmdline+ediff+1));

                free (cmdline);
                free (cmdmask);
                cmdline = nbuf;
                cmdmask = nmask;
                buf = cmdline + ediff;
        }

        q_push(ejob_res, cmdline);
        q_push(ejob_res, cmdmask);
        q_push(ejobs, spl_line_eval);
}

void spl_line_eval (void)
{
        char *cmdline;
        char *cmdmask;
        q_pop(ejob_res, (void **)&cmdline);
        q_pop(ejob_res, (void **)&cmdmask);

        size_t cmdlen = strlen(cmdline);

        char *buf = cmdline;
        char *nbuf = masked_strchr(buf, cmdmask, ';');

        while (buf != NULL && buf - cmdline < cmdlen && *buf != '\0') {
                if (nbuf != NULL) {
                        *nbuf = '\0';
                } else {
                        nbuf = buf + strlen(buf);
                }
                if (buf == '\0') continue;

                ptrdiff_t bdiff = buf - cmdline;
                ptrdiff_t nbdiff = nbuf - buf;

                char *ncmd = malloc ((nbdiff+1) * sizeof(char));
                strcpy (ncmd, buf);

                char *nmask = calloc (nbdiff+1, sizeof(char));
                memcpy (nmask, cmdmask + bdiff, nbdiff + 1);

                buf = nbuf + 1;
                if (buf - cmdline < cmdlen) {
                        nbuf = masked_strchr(buf, cmdmask+nbdiff, ';');
                }

                q_push(ejob_res, ncmd);
                q_push(ejob_res, nmask);
                q_push(ejobs, spl_pipe_eval);
        }

        free (cmdline);
        free (cmdmask);
}

static job_t *prev_job;

void job_form (void)
{
        char *line;
        char *mask;
        char *job_out;
        q_pop(ejob_res, (void **)&line);
        q_pop(ejob_res, (void **)&mask);
        q_pop(ejob_res, (void **)&job_out);

        char *nline = NULL;
        char *nmask = NULL;
        masked_trim_str (line, mask, &nline, &nmask);
        free (line);
        free (mask);

        if (*nline == '\0') {
                free (nline);
                free (nmask);
                return;
        }

        job_t *job = malloc(sizeof(job_t));

        // defaults
        job->p_in = NULL;
        job->p_out = NULL;
        job->p_prev = NULL;
        job->p_next = NULL;
        job->file_in = NULL;
        job->file_out = NULL;
        job->bg = 0;

        spl_cmd (nline, nmask, &(job->argv), &(job->argm), &(job->argc));
        free (nline);
        free (nmask);

        // Background job?
        if (olstrcmp(job->argv[job->argc-1], "&")) {
                job->bg = 1;
                rm_element (job->argv, job->argm, (job->argc)-1,
                                &(job->argc));
        }

        // File redirection
        int i;
        for (i = 0; i < job->argc; i++) {
                if (olstrcmp(job->argv[i], ">") && !(*(job->argm[i]))) {
                        rm_element (job->argv, job->argm, i, &(job->argc));

                        if (i == job->argc) {      // they messed up
                                print_err ("Missing redirect destination.");
                                continue;
                        }

                        char *fname = malloc(strlen(job->argv[i])+1);
                        strcpy (fname, job->argv[i]);
                        rm_element (job->argv, job->argm, i, &(job->argc));
                        i -= 2;

                        job->file_out = fname;

                } else if (olstrcmp(job->argv[i], "<")
                                && !(*(job->argm[i]))) {
                        rm_element (job->argv, job->argm, i, &(job->argc));

                        if (i == job->argc) {      // they messed up
                                print_err ("Missing redirect source.");
                                continue;
                        }

                        char *fname = malloc(strlen(job->argv[i])+1);
                        strcpy (fname, job->argv[i]);
                        rm_element (job->argv, job->argm, i, &(job->argc));
                        i -= 2;

                        job->file_in = fname;
                }
        }

        // Piped from previous job?
        if (prev_job != NULL) {
                job->p_prev = prev_job;
                job->p_prev->p_next = job;
        }

        // Pipe to next job?
        if (job_out != NULL) {
                prev_job = job;
        } else {
                prev_job = NULL;
        }

        job->job_fn = var_eval;

        q_push (jobs, job);
}

void spl_pipe_eval (void)
{
        char *cmdline;
        char *cmdmask;
        q_pop(ejob_res, (void **)&cmdline);
        q_pop(ejob_res, (void **)&cmdmask);

        char *buf = cmdline;
        char *nbuf = masked_strchr(buf, cmdmask, '|');

        size_t cmdlen = strlen(cmdline);

        char lflag = 0;

        while (buf != NULL && buf - cmdline < cmdlen && *buf != '\0') {
                if (nbuf != NULL) {
                        *nbuf = '\0';
                } else {
                        nbuf = buf + strlen(buf);
                        lflag = 1;
                }
                if (buf == '\0') {
                        continue;
                }

                ptrdiff_t bdiff = buf - cmdline;
                ptrdiff_t nbdiff = nbuf - buf;

                char *ncmd = malloc((nbdiff+1) * sizeof(char));
                strcpy(ncmd, buf);

                char *nmask = calloc(nbdiff+1, sizeof(char));
                memcpy (nmask, cmdmask + bdiff, nbdiff);

                buf = nbuf + 1;
                if (!lflag) {
                        nbuf = masked_strchr(buf, cmdmask+nbdiff, '|');
                }
                q_push(ejob_res, ncmd);
                q_push(ejob_res, nmask);

                if (lflag) {
                        q_push(ejob_res, NULL);
                } else {
                        q_push(ejob_res, (void *)0xBEEFCAFE);
                }

                q_push(ejobs, job_form);
        }

        free (cmdline);
        free (cmdmask);
}

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
                        char *nvalue = malloc((strlen(value)+1)
                                        * sizeof(char));
                        strcpy (nvalue, value);
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
