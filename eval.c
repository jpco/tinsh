#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// local includes #include "defs.h" #include "str.h"
#include "env.h"
#include "str.h"
#include "defs.h"
#include "debug.h"

// self-include
#include "eval.h"

/*
 * Parsing:
 * 0) execute subshells
 *    "cat `dir.sh` | ll" => "cat (mydir)/foo | ll"
 * 1) split jobs
 *    "cat (mydir)/foo | ll" => "cat (mydir)/foo" | "ll"
 * 2) expand aliases
 *    "cat (mydir)/foo" | "ll" => "cat (mydir)/foo" | "ls -l"
 * 3) evaluate vars
 *    "cat (mydir)/foo" | "ls -l" => "cat /bin/foo" | "ls -l"
 * 4) pull apart args
 *    "cat /bin/foo" | "ls -l" => "cat" "/bin/foo" | "ls" "-l"
 * 5) execute (w/ piping/redirection)
 *
 * TODO: ADD SUBJOB SUPPORT
 */

char *make_pass (char *cmdline, char start, char end,
                char *(*parse_wd)(char *));

char **split_line (char *cmdline);

// word-parses
char *squotes (char *cmd);
char *subsh (char *cmd);
char *dquotes (char *cmd);
char *parsevar (char *cmd);

// char passes
char *home_pass (char *cmd);

// eval sections
char *eval1 (char *cmdline);
void eval2 (char *cmdline);

void eval (char *cmdline)
{
        // pre-splitting parsing
        cmdline = eval1 (cmdline);
      
        // split!
        char **lines = split_str (cmdline, ';');
        int len;
        for (len = 0; lines[len] != NULL; len++);
        int i;
        for (i = 0; i < len-1; i++) {
                char *line = malloc((strlen(lines[i])+1)*sizeof(char));
                strcpy(line, lines[i]);
                eval2 (line);
        }


        // post-splitting parsing
        eval2 (lines[i]);
}

char *eval1 (char *cmdline)
{
        // single quotes trump all
        cmdline = make_pass (cmdline, '\'', '\'', &squotes);

        // since nested commands often contain special chars...
        cmdline = make_pass (cmdline, '`', '`', &subsh);

        return cmdline;
}

        
void eval2 (char *cmdline)
{
        if (cmdline == NULL) return;

        // parse ~!
        if (get_var ("__jpsh_~home"))
                cmdline = home_pass (cmdline);

        // vars-parsing time!
        cmdline = make_pass (cmdline, '(', ')', &parsevar);

        // single-quotes go last.
        cmdline = make_pass (cmdline, '"', '"', &dquotes);

        char *ncmdline = trim_str(cmdline);
        cmdline = ncmdline;

        // set up exec stuff
        // TODO: background &... also piping and such
        char **argv = split_str (cmdline, ' ');
        int argc;
        for (argc = 0; argv[argc] != NULL; argc++);

        try_exec (argc, argv, 0);
}

// returns new cmdline
char *make_pass (char *cmdline, char start, char end,
                char *(*parse_wd)(char *))
{
        if (cmdline == NULL) return NULL;

        char *word = NULL;
        char *buf = cmdline;
        int len = strlen(cmdline);
        for (; buf <= cmdline+len && *buf != '\0'; buf++) {
                if (*buf != start && *buf != end)  // don't care!
                        continue;

                if (buf > cmdline && *(buf-1) == '\\') {
                        // they escaped! vroom!
                        rm_char (--buf);
                        continue;
                }

                if (*buf == start && !word) {      // start of word
                        *buf = '\0';
                        word = buf+1;
                } else if (*buf == end && word) {  // end of word
                        if (word == buf) {        // trivial word
                                rm_char (buf-1);
                                rm_char (buf-1);
                                word = NULL;
                                len = strlen(cmdline);
                                buf--;
                        } else {                  // the hard part
                                *buf = '\0';
                                char *nword = parse_wd(word);
                                char *ncmdline = NULL;
                                ncmdline = vcombine_str ('\0', 3,
                                                cmdline, nword, buf+1);
                                free (nword);
                                free (cmdline);
                                buf = (word-1-cmdline)+ncmdline;
                                cmdline = ncmdline;
                                len = strlen(cmdline);
                                word = NULL;
                        }
                }
        }

        if (word) {
                print_err ("Malformed input... continuing.");
        }
        return cmdline;
}

// NOTE TO SELF:
// PLEASE DO NOT FREE cmd

char *squotes (char *cmd)
{
        // escape EVERYTHING!
        char *ncmd = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(ncmd, cmd);
        char *buf = ncmd;
        for (; *buf != '\0'; buf++) {
                if (is_separator (*buf)) {
                        char *thrchr = calloc(3, sizeof(char));
                        thrchr[0] = '\\';
                        thrchr[1] = *buf;
                        *buf = '\0';
                        char *nncmd = vcombine_str(0, 3, ncmd, thrchr,
                                        buf+1);
                        free (ncmd);
                        free (thrchr);
                        buf = buf+1-ncmd+nncmd;
                        ncmd = nncmd;
                }
        }

        return ncmd;
}

char *subsh (char *cmd)
{
        // AAA HAHAHAH HHAHHAAAAHHHHH
        char *ncmd = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(ncmd, cmd);
        return ncmd;
}

char *dquotes (char *cmd)
{
        // escape all spaces!
        char *ncmd = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(ncmd, cmd);
        char *buf = ncmd;
        for (; *buf != '\0'; buf++) {
                if (*buf == ' ') {
                        *buf = '\0';
                        char *nncmd = vcombine_str(0, 3, ncmd, "\\ ",
                                        buf+1);
                        free (ncmd);
                        buf = buf+1-ncmd+nncmd;
                        ncmd = nncmd;
                }
        }

        return ncmd;
}

char *parsevar (char *cmd)
{
        // get values! (var, env)
        char *varval = get_var (cmd);
        if (varval == NULL) {
                char *envval = getenv (cmd);
                if (envval == NULL) {
                        return calloc(1, sizeof(char));
                } else {
                        // we have to malloc after getenv
                        char *menvval = malloc((strlen(envval)+1)*sizeof(char));
                        strcpy(menvval, envval);
                        return menvval;
                }
        } else {
                return varval;
        }
}

char *home_pass (char *cmd)
{
        char *home = get_var ("__jpsh_~home");
        char *ncmd = malloc((strlen(cmd)+1)*sizeof(char));
        strcpy(ncmd, cmd);
        cmd = ncmd;
        char *buf = cmd;
        for (; *buf != '\0'; buf++) {
                if (*buf != '~') continue;
                if (buf > cmd && *(buf-1) == '\\') {
                        rm_char (--buf);
                        continue;
                }
                *buf = '\0';
                ncmd = vcombine_str (0, 3, cmd,
                                home,
                                buf+1);
                buf = buf-cmd+ncmd;
                free (cmd);
                cmd = ncmd;
        }

        return cmd;
}

void free_ceval (void)
{
        // I DONUT KNOW
}
