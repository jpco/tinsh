#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

// local includes
#include "debug.h"
#include "str.h"
#include "env.h"

// self-include
#include "eval_utils.h"

void rm_element (char **argv, char **argm, int idx, int *argc)
{
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
                char *na, char *nm, int idx, int *argc)
{
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

void masked_trim_str (const char *s, const char *m, char **ns, char **nm)
{
        const char *buf = s;
        size_t i;
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
        memcpy (*nm, m+(buf-s), strlen(buf));

        for (i = strlen(buf) - 1; i >= 0 && i < strlen(buf) &&
                                !(*nm)[i] &&
                                ((*ns)[i] == '\n' ||
                                (*ns)[i] == ' '); i--) {
                (*ns)[i] = '\0';
                (*nm)[i] = '\0';
        }
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

void arm_char (char *line, size_t len)
{
        int i;
        for (i = 0; i < len; i++) {
                line[i] = line[i+1];
        }
}

char *mask_str (char *cmdline)
{
        // TODO: iron out subtleties of \, ', ` priority

        char *cmdmask = calloc(strlen(cmdline) + 1, sizeof(char));
        
        // \-pass
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

        // "-pass
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
                        if (*sbuf == ' ' || *sbuf == '>' ||
                                *sbuf == '<') {
                                ptrdiff_t sdiff = sbuf - cmdline;
                                cmdmask[sdiff] = '"';
                        }
                }
                buf = end;
                mbuf = ediff + mbuf;
        }

        return cmdmask;
}

char *unmask_str (char *str, char *mask)
{
        char *nstr = calloc((2*strlen(str)+1), sizeof(char));

        int i;
        int j = 0;
        int len = strlen(str);
        for (i = 0; i < len; i++) {
                if (mask[i]) {
                        nstr[j++] = '\\';
                }
                nstr[j++] = str[i];
        }

        return nstr;
}

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
