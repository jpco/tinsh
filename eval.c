#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// local includes
#include "queue.h"
#include "str.h"
#include "debug.h"
#include "exec.h"

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
                strncpy(nmask, cmdmask, strlen(cmdline));
                strncpy(nmask + strlen(cmdline), rmask, strlen(rmask));
                strncpy(nmask + strlen(cmdline) + strlen(rmask),
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
                strncpy (nmask, cmdmask + bdiff, nbdiff);

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

        char *nline;
        char *nmask;
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
        int argc;
        spl_cmd (nline, nmask, &(job->argv), &argm, &argc);
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
                strncpy (nmask, cmdmask + bdiff, nbdiff);

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

}

void spl_cmd (const char *s, const char *m, char ***argv, char ***argm, 
                int *argc)
{
        *argv = malloc(strlen(s) * sizeof(char *));
        *argm = malloc(strlen(s) * sizeof(char *));

        size_t wdcount = 0;
        char wdflag = 1;
        char *wdbuf = calloc(strlen(s)+1, sizeof(char));
        for (const char *buf = s; buf != NULL && *buf != '\0'; buf++) {
                
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

        *ns = malloc((strlen(buf) + 1) * sizeof(char));
        *nm = malloc((strlen(buf) + 1) * sizeof(char));
        strcpy (*ns, buf);
        strncpy (*nm, m, strlen(buf));

        for (i = strlen(buf) - 1; i >= 0 && !(*nm)[i] &&
                                ((*ns)[i] == '\n' ||
                                (*ns)[i] == ' '); i--) {
                (*ns)[i] = '\0';
                (*nm)[i] = '\0';
        }
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
