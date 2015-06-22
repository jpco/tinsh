#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

// local includes
#include "../debug.h"
#include "../str.h"
#include "../var.h"
#include "queue.h"

// self-include
#include "eval_utils.h"

extern queue *ejob_res;
extern queue *ejobs;
extern queue *jobs;

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
