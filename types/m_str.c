#include <stdlib.h>
#include <string.h>

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

char *ms_strchr (const m_str *ms)
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
