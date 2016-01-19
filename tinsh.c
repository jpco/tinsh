#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "posix/putils.h"
#include "posix/ptypes.h"
#include "types/linkedlist.h"
#include "builtins/builtins.h"
#include "inter/hist.h"
#include "inter/buffer.h"
#include "util/defs.h"
#include "inter/term.h"

#include "symtable.h"
#include "eval.h"

typedef enum {
    LINE_NORMAL,    // normal parsing
    LINE_COMMENT,   // we're in a comment; ignore unless it's time to leave the comment
    LINE_CONTINUE,  // newline has been escaped; append current line to buffer line & execute

    LINE_BGATHER,   // need to gather a line/block for buffer line
    LINE_IF_FALSE,  // good to jump into 'else'
    // More states here as syntax gets more 'fancy'
} line_state_t;

line_state_t lstate = LINE_NORMAL;
// static char *linebuffer;

void parse_args (int argc, char **argv)
{
}

int main (int argc, char **argv)
{
    // parse arguments
    parse_args (argc, argv);

    // initialize POSIX sorts of things
    posix_init ();

    // initialize symbol table
    create_symtable ();

    // initialize builtins
    builtins_init ();

    term_init ();
    
    // read config file

    // read input/listen for input
    char inbuf[MAX_LINE];
    while (1) {
        // get input
        memset (inbuf, 0, MAX_LINE);
		get_cmd (inbuf);
        if (!*inbuf) continue;

        char *cmd = strndup (inbuf, MAX_LINE);

        if (!*cmd) continue;

        // TODO: fork behavior here based on line state

        // eval() calls exec()
        int stat = eval (cmd);
        hist_add (inbuf);
        free (cmd);
    }
}
