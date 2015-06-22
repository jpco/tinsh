#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// local includes
#include "queue.h"
#include "eval_utils.h"
#include "comment.h"

#include "../str.h"
#include "../debug.h"
#include "../defs.h"

// self-include
#include "mask.h"

extern queue *ejob_res;
extern queue *ejobs;
extern queue *jobs;

void mask_eval (void)
{
        char *cmdline;
        q_pop(ejob_res, (void **)&cmdline);

        char *cmdmask = mask_str(cmdline);

        q_push(ejob_res, cmdline);
        q_push(ejob_res, cmdmask);
        q_push(ejobs, comment_eval);
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

char *mask_str (char *cmdline)
{
        // TODO: iron out subtleties of \, ', ` priority

        char *cmdmask = calloc(strlen(cmdline) + 1, sizeof(char));        
        bs_pass (&cmdline, &cmdmask);
        squote_pass (&cmdline, &cmdmask);
        dquote_pass (&cmdline, &cmdmask);
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
