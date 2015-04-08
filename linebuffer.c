#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// local includes
#include "defs.h"
#include "str.h"
#include "env.h"
#include "hist.h"
#include "term.h"

// self-include
#include "linebuffer.h"

static int idx;
static int length;
static int prompt_length;
static char *buf;
static char *sprompt;

/**
 * Decides the prompt & prints it
 */
void prompt (void)
{
        if (has_var ("prompt")) {
                // TODO: parse prompt
                sprompt = get_var ("prompt");
        } else {
                sprompt = vcombine_str('\0', 3, "\e[1m..",
                                strrchr (getenv ("PWD"), '/'), "$\e[0m ");
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

void buffer (char cin)
{
        if (idx < length) {
                int i;
                for (i = length; i > idx; i--)
                        buf[i] = buf[i-1];
        }

        buf[idx++] = cin;
        length++;
        reprint();
}

void rebuffer (char *sin)
{
        memset (buf, 0, MAX_LINE * sizeof(char));
        if (*sin == '\0') {
                length = 0;
        } else {
                strcpy (buf, sin);
                length = strlen(sin);
        }

        idx = length;
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
                printf("\n");
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
                                sbuffer("TAB");
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

/**
 * NOTE:
 *  - returned char* is allocated in the heap!
 *
 * The main line-buffer loop. This is what is in charge in between each
 * line eval.
 */
char *line_loop (void)
{
        char cin[2];
        idx = 0;
        length = 0;
        buf = calloc (MAX_LINE, sizeof(char));

        prompt();

        int status = 0;
        while (1) {
                fgets(cin, 2, stdin);
                status = interp(*cin, status);
                if (status == -1) break;
        }

        free (sprompt);

        hist_add (buf);
        return buf;
}
