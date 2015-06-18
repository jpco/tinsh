#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// local includes
#include "queue.h"
#include "str.h"
#include "debug.h"
#include "exec.h"
#include "env.h"

// self-include
#include "eval.h"

// SECTION EVAL FUNCTIONS

static queue *ejob_res;
static queue *ejobs;
static queue *jobs;

void mask_eval();
void subsh_eval();
void spl_line_eval();
void spl_pipe_eval();
void job_form();
void var_eval();

void free_ceval();

void rm_element (char **argv, char **argm, int idx, int *argc);
void add_element (char **argv, char **argm, char *na, char *nm, int idx,
                int *argc);
char *masked_strchr(const char *s, const char *m, char c);
void masked_trim_str(const char *s, const char *m, char **ns, char **nm);
void spl_cmd (const char *s, const char *m, char ***argv, char ***argm,
                int *argc);

void print_msg (char *msg, char *mask, int nl)
{
        size_t i;
        size_t len = strlen(msg);
        for (i = 0; i < len; i++) {
                if (mask[i]) {
                        printf ("[7m%c[0m", msg[i]);
                } else {
                        printf ("%c", msg[i]);
                }
        }

        if (nl) printf ("\n");
}

void eval (char *cmd)
{
        ejob_res = q_make();
        ejobs = q_make();
        jobs = q_make();

        char *ncmd = malloc((strlen(cmd)+1) * sizeof(char));
        strcpy(ncmd, cmd);

        q_push(ejob_res, ncmd);
        q_push(ejobs, mask_eval);

        while (q_len(ejobs) > 0) {
                void (*ejob)(void);
                q_pop(ejobs, (void **)&ejob);
                ejob();
        }

        job_t *job;
        q_pop (ejob_res, (void **)&job);
        if (job != NULL) {
                try_exec (job);
        }
}

// Takes care of \ and ' masking.
void mask_eval (void)
{
        // TODO: iron out subtleties of \, ', ` priority

        char *cmdline;
        q_pop(ejob_res, (void **)&cmdline);

        char *cmdmask = calloc(strlen(cmdline) + 1, sizeof(char));
        
        char *buf = cmdline;
        char *mbuf = cmdmask;
        while ((buf = masked_strchr(buf, mbuf, '\\'))) {
                ptrdiff_t diff = buf - cmdline;
                mbuf = cmdmask + diff;
                rm_char (buf);
                arm_char (mbuf, strlen(cmdline) - diff + 1);
                *mbuf = '\\';
        }

        // '-pass
        buf = cmdline;
        mbuf = cmdmask;
        while ((buf = masked_strchr(buf, mbuf, '\''))) {
                ptrdiff_t diff = buf - cmdline;
                mbuf = cmdmask + diff;

                rm_char (buf);
                arm_char (mbuf, strlen(cmdline) - diff + 1);
                char *end = masked_strchr(buf, mbuf, '\'');
                ptrdiff_t ediff = end - buf;

                if (end == NULL) {
                        print_err ("Unmatched ' in command.");
                        end = buf + strlen(buf);
                        ediff = end - buf;
                } else {
                        rm_char (end);
                        arm_char (mbuf + ediff,
                                        strlen(cmdline) - ediff - diff);
                }

                memset (mbuf, '\'', ediff);
                buf = end;
                mbuf = ediff + mbuf;
        }

        // '-pass
        buf = cmdline;
        mbuf = cmdmask;
        while ((buf = masked_strchr(buf, mbuf, '"'))) {
                ptrdiff_t diff = buf - cmdline;
                mbuf = cmdmask + diff;

                rm_char (buf);
                arm_char (mbuf, strlen(cmdline) - diff + 1);
                char *end = masked_strchr(buf, mbuf, '"');
                ptrdiff_t ediff = end - buf;

                if (end == NULL) {
                        print_err ("Unmatched \" in command.");
                        end = buf + strlen(buf);
                        ediff = end - buf;
                } else {
                        rm_char (end);
                        arm_char (mbuf + ediff,
                                        strlen(cmdline) - ediff - diff);
                }

                char *sbuf;
                for (sbuf = buf; sbuf < end; sbuf++) {
                        if (*sbuf == ' ') {
                                ptrdiff_t sdiff = sbuf - cmdline;
                                cmdmask[sdiff] = '"';
                        }
                }
                buf = end;
                mbuf = ediff + mbuf;
        }

//        printf ("Masked: ");
//        print_msg (cmdline, cmdmask, 1);

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
                char *ret = subshell(buf+1);

                // Hacky but here it is
                // TODO: split out mask_create from mask_eval,
                //       then just call mask_create from here and not
                //       muck up the queue
                q_push(ejob_res, ret);
                mask_eval();
                char *rmask;
                void (*throwaway)(void);
                q_pop(ejob_res, (void **)&ret);
                q_pop(ejob_res, (void **)&rmask);
                q_pop(ejobs, (void **)&throwaway);  // gross

                size_t retlen = strlen(ret);
                char *nbuf = vcombine_str(0, 3, cmdline, ret, end+1);

                char *nmask = calloc(strlen(nbuf)+1, sizeof(char));
                memcpy(nmask, cmdmask, strlen(cmdline));
                memcpy(nmask + strlen(cmdline), rmask, strlen(rmask));
                memcpy(nmask + strlen(cmdline) + strlen(rmask),
                                cmdmask+ediff+1, strlen(cmdline+ediff+1));

                free (cmdline);
                free (cmdmask);
                cmdline = nbuf;
                cmdmask = nmask;
                buf = cmdline + ediff;
        }

//        printf("Subshelled: ");
//        print_msg(cmdline, cmdmask, 1);

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

                char *ncmd = malloc((nbdiff+1) * sizeof(char));
                strcpy(ncmd, buf);

                char *nmask = calloc(nbdiff+1, sizeof(char));
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

void job_form (void)
{
        char *line;
        char *mask;
        int *fds;
        q_pop(ejob_res, (void **)&line);
        q_pop(ejob_res, (void **)&mask);
        q_pop(ejob_res, (void **)&fds);

        char *nline = NULL;
        char *nmask = NULL;
        masked_trim_str (line, mask, &nline, &nmask);
        free (line);
        free (mask);

        if (*nline == '\0') {
                free (nline);
                free (nmask);
                free (fds);
                return;
        }

        job_t *job = malloc(sizeof(job_t));
        if (fds[0] != -1) job->in_fd = fds[0];
        else job->in_fd = STDIN_FILENO;

        if (fds[1] != -1) job->out_fd = fds[1];
        else job->out_fd = STDOUT_FILENO;

        char **argm;
        spl_cmd (nline, nmask, &(job->argv), &argm, &(job->argc));
        if (olstrcmp(job->argv[job->argc-1], "&")) {
                job->bg = 1;
                rm_element (job->argv, argm, (job->argc)-1, &(job->argc));
        }

        int i;
        for (i = 0; i < job->argc; i++) {
                if (olstrcmp(job->argv[i], ">") && !(*argm[i])) {
                        rm_element (job->argv, argm, i, &(job->argc));

                        if (i == job->argc) {      // they messed up
                                print_err ("Missing redirect destination.");
                                continue;
                        }
                        int j;
                        for (j = 0; j < job->argc; j++) {
                                printf (" - %s\n", job->argv[j]);
                        }
                        printf (" - correct is %s\n", job->argv[i]);
                        char *fname = malloc(strlen(job->argv[i])+1);
                        strcpy (fname, job->argv[i]);
                        rm_element (job->argv, argm, i, &(job->argc));
                        i -= 2;

                        printf ("FILE NAME = %s\n", fname);
                        int fd = open(fname, O_RDWR | O_CREAT);
                        if (fd >= 0) {
                                if (job->out_fd != -1) close(job->out_fd);
                                job->out_fd = fd;
                        } else {
                                dbg_print_err_wno ("Could not open"
                                                "redirect destination.",
                                                errno);
                        }

                        free (fname);

                } else if (olstrcmp(job->argv[i], "<") && !(*argm[i])) {
                        rm_element (job->argv, argm, i, &(job->argc));

                        if (i == job->argc) {      // they messed up
                                print_err ("Missing redirect source.");
                                continue;
                        }
                        int j;
                        for (j = 0; j < job->argc; j++) {
                                printf (" - %s\n", job->argv[j]);
                        }
                        printf (" - correct is %s\n", job->argv[i]);
                        char *fname = malloc(strlen(job->argv[i])+1);
                        strcpy (fname, job->argv[i]);
                        rm_element (job->argv, argm, i, &(job->argc));
                        i -= 2;

                        printf ("FILE NAME = %s\n", fname);
                        int fd = open(fname, O_RDWR | O_TRUNC | O_CREAT,
                                        0664);
                        if (fd >= 0) {
                                if (job->in_fd != -1) close(job->in_fd);
                                job->in_fd = fd;
                        } else {
                                dbg_print_err_wno ("Could not open"
                                                "redirect source.",
                                                errno);
                        }

                        free (fname);
                }
        }

        q_push(ejob_res, job);
        q_push(ejob_res, argm);
        q_push(ejobs, var_eval);
}

void spl_pipe_eval (void)
{
        char *cmdline;
        char *cmdmask;
        q_pop(ejob_res, (void **)&cmdline);
        q_pop(ejob_res, (void **)&cmdmask);

        char *buf = cmdline;
        char *nbuf = masked_strchr(buf, cmdmask, '|');

//        printf ("pre-pipe: ");
//        print_msg (cmdline, cmdmask, 1);

        size_t cmdlen = strlen(cmdline);

        char fflag = 1;
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

                int *fds = malloc(2 * sizeof(int));

                if (!(fflag && lflag)) {
                        pipe (fds);

                        if (fflag) {
                                close (fds[0]);
                                fds[0] = -1;
                        }
                        if (lflag) {
                                close (fds[1]);
                                fds[1] = -1;
                        }
                } else {
                        fds[0] = -1;
                        fds[1] = -1;
                }

                q_push(ejob_res, ncmd);
                q_push(ejob_res, nmask);
                q_push(ejob_res, fds);
                q_push(ejobs, job_form);

                fflag = 0;
        }
}

void var_eval (void)
{
        job_t *job;
        char **argm;
        q_pop(ejob_res, (void **)&job);
        q_pop(ejob_res, (void **)&argm);

        int a1mask = 0;
        int i;
        for (i = 0; i < strlen(job->argv[0])+1; i++) {
                a1mask += argm[0][i];
        }

        if (!a1mask) {
                char *alias = get_alias (job->argv[0]);
                if (alias != NULL) {
                        rm_element (job->argv, argm, 0, &(job->argc));
                        // TODO: mask eval!!
                        char **a_argv;
                        char **a_argm;
                        int a_argc;
                        spl_cmd (alias, NULL, &a_argv, &a_argm, &a_argc);
                        int k;
                        for (k = 0; k < a_argc; k++) {
                                add_element (job->argv, argm, a_argv[k],
                                                NULL, k, &(job->argc));
                        }
                }
        }

        for (i = 0; i < job->argc; i++) {
                char *arg = job->argv[i];
                char *lparen = masked_strchr (arg, argm[i], '(');
                if (lparen == NULL) continue;

                char *rparen = masked_strchr (lparen,
                                        argm[i] + (lparen - arg),
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
                if (value != NULL) {
                        nwd = vcombine_str(0, 3, arg, value, rparen+1);
                } else {
                        nwd = vcombine_str(0, 2, arg, rparen+1);
                }
                if (nwd != NULL) {
                        // todo: VAR MASKING
                        char **v_argv;
                        char **v_argm;
                        int v_argc;
                        spl_cmd (nwd, NULL, &v_argv, &v_argm, &v_argc);
                        int k;
                        for (k = 0; k < v_argc; k++) {
                                add_element (job->argv, argm, v_argv[k],
                                                NULL, k+i, &len);
                        }
                        i += k;
                        free (nwd);
                }

                rm_element (job->argv, argm, i, &len);
                i -= 2;

                job->argc = len;
        }

        q_push (ejob_res, job);
}

void spl_cmd (const char *s, const char *m, char ***argv, char ***argm, 
                int *argc)
{
        *argv = calloc((1+strlen(s)), sizeof(char *));
        *argm = calloc((1+strlen(s)), sizeof(char *));
        char null_m = 0;
        if (m == NULL) {
                m = calloc((1+strlen(s)), sizeof(char));
                null_m = 1;
        }

        size_t wdcount = 0;
        char wdflag = 0;
        char *wdbuf = calloc(strlen(s)+1, sizeof(char));
        char *wmbuf = calloc(strlen(s)+1, sizeof(char));
        size_t wblen = 0;
        for (const char *buf = s; buf != NULL && *buf != '\0'; buf++) {
                if (!m[buf-s]) {
                        if (*buf == ' ') {
                                continue;
                        }
        
                        if (*buf == '>' || *buf == '<' || *buf == '&') {
                                wdflag = 1;
                        }
                }

                if (!m[buf-s+1]) {
                        if (buf[1] == ' ' || buf[1] == '>' ||
                            buf[1] == '<' || buf[1] == '&') {
                                wdflag = 1;
                        }
                }
                wdbuf[wblen] = *buf;
                wmbuf[wblen++] = m[buf-s];

                if (wdflag) {
                        (*argv)[wdcount] = wdbuf;
                        (*argm)[wdcount++] = wmbuf;

                        wdbuf = calloc(strlen(s)+1, sizeof(char));
                        wmbuf = calloc(strlen(s)+1, sizeof(char));
                        wblen = 0;

                        wdflag = 0;
                }
        }

        if (strlen(wdbuf) > 0) {
                (*argv)[wdcount] = wdbuf;
                (*argm)[wdcount++] = wmbuf;
        }

        *argc = wdcount;
        if (null_m) {
                free ((char *)m);
        }
}

void masked_trim_str (const char *s, const char *m, char **ns, char **nm)
{
        const char *buf = s;
        size_t i;
        size_t len = strlen(s);
        for (i = 0; !m[i] && (s[i] == ' ' || s[i] == '\t'); i++) {
                buf++;
        }

        if (*buf == '\0') {
                *ns = calloc(1, sizeof(char));
                *nm = calloc(1, sizeof(char));
                return;
        }

        *ns = calloc((strlen(buf) + 1), sizeof(char));
        *nm = calloc((strlen(buf) + 1), sizeof(char));
        strcpy (*ns, buf);
        memcpy (*nm, m, strlen(buf));

        for (i = strlen(buf) - 1; i >= 0 && i < strlen(buf) &&
                                !(*nm)[i] &&
                                ((*ns)[i] == '\n' ||
                                (*ns)[i] == ' '); i--) {
                (*ns)[i] = '\0';
                (*nm)[i] = '\0';
        }
}

void rm_element (char **argv, char **argm,
                int idx, int *argc) {
        free (argv[idx]);
        free (argm[idx]);
        int i;
        for (i = idx; i < *argc-1; i++) {
               argv[i] = argv[i+1];
               argm[i] = argm[i+1];
        }
        argv[*argc-1] = NULL;
        argm[*argc-1] = NULL;

        (*argc)--;
}

void add_element (char **argv, char **argm,
                char *na, char *nm, int idx, int *argc) {
        if (nm == NULL) {
                nm = calloc(strlen(na)+1, sizeof(char));
        }
        int i;
        for (i = *argc; i > idx; i--) {
                argv[i] = argv[i-1];
                argm[i] = argm[i-1];
        }
        argv[idx] = na;
        argm[idx] = nm;

        (*argc)++;
}

char *masked_strchr (const char *s, const char *m, char c)
{
        size_t len = strlen(s);
        size_t i;
        for (i = 0; i < len; i++) {
                if (m[i]) {
                        continue;
                }
                if (s[i] == c) return (char *)(s+i);
        }

        return NULL;
}

void free_ceval (void)
{
//       free (cmdline);
}
