#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// local includes
#include "defs.h"
#include "str.h"
#include "env.h"
#include "hist.h"
#include "term.h"
#include "debug.h"
#include "color.h"
#include "compl.h"

// self-include
#include "linebuffer.h"

static int idx;
static int length;
static int prompt_length;

static char *buf;
static char *sprompt;
static char *wds[MAX_LINE];
static int nwds;

/**
 * Decides the prompt & prints it
 */
void prompt (void)
{
        if (has_var ("prompt")) {
                // TODO: parse prompt
                sprompt = get_var ("__jpsh_prompt");
        } else {
                char *co = get_var ("__jpsh_color");
                if (co) {
                        sprompt = vcombine_str('\0', 3, "\e[1m..",
                                strrchr (getenv ("PWD"), '/'), "$\e[0m ");
                } else {
                        sprompt = vcombine_str('\0', 3, "..",
                                strrchr (getenv ("PWD"), '/'), "$ ");
                }
                free (co);
        }

        int row = 0;
        printf("%s", sprompt);
        fflush(stdout);         // so cursor_pos works correctly
        if (cursor_pos (&row, &prompt_length)) prompt_length = 25;
}

void reprint (void)
{
        /* For reference because I know I'm going to forget:
        printf("[%dG", prompt_length);        // move to beginning
        printf("%s", buf);                      // print line
        printf("[K");                         // clear rest of line
        printf("[%dG", prompt_length + idx);  // move back to idx
        */

        printf("[%dG%s[K[%dG", prompt_length, buf,
                        prompt_length + idx);
}

void rewd (void)
{
        nwds = 0;
        int i;
        for (i = 0; i < length; i++) {
                int isi = (i == 0 ? 1 : is_separator(buf[i]));
                int ibsi1 = (i < 1 || buf[i-1] != '\\' ? 0 : 1);
                int isi1 = (i < 1 ? 0 : is_separator(buf[i-1]));
                int ibsi2 = (i < 2 || buf[i-2] != '\\' ? 0 : 1);

                if ((isi && !ibsi1) || (isi1 && !ibsi2)) {
                        wds[nwds++] = buf+i;
                }
        }
        for (i = nwds; i < MAX_LINE; i++) {
                wds[i] = NULL;
        }
}

void rmwd (int index)
{
        char *loc = buf + index;
        int i;
        for (i = 0; loc > wds[i]; i++);
        int j;
        for (j = i; j < nwds; j++) {
                wds[j] = wds[j+1];
        }
        nwds--;
}
void thiswd (char **start, char **end)
{
        int i;
        for (i = nwds-1; i >= 0 && wds[i] > buf+idx; i--);
        *start = wds[i];
        *end = wds[i+1];
}

void buffer (char cin)
{
        if (idx < length) {
                int i;
                for (i = length; i > idx; i--)
                        buf[i] = buf[i-1];
        }

        int i;
        buf[idx++] = cin;
        length++;

        rewd();
        reprint();
}

void rebuffer (char *sin)
{
        int cidx = idx;
        int clen = length;
        memset (buf, 0, MAX_LINE * sizeof(char));
        if (*sin == '\0') {
                length = 0;
        } else {
                strcpy (buf, sin);
                length = strlen(sin);
        }
        idx = length;
        rewd();
        reprint();
}

void sbuffer (char *sin)
{
        int inlen = strlen(sin);
        int i;
        if (idx < length) {
                for (i = length; i > (idx + inlen); i--)
                        buf[i] = buf[i-inlen];
        }

        for (i = 0; i < inlen; i++) {
                buf[idx++] = sin[i];
        }
        length += inlen;

        rewd();
        reprint();
}

void buffer_rm (int bksp)
{
        if (bksp == 1 && idx == 0) return;
        if (length == 0) return;

        if (bksp) idx--;

        int i;
        for (i = idx; i < length; i++)
                buf[i] = buf[i+1];

        for (i = 0; i < nwds; i++) {
                if (wds[i] == buf+idx) {
                        rmwd(idx);
                        break;
                }
        }

        if (idx == length) idx--;
        length--;

        reprint();
}

void buffer_mv (char dir)
{
        if (dir == 'D') {// left
                if (idx > 0) idx--;
        } else if (dir == 'C') {// right
                if (idx < length) idx++;
        }

        reprint();
}

void buffer_cpl (void)
{
        int cidx = idx;
        int clen = length;
        char *cplst, *cplend;
        thiswd (&cplst, &cplend);
        cplend = buf + idx;
        char *nbuf = l_compl (buf, cplst, cplend);
        if (nbuf != NULL) {
                rebuffer(nbuf);
        }
        free (nbuf);
}

/**
 * Interprets the given typed character, based on the current status.
 * Returns the new status, from the typed character. Statuses are used
 * for multi-character keypresses, like backspace, delete, the arrow
 * keys, etc.
 *
 * Arguments:
 *  - cin: character just typed
 *  - status: current status
 * Returns:
 *  - -1 if newline was pressed
 *  - 0 if character wasn't special or finished a special character.
 *  - 1 if character was .
 *  - 2 if status was 1 and character was [.
 *  - 3 if status was 2 and character was 3.
 *
 *  TODO: make this actually properly interpret ANSI escape codes, as per
 *  http://en.wikipedia.org/wiki/ANSI_escape_code
 */
int interp (char cin, int status)
{
        if (cin == '\n') {
                return -1;
        }
        if (status == 3) {
                if (cin == '~') {
                        buffer_rm(0);
                        return 0;
                } else if (64 <= cin && cin <= 126) {
                        buffer (cin);
                        return 0;
                }
                return 3;
        } else if (status == 2) {
                char *nline = NULL;
                switch (cin) {
                        case 'D':
                        case 'C':
                                buffer_mv (cin);
                                return 0;
                        case 'A':
                                nline = hist_up();
                                rebuffer (nline);
                                free (nline);
                                return 0;
                        case 'B':
                                nline = hist_down();
                                rebuffer (nline);
                                free (nline);
                                return 0;
                        case '3':
                                return 3;
                        default:
                                return 0;
                }
        } else if (status == 1) {
                if (cin == '[') return 2;
                else {
                        buffer (cin);
                        return 0;
                }
        } else {
                switch (cin) {
                        case '':
                                buffer_rm(1);
                                return 0;
                        case '\t':
                                buffer_cpl();
                                return 0;
                        case '':
                                return 1;
                        case '':
                                printf("[%dD", idx);
                                idx = 0;
                                return 0;
                        case '':
                                printf("[%dC", length - idx);
                                idx = length;
                                return 0;
                        default:
                                buffer (cin);
                                return 0;
                }
        }
}

char *line_loop (void)
{
        char cin[2];
        idx = 0;
        length = 0;
        errno = 0;
        buf = calloc (MAX_LINE, sizeof(char));
        if (errno == ENOMEM) {
                print_err ("Line loop calloc failed.");
                return NULL;
        }

        prompt();

        int status = 0;
        while (1) {
                fgets(cin, 2, stdin);
                if (cin == NULL) {
                        print_err_wno ("Error during linebuffer.", errno);
                        continue;
                }
                status = interp(*cin, status);
                if (status == -1) break;
        }
        char *color = get_var("__jpsh_color");
        if (color != NULL) {
                printf("[%dG[K", prompt_length);
                color_line_s(buf);
        } else {
                printf("\n");
        }
        free (color);
        free (sprompt);

        hist_add (buf);
        return buf;
}
