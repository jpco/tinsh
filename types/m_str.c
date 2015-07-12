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

        nms->str = calloc (len);
        if (nms->str == NULL) {
                free (nms);
                return NULL;
        }

        nms->mask = calloc (len);
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

char *ms_strchr (m_str *ms)
{
        // TODO: this
}

int ms_mstrcmp (m_str *first, m_str *second)
{
        // TODO: this
}

int ms_ustrcmp (m_str *first, m_str *second)
{
        return olstrcmp(first->str, second->str);
}

size_t ms_len (m_str *ms)
{
        return strlen (ms->str);
}

void ms_trim (m_str **ms)
{
        const char *buf = s;
        size_t i;
        for (i = 0; !m[i] && (s[i] == ' ' || s[i] == '\t'); i++) {
                buf++;
        }

        if (*buf == '\0') {
                *ns = calloc(1, sizeof(char));
                if (ns == NULL && errno == ENOMEM) {
                        return;
                }

                *nm = calloc(1, sizeof(char));
                if (nm == NULL && errno == ENOMEM) {
                        return;
                }

                return;
        }

        *ns = strdup (buf);
        *nm = calloc((strlen(buf) + 1), sizeof(char));

        memcpy (*nm, m+(buf-s), strlen(buf));

        for (i = strlen(buf) - 1; i >= 0 && i < strlen(buf) &&
                                !(*nm)[i] &&
                                ((*ns)[i] == '\n' ||
                                (*ns)[i] == ' '); i--) {
                (*ns)[i] = '\0';
                (*nm)[i] = '\0';
        }

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
