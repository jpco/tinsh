#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "../posix/putils.h"
#include "../posix/ptypes.h"
#include "../types/linkedlist.h"
#include "../ll_utils.h"
#include "../util/defs.h"

#include "hist.h"
#include "term.h"
#include "compl.h"

#include "buffer.h"

static int idx;
static int length;
static int prompt_length;

static char buf[MAX_LINE];
static char *wds[MAX_LINE];
static int nwds;
// static char *sprompt;

void prompt (void)
{
    char *cwd = getcwd(NULL, 0);
    printf ("%s$ ", cwd);
    free (cwd);
    fflush (stdout);

    int row;
    if (cursor_pos (&row, &prompt_length)) prompt_length = 25;
}

static int last_vert = 0;
static int last_nlines = 0;

void reprint (void)
{
    int width = term_width ();
    if (!width) width = INT_MAX;
    int nlines = (prompt_length + length - 2) / width;
    int vert = (prompt_length + idx - 1) / width;
    int horiz = (prompt_length + idx - 1) % width;

    if (last_vert) printf ("[%dA", last_vert);
    printf ("[%dG[K[0G", prompt_length);
    int i;
    for (i = 0; i < last_nlines; i++) {
    printf ("[B[K");
    }

    if (last_nlines) printf ("[%dA", last_nlines);

    // position for print
    printf("[%dG", prompt_length); 
    // print
    printf("%s", buf);
    // clean up and relocate
    if (vert > nlines && horiz == 0) {
        // at end and hit new line
        printf ("\n");
    } else {
        if (nlines) printf ("[%dA", nlines);
        if (vert) printf ("[%dB", vert);
        printf("[%dG", horiz + 1);
    }

    last_vert = vert;
    last_nlines = nlines;

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
    int r = l_compl (buf, cplst, cplend);
    if (!r) rebuffer (cpl_line);
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

//    printf ("escape.\n");
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

void clear_ansi_chars (void)
{
    int i;
    for (i = 0; i < 20; i++) {
        ansi_chars[i] = 0;
    }
}

void append_ansi_char (char c)
{
    int i;
    for (i = 0; ansi_chars[i] != 0; i++);
    ansi_chars[i] = c;
}

int ansi (char cin, int status)
{
    int retval = 0;
//    printf ("ANSI. ansi_chars: [%s]\n", ansi_chars);
    // [C, [D:  buffer_mv
    // [A:    nline = hist_up(), rebuffer(nline), free(nline)
    // [B:    nline = hist_down(), rebuffer(nline), free(nline)
    // [3~:       buffer_rm(0)
    //
    if (status == 1) {      // 2-char seq potentially
        if (64 <= cin && cin <= 95 && cin != '[') {
//            printf ("end-of-ANSI word (1)\n");
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
//            printf ("end-of-ANSI word (2)\n");
            if (strcmp(ansi_chars, "A") == 0) {
//                printf ("HIST UP\n");
                char nline[MAX_LINE];
                hist_up(nline);
                rebuffer (nline);
            } else if (strcmp(ansi_chars, "B") == 0) {
//                printf ("HIST DOWN\n");
                char nline[MAX_LINE];
                hist_down(nline);
                rebuffer (nline);
            } else if (strcmp(ansi_chars, "C") == 0) {
                buffer_mv_right();
            } else if (strcmp(ansi_chars, "D") == 0) {
                buffer_mv_left();
            } else if (strcmp(ansi_chars, "3~") == 0) {
//                printf ("DEL\n");
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


void get_cmd (char *cmdbuf)
{
    char cin[5];
    idx = 0;
    length = 0;

    term_unprep ();
    prompt ();

    int status = 0;
    while (1) {
        *cin = 0;
        fgets (cin, 2, stdin);
        if (!*cin) continue;
        status = interp (*cin, status);
        if (status == -1) break;
    }

    term_prep ();
    printf ("\n");

    hist_add (buf);
    strncpy (cmdbuf, buf, MAX_LINE - 1);
    memset (buf, 0, MAX_LINE);
}
