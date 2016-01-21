#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "util/defs.h"

#include "posix/ptypes.h"
#include "posix/putils.h"

#include "eval.h"
#include "exec.h"

job *n_eval (char *cmdline, int job_stdout, int force_bg);

struct tokendat {
    char tokarr[MAX_LINE];
    char *tokstr;
};

static struct tokendat gtkd;

int settoken (char *tkstr, struct tokendat *tkd) {
    if (!tkd) tkd = &gtkd;
    strncpy (tkd->tokarr, tkstr, MAX_LINE);
    tkd->tokstr = tkd->tokarr;

    return 0;
}

int instoken (char *tkins, struct tokendat *tkd)
{
    if (!tkd) tkd = &gtkd;
    int len = strlen (tkins);
    char *tmpstr = strdup (tkd->tokstr);
    strcpy (tkd->tokstr, tkins);
    strcpy (tkd->tokstr + len, tmpstr);
    free (tmpstr);

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
char gettoken (char *tokbuf, struct tokendat *tkd)
{
    if (!tkd) tkd = &gtkd;
    // word, |, -2>1, (var (resolution)), 'quotes (single)', "double quotes", #, :, {, }
    // some tokens are symbols: |#:{};&
    // some tokens fill a buffer: word, -2>1, (var (resolution)), 'quotes', "quotes"
    // whitespace is ' \t\r\n'

    // eat initial whitespace
    while (*(tkd->tokstr) && strchr (WHITESPACE, *(tkd->tokstr))) {
        tkd->tokstr++;
    }

    // indicate end of string
    if (!*(tkd->tokstr)) return 0;

    // if we have a single-char token it's easy--advance tokstr and return the token
    if (strchr (STOKEN, *(tkd->tokstr))) {
        char rv = *(tkd->tokstr);
        tkd->tokstr++;
        return rv;
    }

    // if we're here we have a 'word-style' token
    // need to extract word

    char type = 'w';
    char q = 0;
    int vdepth = 0;
    char *tkbuf = tokbuf;

    char cchar;
    while ((cchar = *(tkd->tokstr++))) {
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
// (-|~)(fd|&)?>(fd|+)?, or <<?(-|~)
    tkd->tokstr--;
    *tkbuf = 0;
    if (*tokbuf == '<') {
        char *b = tokbuf + 1;
        if (*b == '<') b++;
        if ((*b == '-' || *b == '~') && b + 1 == tkbuf) {
            type = '>';
        }
    } else if (*tokbuf == '-' || *tokbuf == '~') {
        char *b = tokbuf + 1;
        if (*b == '&') b++;
        else while (*b >= '0' && *b <= '9') b++;
        if (*b != '>') return type;
        b++;
        if (*b == '+') b++;
        else while (*b >= '0' && *b <= '9') b++;
        if (b == tkbuf) type = '>';
    }
    return type;
}

int ntoken (struct tokendat *tkd)
{
    int i = 0;
    char dumbuf[MAX_LINE];
    while (gettoken (dumbuf, tkd)) i++;

    return i;
}

job *make_job (int stdo)
{
    job *cjob = calloc (sizeof (job), 1);
    cjob->stdin = STDIN_FILENO;
    cjob->stdout = stdo;
    cjob->stderr = STDERR_FILENO;
    cjob->first = calloc (sizeof (process), 1);

    return cjob;
}

// resolve variable && move resolved result out
// returns new position for output buf
char *_devar (const char *input, char *output)
{
    sym_t *res = sym_resolve (input, SYM_VAR);
    if (res) {
        char *val = res->value;
        for (; *val; *output++ = *val++);
    } else {
        char *envval;
        if ((envval = getenv (input))) {
            for (; *envval; *output++ = *envval++);
        }
    }
    *output = 0;
    return output;
}

void devar_np (char *token)
{
    char *tokdup = strdup (token);
    _devar (tokdup, token);
    if (!*token) strcpy (token, tokdup);
    free (tokdup);
}

char *devar_tok (char *input, char *output)
{
    struct tokendat tkd;
    settoken (input, &tkd);
    if (ntoken (&tkd) > 1) {
        // need to sub-eval
        int fds[2];
        pipe (fds);
        n_eval (input, fds[1], 0);
        close (fds[1]);
        int nread;
        int ops = 128;
        char *sshout = calloc (ops, 1);
        char *outbuf = sshout;
        char readbuf[64];
        while ((nread = read (fds[0], readbuf, 64)) > 0) {
            if (outbuf + nread > sshout + ops) {
                ops *= 2;
                output = realloc (sshout, ops);
            }
            strncpy (outbuf, readbuf, nread);
            outbuf += nread;
            update_status();
        }
        if (nread < 0) {
            perror ("devar_tok read");
        }
        close (fds[0]);
        close (fds[1]);
        int len = strlen (sshout);
        strncpy (output, sshout, len);
        free (sshout);
        return output + len;
    } else {
        return _devar (input, output);
    }
}


// parse token and resolve all variables present
void devar (char *token)
{
    char res[MAX_LINE];
    char *resi = res;
    char wvar[MAX_LINE];
    char *wvari = wvar;

    char q = 0;

    int vdepth = 0;
    char *tokbuf;
    // _dv is to mark when to evaluate a var
    // _fc is to mark to skip the opening paren when loading the var buf
    int _dv = 0, _fc = 0;
    for (tokbuf = token; *tokbuf; tokbuf++) {
        char cchr = *tokbuf;

        // quoting
        if (q && cchr == q) q = 0;
        else if (!q && (cchr == '"' || cchr == '\'')) q = cchr;

        // parentheses
        if (q != '\'' && cchr == '(') {
            vdepth++;
            if (vdepth == 1) _fc = 1;
        } else if (q != '\'' && cchr == ')' && vdepth > 0) {
            vdepth--;
            if (!vdepth) _dv = 1;
        }

        if (vdepth) {     // still building var
            if (_fc) _fc = 0;
            else *wvari++ = cchr;
        } else if (_dv) { // need to resolve var
            *wvari = 0;
            resi = devar_tok (wvar, resi);
            wvari = wvar;
            _dv = 0;
        } else {          // just adding misc chars
            *resi++ = cchr;
        }
    }

    *resi = 0;
    strncpy (token, res, MAX_LINE);
}

int dequote (char *tok)
{
    char q = 0;
    char *dest = tok;
    char *curr = tok;
    for (; *curr; curr++) {
        char cchr = *curr;
        if (!q && (cchr == '\'' || cchr == '"')) {
            q = cchr;
        } else if (q && q == cchr) {
            q = 0;
        } else {
            *dest++ = cchr;
        }
    }
    *dest = 0;

    return 0;
}

job *n_eval (char *cmdline, int job_stdout, int force_bg)
{
    // create first job/process
    job *cjob = make_job (job_stdout);
    cjob->command = strdup (cmdline);
    process *cproc = cjob->first;
    cproc->argv = calloc (MAX_LINE, sizeof (char *));
    char **carg = cproc->argv;

    // prepare to tokenize
    settoken (cmdline, NULL);

    // tokenize!
    int ttype;
    char tokbuf[MAX_LINE];
    int jfg = !force_bg, ftok = 1;
    while ((ttype = gettoken (tokbuf, NULL))) {
        switch (ttype) {
            case '|':
                cproc->next = calloc (sizeof (process), 1);
                cproc = cproc->next;
                cproc->argv = calloc (MAX_LINE, sizeof (char *));
                carg = cproc->argv;
                ftok = 1;
                break;
            case '#':
                goto brk;
            case ':':
                break;
            case ';':
                exec (cjob, jfg);
                cjob = make_job (job_stdout);
                cjob->command = strdup (cmdline);
                cproc = cjob->first;
                cproc->argv = calloc (MAX_LINE, sizeof (char *));
                carg = cproc->argv;
                jfg = !force_bg;
                ftok = 1;
                break;
            case '&':
                jfg = 0;
                break;
            case '(':
                ftok = 0;
                devar (tokbuf);
            case 'w':
                if (ftok) {
                    devar_np (tokbuf);
                    instoken (tokbuf, NULL);
                    ftok = 0;
                } else {
                    dequote (tokbuf);
                    *carg++ = strdup (tokbuf);
                }
                break;
            case '>':
                break;
        }
    }

brk: ;
    exec (cjob, jfg);

    return cjob;
}

int eval (char *cmdline)
{
    n_eval (cmdline, STDOUT_FILENO, 0);
    return 0;
}
