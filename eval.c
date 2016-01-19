#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "util/defs.h"
#include "posix/ptypes.h"

#include "eval.h"
#include "exec.h"

static char tokarr[MAX_LINE];
static char *tokstr = tokarr;

int settoken (char *tkstr)
{
    strncpy (tokarr, tkstr, MAX_LINE);
    tokstr = tokarr;

    return 0;
}

/*

cmdline: ls (HOMEDIR)/configs | uniq -c 'config file (new).cfg' -2> uniq_errs.log & # neato
token calls:
1: return 'w', fill tokbuf with "ls"
2: return '(', fill tokbuf with "(HOMEDIR)/configs"
3: return '|', do not fill tokbuf
4: return 'w', fill tokbuf with "uniq"
5: return 'w', fill tokbuf with "-c"
6: return 'w', fill tokbuf with "'config file (new).cfg'"
7: return '>', fill tokbuf with "-2>"
8: return 'w', fill tokbuf with "uniq_errs.log"
9: return '&', do not fill tokbuf
0: return '#', do not fill tokbuf

 */

#define WTOKEN "w>("            // word token
#define STOKEN "|#:;&"          // single-character token
#define WHITESPACE " \t\r\n"

// TODO: add { and }, enable escaping newlines
// TODO: enable escaping things with '\'
char gettoken (char *tokbuf)
{
    // word, |, -2>1, (var (resolution)), 'quotes (single)', "double quotes", #, :, {, }
    // some tokens are symbols: |#:{};&
    // some tokens fill a buffer: word, -2>1, (var (resolution)), 'quotes', "quotes"
    // whitespace is ' \t\r\n'

    // eat initial whitespace
    while (*tokstr && strchr (WHITESPACE, *tokstr)) {
        tokstr++;
    }

    // indicate end of string
    if (!*tokstr) return 0;

    // if we have a single-char token it's easy--advance tokstr and return the token
    if (strchr (STOKEN, *tokstr)) {
        char rv = *tokstr;
        tokstr++;
        return rv;
    }

    // if we're here we have a 'word-style' token
    // need to extract word

    char type = 'w';
    char q = 0;
    int vdepth = 0;
    char *tkbuf = tokbuf;

    char cchar;
    while ((cchar = *tokstr++)) {
        if (cchar == '(' && q != '\'') {
            vdepth++;
            type = '(';
        } else if (cchar == ')') {
            vdepth > 0 ? vdepth-- : vdepth;
        } else if (cchar == '"' || cchar == '\'') {
            if (!q) q = cchar;
            else if (cchar == q) q = 0;
        } else if (!q && !vdepth && strchr (STOKEN WHITESPACE, cchar)) {
            // end of word!
            goto end;
        }
        *tkbuf++ = cchar;
    }

end:
    tokstr--;
    *tkbuf = 0;
    if (*tokbuf == '-') {
        tkbuf--;
        if (*tkbuf == '>' || *(tkbuf - 1) == '>') {
            type = '>';
        }
    }
    return type;
}

job *make_job (void)
{
    job *cjob = calloc (sizeof (job), 1);
    cjob->stdin = STDIN_FILENO;
    cjob->stdout = STDOUT_FILENO;
    cjob->stderr = STDERR_FILENO;
    cjob->first = calloc (sizeof (process), 1);

    return cjob;
}

int eval (char *cmdline)
{
    // create first job/process
    job *cjob = make_job ();
    cjob->command = strdup (cmdline);
    process *cproc = cjob->first;
    cproc->argv = calloc (MAX_LINE, sizeof (char *));
    char **carg = cproc->argv;

    // prepare to tokenize
    settoken (cmdline);

    // tokenize!
    int ttype;
    char tokbuf[MAX_LINE];
    int jfg = 1;
    // tokens: 
    // #define WTOKEN "w>("            // word token
    // #define STOKEN "|#:;&"          // single-character token
    while ((ttype = gettoken (tokbuf))) {
        switch (ttype) {
            case '|':
                cproc->next = calloc (sizeof (process), 1);
                cproc = cproc->next;
                cproc->argv = calloc (MAX_LINE, sizeof (char *));
                carg = cproc->argv;
                break;
            case '#':
                goto brk;
            case ':':
                break;
            case ';':
                exec (cjob, jfg);
                jfg = 1;
                cjob = make_job ();
                cjob->command = strdup (cmdline);
                cproc = cjob->first;
                cproc->argv = calloc (MAX_LINE, sizeof (char *));
                carg = cproc->argv;
                break;
            case '&':
                jfg = 0;
                break;
            case '(':
                // devar (tokbuf);
            case 'w':
                // dequote (tokbuf);
                *carg++ = strdup (tokbuf);
                break;
            case '>':
                break;
        }
    }

brk:
    exec (cjob, jfg);

    return 0;
}
