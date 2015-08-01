#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

// local includes
#include "color.h"
#include "compl.h"
#include "hist.h"
#include "term.h"

#include "../util/defs.h"
#include "../util/str.h"
#include "../util/debug.h"

#include "../exec/env.h"

#include "../types/m_str.h"

// self-include
#include "buffer.h"

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
                sprompt = ms_strip (get_var ("__jpsh_prompt"));
        } else {
                char *co = ms_strip (get_var ("__jpsh_color"));
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

        int width = term_width();
        if (width == 0) width = INT_MAX;
        int nlines = (prompt_length + length - 2) / width;
        int vert = (prompt_length + idx - 1) / width;
        int horiz = (prompt_length + idx - 1) % width;

        // position for print
        printf("[%dA[%dG", vert, prompt_length);
        // print
        printf("%s", buf);
        // clean up and relocate
        if (vert > nlines && horiz == 0) {
                // at end and hit new line
                printf ("\n");
        } else {
                printf ("[K");
                printf("[%dA[%dB[%dG", nlines, vert, horiz + 1);
        }
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

        buf[idx++] = cin;
        length++;

        rewd();
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

void buffer_mv_left (void)
{
        if (idx > 0) idx--;
        reprint();
}
void buffer_mv_right (void)
{
        if (idx < length) idx++;
        reprint();
}

void buffer_cpl (void)
{
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
int escape (char cin);
int ansi (char cin, int status);

int interp (char cin, int status)
{
//        printf ("\n%d, s: %d\n", cin, status);

        if (cin == '\n') {
                return -1;
        }

        if (status != 0) {
                return ansi (cin, status);
        }

        if (cin < 32 || cin == 127) {
                return escape (cin);
        }

        buffer (cin);

        return 0;
}

int escape (char cin)
{
        // ctrl-A = 1
        // ctrl-B = 2
        // ctrl-E = 5
        // ctrl-W = 23

//        printf ("escape.\n");
        if (cin == 1) {  // ctrl-A
                idx = 0;
                reprint();
        } else if (cin == 5) {  // ctrl-E
                idx = length;
                reprint();
        } else if (cin == 2) {  // ctrl-B

        } else if (cin == 23) {  // ctrl-W

        } else if (cin == '\t') {
                buffer_cpl();
        } else if (cin == '') {
                buffer_rm(1);
        } else if (cin == '') {
                return 1;
        }

        return 0;
}

static char ansi_chars[20];

void clear_ansi_chars (void) {
        int i;
        for (i = 0; i < 20; i++) {
                ansi_chars[i] = 0;
        }
}

void append_ansi_char (char c) {
        int i;
        for (i = 0; ansi_chars[i] != 0; i++);
        ansi_chars[i] = c;
}

int ansi (char cin, int status)
{
        int retval = 0;
//        printf ("ANSI. ansi_chars: [%s]\n", ansi_chars);
        // [C, [D:  buffer_mv
        // [A:        nline = hist_up(), rebuffer(nline), free(nline)
        // [B:        nline = hist_down(), rebuffer(nline), free(nline)
        // [3~:       buffer_rm(0)
        //
        if (status == 1) {      // 2-char seq potentially
                if (64 <= cin && cin <= 95 && cin != '[') {
//                        printf ("end-of-ANSI word (1)\n");
                        buffer ('');
                        buffer (cin);
                        retval = 0;
                } else if (cin == '[') {
                        retval = 2;
                } else {
                        buffer (cin);
                        retval = 0;
                }
        } else if (status == 2) {       // CSI has been typed
                append_ansi_char (cin);

                if (64 <= cin && cin <= 126) {  // end of seq, parse!
//                        printf ("end-of-ANSI word (2)\n");
                        if (olstrcmp(ansi_chars, "A")) {
//                                printf ("HIST UP\n");
                                char *nline = hist_up();
                                rebuffer (nline);
                                free (nline);
                        } else if (olstrcmp(ansi_chars, "B")) {
//                                printf ("HIST DOWN\n");
                                char *nline = hist_down();
                                rebuffer (nline);
                                free (nline);
                        } else if (olstrcmp(ansi_chars, "C")) {
                                buffer_mv_right();
                        } else if (olstrcmp(ansi_chars, "D")) {
                                buffer_mv_left();
                        } else if (olstrcmp(ansi_chars, "3~")) {
//                                printf ("DEL\n");
                                buffer_rm(0);
                        } else {
                                sbuffer ("[");
                                sbuffer (ansi_chars);
                        }
                        clear_ansi_chars();
                        retval = 0;
                } else {
                        retval = 2;
                }
        }
        return retval;
}

void free_buf (void)
{
        free (buf);
        buf = NULL;
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
        atexit (free_buf);

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
        char *color = ms_strip (get_var("__jpsh_color"));
        if (color != NULL) {
                printf("[%dG[K", prompt_length);
                color_line_s(buf);
        } else {
                printf("\n");
        }
        free (color);
        free (sprompt);

        hist_add (buf);
        char *rbuf = buf;
        buf = NULL;
        return rbuf;
}
