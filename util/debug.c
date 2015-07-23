#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// self-include
#include "debug.h"

int debug_line_no = 0;

void print_err (const char *errmsg)
{
        if (debug_line_no) fprintf (stderr, "Line %d: ", debug_line_no);
        fprintf (stderr, "%s\n", errmsg);
}

void dbg_print_err (const char *errmsg)
{
        // TODO: get these in gear
/*        char *dbg = get_var ("debug");
        if (!dbg) return;
        else free (dbg); */
        
        print_err (errmsg);
}

void print_err_wno (const char *errmsg, int err)
{
        if (debug_line_no) fprintf (stderr, "Line %d: ", debug_line_no);
        if (errmsg != NULL) fprintf (stderr, "%s\n", errmsg);
        fprintf (stderr, "%d: %s\n", err, strerror(err));
}

void dbg_print_err_wno (const char *errmsg, int err)
{
/*        char *dbg = get_var ("debug");
        if (!dbg) return;
        else free (dbg); */

        print_err_wno (errmsg, err);
}
