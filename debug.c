#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// local includes
#include "env.h"

// self-include
#include "debug.h"

void print_err (const char *errmsg)
{
        fprintf (stderr, "%s\n", errmsg);
}

void dbg_print_err (const char *errmsg)
{
        char *dbg = get_var ("debug");
        if (!dbg) return;
        else free (dbg);
        
        print_err (errmsg);
}

void print_err_wno (const char *errmsg, int err)
{
        fprintf (stderr, "%s\n", errmsg);
        fprintf (stderr, "%d: %s\n", err, strerror(err));
}

void dbg_print_err_wno (const char *errmsg, int err)
{
        char *dbg = get_var ("debug");
        if (!dbg) return;
        else free (dbg);

        print_err_wno (errmsg, err);
}
