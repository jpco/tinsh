#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

// local includes
#include "../util/str.h"
#include "../util/debug.h"
#include "../util/defs.h"

// self-include
#include "m_str.h"

m_str *ms_make (size_t len)
{
        m_str *nms = malloc (sizeof(m_str));
        if (nms == NULL) return NULL;

        nms->str = calloc (len + 1, sizeof(char));
        if (nms->str == NULL) {
                free (nms);
                return NULL;
        }

        nms->mask = calloc (len + 1, sizeof(char));
        if (nms->mask == NULL) {
                free (nms->str);
                free (nms);
                return NULL;
        }
        nms->len = 0;

        return nms;
}

// TODO: Redo mask logic!
//
// HEY! | and ; in `` get masked!
// HEY! \ at the end of a line kills a newline!
//
m_str *ms_mask (const char *str)
{
        m_str *nms = calloc (sizeof(m_str), 1);
        if (nms == NULL) return NULL;

        if (str == NULL) {
                return nms;
        }

        nms->str = strdup(str);
        nms->mask = calloc (strlen(str)+1, sizeof(char));
        if (nms->mask == NULL) {
                free (nms);
                return NULL;
        }

        // masking
        int i;
        char squote = 0;
        char dquote = 0;
        char tick = 0;
        for (i = 0; i < strlen(nms->str); i++) {
                // dquote mask:
                // space, backslash, {, }, :,
                // squote mask: everything
                // bs mask: this char
                // tick: pipe
                size_t len = strlen(nms->str);

                if (nms->mask[i]) continue;

                        if (nms->str[i] == '\\') {
                                rm_char (nms->str + i);
                                arm_char (nms->mask + i, len - i);
                                nms->mask[i] = '\\';
                                i--;
                        }

                if (squote) {
                        if (nms->str[i] == '\'') {
                                // TODO: allow for \' not to end
                                // squotes
                                rm_char (nms->str + i);
                                arm_char (nms->mask + i, len - i);
                                squote = 0;
                                i--;
                        } else {
                                nms->mask[i] = '\'';
                        }
                } else {
                        if (dquote || tick) {
                                if (nms->str[i] == '"') {
                                        rm_char (nms->str + i);
                                        arm_char (nms->mask + i, len - i);
                                        dquote = 0;
                                        i--;
                                        continue;
                                } else if (nms->str[i] == '`') {
                                        tick = 0;
                                        continue;
                                }

                                char cchar = nms->str[i];
                                if (cchar == ' ' ||
                                    cchar == '|' || cchar == '{' ||
                                    cchar == '}' || cchar == ':') {
                                        nms->mask[i] = '"';
                                        continue;
                                }
                        }

                        if (nms->str[i] == '\'') {
                                rm_char (nms->str + i);
                                arm_char (nms->mask + i, len - i);
                                squote = 1;
                                i--;
                        } else if (nms->str[i] == '"') {
                                rm_char (nms->str + i);
                                arm_char (nms->mask + i, len - i);
                                dquote = 1;
                                i--;
                        } else if (nms->str[i] == '`') {
                                tick = 1;
                        }
                }
        }
        if (squote || dquote || tick) {
                print_err ("Unclosed quote or tick");
        }

        // finished
        ms_updatelen (nms);
        return nms;
}

m_str *ms_dup (m_str *oms)
{
        if (oms == NULL || oms->str == NULL || oms->mask == NULL)
                return NULL;
        m_str *nms = malloc (sizeof(m_str));
        if (nms == NULL) return NULL;

        nms->str = strdup (oms->str);
        nms->mask = calloc (ms_len(oms)+1, sizeof(char));
        memcpy (nms->mask, oms->mask, ms_len(oms));

        if (nms->str == NULL || nms->mask == NULL) {
                if (nms->str != NULL) free (nms->str);
                if (nms->mask != NULL) free (nms->mask);
                free (nms);

                return NULL;
        }

        ms_updatelen (nms);
        return nms;
}

char *ms_strip (m_str *ms)
{
        if (ms == NULL) return NULL;
        else return strdup (ms->str);
}

char *ms_unmask (m_str *ms)
{
        ms_updatelen (ms);
        char *nstr = calloc((2*ms_len(ms)+1), sizeof(char));
        if (nstr == NULL) return NULL;

        int i;
        int j = 0;
        int len = ms_len(ms);
        for (i = 0; i < len; i++) {
                if (ms->mask[i]) {
                        nstr[j++] = '\\';
                }
                nstr[j++] = ms->str[i];
        }

        return nstr;
}

char *ms_strchr (const m_str *ms, char c)
{
        if (ms == NULL) return NULL;
        size_t len = ms->len;
        size_t i;
        for (i = 0; i < len; i++) {
                if (ms->mask[i]) continue;
                if (ms->str[i] == c) return (char *)(ms->str+i);
        }

        return NULL;
}

int mbuf_strchr(m_str *mbuf, char d)
{
        if (mbuf == NULL) return 0;
        size_t len = mbuf->len;
        size_t i;
        for (i = 0; i < len; i++) {
                if (mbuf->mask[i]) continue;
                if (mbuf->str[i] == d) {
                        mbuf->str += i;
                        mbuf->mask += i;
                        ms_updatelen (mbuf);
                        return 1;
                }
        }
        return 0;
}

int ms_mstrcmp (const m_str *first, const m_str *second)
{
        int i;
        for (i = 0; first->str[i] != '\0'; i++)
                if (first->str[i] != second->str[i]) return 0;
                if ((first->mask[i] && !second->mask[i]) ||
                    (!first->mask[i] && second->mask[i])) return 0;
        return (second->str[i] == '\0');
}

int ms_ustrcmp (const m_str *first, const m_str *second)
{
        return olstrcmp(first->str, second->str);
}

size_t ms_len (const m_str *ms)
{
        if (ms == NULL) return 0;
        return ms->len;
}

void ms_trim (m_str **ms_ptr)
{
        if (ms_ptr == NULL || *ms_ptr == NULL) return;
        m_str *ms = *ms_ptr;
        const char *buf = ms->str;

        size_t i;
        for (i = 0; !ms->mask[i] &&
                        (ms->str[i] == ' ' || ms->str[i] == '\t'); i++) {
                buf++;
        }

        if (*buf == '\0') {
                *ms_ptr = ms_make (0);
                return;
        }
        m_str *nms = malloc (sizeof(m_str));
        if (nms == NULL) return;

        nms->str = strdup (buf);
        nms->mask = calloc (strlen(buf)+1, sizeof(char));
        memcpy (nms->mask, ms->mask + (buf - ms->str), strlen (buf));

        for (i = strlen(buf) - 1; i >= 0 && i < strlen (buf) &&
                        !(nms->mask[i]) &&
                        (nms->str[i] == '\n' || nms->str[i] == ' '); i--) {
                nms->str[i] = '\0';
                nms->mask[i] = '\0';
        }

        ms_updatelen (nms);
        *ms_ptr = nms;
}

char ms_rmchar (m_str *ms)
{
        char tr = *(ms->str);
        int i;
        int len = ms_len (ms);
        for (i = 0; i < len; i++) {
                ms->str[i] = ms->str[i+1];
                ms->mask[i] = ms->mask[i+1];
        }

        ms_updatelen (ms);
        return tr;
}

char ms_startswith (m_str *ms, char *pre)
{
        size_t len = strlen(pre);
        if (ms->len < len) return 0;

        int i;
        for (i = 0; i < len; i++) {
                if (ms->mask[i] || ms->str[i] != pre[i])
                        return 0;
        }

        return 1;
}

// TODO: make this one ms-native as well
m_str **ms_spl_cmd (const m_str *ms)
{
        char *s = ms->str;
        char *m = ms->mask;

        m_str **argv = calloc((1+strlen(s)), sizeof(m_str *));
        char null_m = 0;
        if (m == NULL) {
                m = calloc((1+strlen(s)), sizeof(char));
                null_m = 1;
        }

        size_t wdcount = 0;
        char wdflag = 0;
        char *wdbuf = calloc((ms->len)+1, sizeof(char));
        char *wmbuf = calloc((ms->len)+1, sizeof(char));
        size_t wblen = 0;
        for (const char *buf = s; buf != NULL && *buf != '\0'; buf++) {
                if (!m[buf-s]) {
                        if (*buf == ' ') {
                                continue;
                        }        
                }

                if (!m[buf-s+1]) {
                        if (buf[1] == ' ') {
                                wdflag = 1;
                        }
                }
                wdbuf[wblen] = *buf;
                wmbuf[wblen++] = m[buf-s];

                if (wdflag) {
                        argv[wdcount] = malloc(sizeof(m_str));
                        argv[wdcount]->str = wdbuf;
                        argv[wdcount]->mask = wmbuf;
                        argv[wdcount++]->len = wblen;

                        wdbuf = calloc(strlen(s)+1, sizeof(char));
                        wmbuf = calloc(strlen(s)+1, sizeof(char));
                        wblen = 0;

                        wdflag = 0;
                }
        }

        if (strlen(wdbuf) > 0) {
                argv[wdcount] = malloc(sizeof(m_str));
                argv[wdcount]->str = wdbuf;
                argv[wdcount]->mask = wmbuf;
                ms_updatelen(argv[wdcount++]);
        }

        if (null_m) {
                free ((char *)m);
        }

        return argv;
}

m_str *ms_dup_at (m_str *ms, char *s)
{
        if (s - ms->str < 0 || s - ms->str >= ms->len) {
                return NULL;
        }

        m_str *nms = malloc(sizeof(m_str));
        nms->str = strdup(s);
        nms->mask = calloc(strlen(s)+1, sizeof(char));
        memcpy(nms->mask, ms->mask+(s-(ms->str)), strlen(s));
        ms_updatelen(nms);

        return nms;
}

m_str *ms_advance (m_str *ms, size_t idx)
{
        if (idx >= ms->len) {
                return NULL;
        }

        m_str *nms = malloc(sizeof(m_str));
        nms->str = strdup((ms->str) + idx);
        nms->mask = calloc(strlen(nms->str) + 1, sizeof(char));
        memcpy(nms->mask, (ms->mask)+idx, strlen(nms->str));
        ms_updatelen(nms);

        return nms;
}

void ms_updatelen (m_str *ms)
{
        if (ms == NULL) return;
        if (ms->str == NULL) {
                ms->len = 0;
        } else {
                int i;
                for (i = 0; ms->str[i] != '\0'; i++);
                ms->len = i;
        }
}

m_str *ms_combine (const m_str **strs, int arrlen, char delim)
{
        int len = 0;
        int i;
        for (i = 0; i < arrlen; i++) {
                if (strs[i] == NULL || *(strs[i]->str) == '\0') continue;
                len += ms_len(strs[i]);
        }

        m_str *comb = ms_make (len+1);
        if (delim != '\0') {
                ms_free (comb);
                comb = ms_make (arrlen+len+1);
        }
        if (errno == ENOMEM) {
                print_err ("Could not malloc to combine m_str");
                return NULL;
        }

        int ctotal = 0;
        for (i = 0; i < arrlen; i++) {
                if (strs[i] == NULL) continue;
                int j;
                for (j = 0; strs[i]->str[j] != '\0'; j++) {
                        comb->str[ctotal+j] = strs[i]->str[j];
                        comb->mask[ctotal+j] = strs[i]->mask[j];
                }
                ctotal += j;
                if (delim != '\0' && i < arrlen-1) {
                        comb->str[ctotal] = delim;
                        comb->mask[ctotal++] = 0;
                }
        }
        return comb;
}

m_str *ms_vcombine (char delim, int ct, ...)
{
        va_list argl;
        const m_str *strarr[ct];

        va_start (argl, ct);
        int i;
        for (i = 0; i < ct; i++) {
                strarr[i] = va_arg(argl, const m_str *);
        }
        return ms_combine (strarr, ct, delim);
}

void ms_print (m_str *ms, int nl)
{
        if (ms == NULL) {
                if (nl) printf ("\n");
                return;
        }

        size_t i;
        for (i = 0; i < ms->len; i++) {
                if (ms->mask[i]) {
                        printf ("[7m%c[0m", ms->str[i]);
                } else {
                        printf ("%c", ms->str[i]);
                }
        }

        if (nl) printf ("\n");

}

void ms_free (m_str *ms)
{
        if (ms == NULL) return;
        if (ms->str != NULL) free (ms->str);
        if (ms->mask != NULL) free (ms->mask);
        free (ms);
}
