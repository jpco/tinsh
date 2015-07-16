#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// local includes
#include "../util/str.h"
#include "../util/debug.h"

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

        return nms;
}

void bs_pass (char **cmdline_ptr, char **cmdmask_ptr)
{
        char *cmdline = *cmdline_ptr;
        char *cmdmask = *cmdmask_ptr;

        char *buf = cmdline;
        char *mbuf = cmdmask;
        while ((buf = masked_strchr(buf, mbuf, '\\'))) {
                ptrdiff_t diff = buf - cmdline;
                mbuf = cmdmask + diff;
                rm_char (buf);
                arm_char (mbuf, strlen(cmdline) - diff + 1);
                *mbuf = '\\';
        }
}

void squote_pass (char **cmdline_ptr, char **cmdmask_ptr)
{
        char *cmdline = *cmdline_ptr;
        char *cmdmask = *cmdmask_ptr;

        char *buf = cmdline;
        char *mbuf = cmdmask;
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
}

void dquote_pass (char **cmdline_ptr, char **cmdmask_ptr)
{
        char *cmdline = *cmdline_ptr;
        char *cmdmask = *cmdmask_ptr;

        char *buf = cmdline;
        char *mbuf = cmdmask;
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
                        char cb = *sbuf;
                        if (is_separator(cb) && cb != '(' &&
                                cb != ')' && cb != '~' && cb != ';' && cb != '>' && cb != '<') {
                                ptrdiff_t sdiff = sbuf - cmdline;
                                cmdmask[sdiff] = '"';
                        }
                }
                buf = end;
                mbuf = ediff + mbuf;
        }
}

// TODO: Redo mask logic!
//
// HEY! | and ; in `` get masked!
// HEY! \ at the end of a line kills a newline!
//
m_str *ms_mask (char *str)
{
        m_str *nms = malloc (sizeof(m_str));
        if (nms == NULL) return NULL;

        char *mask = calloc (strlen(str)+1);
        if (mask == NULL) {
                free (nms);
                return NULL;
        }
        bs_pass (&str, &mask);
        squote_pass (&str, &mask);
        dquote_pass (&str, &mask);

        nms->str = str;
        nms->mask = mask;
        nms->len = strlen(str);

        return nms;
}

m_str *ms_dup (m_str *oms)
{
        m_str *nms = malloc (sizeof(m_str));
        if (nms == NULL) return NULL;

        nms->str = strdup (oms->str);
        nms->mask = calloc (ms_len(oms));
        memcpy (nms->mask, oms->mask, ms_len(oms));

        if (nms->str == NULL || nms->mask == NULL) {
                if (nms->str != NULL) free (nms->str);
                if (nms->mask != NULL) free (nms->mask);
                free (nms);

                return NULL;
        }

        return nms;
}

char *ms_strip (m_str *ms)
{
        return strdup (m_str->str);
}

char *ms_unmask (m_str *ms)
{
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
                if (ms->str[i] == c) return (char *)(ms->str+i)
        }

        return NULL;
}

int ms_mstrcmp (const m_str *first, const m_str *second)
{
        int i;
        for (i = 0; first->str[i] != '\0'; i++)
                if (first->str[i] != second->str[i]) return 0;
                if (first->mask[i] && !second->mask[i] ||
                    !first->mask[i] && second->mask[i]) return 0;
        return (second->str[i] == '\0');
}

int ms_ustrcmp (const m_str *first, const m_str *second)
{
        return olstrcmp(first->str, second->str);
}

size_t ms_len (const m_str *ms)
{
        return ms->len;
}

void ms_trim (m_str **ms_ptr)
{
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
        char *wdbuf = calloc(strlen(s)+1, sizeof(char));
        char *wmbuf = calloc(strlen(s)+1, sizeof(char));
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
                        argm[wdcount++]->mask = wmbuf;

                        wdbuf = calloc(strlen(s)+1, sizeof(char));
                        wmbuf = calloc(strlen(s)+1, sizeof(char));
                        wblen = 0;

                        wdflag = 0;
                }
        }

        if (strlen(wdbuf) > 0) {
                argv[wdcount] = malloc(sizeof(m_str));
                argv[wdcount]->str = wdbuf;
                argm[wdcount++]->mask = wmbuf;
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
        ms->len = strlen(ms->str);
}

void ms_free (m_str *ms)
{
        free (ms->str);
        free (ms->mask);
        free (ms);
}