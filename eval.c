#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// local includes
#include "env.h"
#include "str.h"
#include "defs.h"
#include "debug.h"
#include "exec.h"

// self-include
#include "eval.h"

static char *cmdline;
static char **lines;
static char **argv;

char *make_pass (char *cmd, char start, char end,
                char *(*parse_wd)(char *));

char **split_line (char *cmd);

// word-parses
char *squotes (char *cmd);
char *subsh (char *cmd);
char *dquotes (char *cmd);
char *parsevar (char *cmd);

// misc passes
char *home_pass (char *cmd);
char *alias_pass (char *cmd);

// eval sections
char *eval1 (char *cmd);
void eval2 (char **cmd);

void eval (char *cmd)
{
        cmdline = cmd;

        // pre-splitting parsing
        cmdline = eval1 (cmdline);
      
        // split!
        lines = split_str (cmd, ';');
        int len;
        for (len = 0; lines[len] != NULL; len++);
        int i;
        for (i = 0; i < len-1; i++) {
                char *line = malloc((strlen(lines[i])+1)*sizeof(char));
                char **lineptr = &line;
                strcpy(line, lines[i]);
                eval2 (lineptr);
                free (line);
        }


        // post-splitting parsing
        eval2 (lines+i);

        free (lines[0]);
        free (lines);
        free (cmdline);
}

char *eval1 (char *cmd)
{
        // single quotes trump all
        cmd = make_pass (cmd, '\'', '\'', &squotes);

        // since nested commands often contain special chars...
        cmd = make_pass (cmd, '`', '`', &subsh);

        return cmd;
}

        
void eval2 (char **cmd)
{
        if (*cmd == NULL) return;

        // try aliases
        *cmd = alias_pass (*cmd);
        
        // parse ~!
        *cmd = home_pass (*cmd);

        // vars-parsing time!
        *cmd = make_pass (*cmd, '(', ')', &parsevar);

        // single-quotes go last.
        *cmd = make_pass (*cmd, '"', '"', &dquotes);

        char *ncmd = trim_str(*cmd);
        free (*cmd);
        *cmd = ncmd;

        // set up exec stuff
        // TODO: background &... also piping and redirection and such
        argv = split_str (*cmd, ' ');
        int argc;
        for (argc = 0; argv[argc] != NULL; argc++);

        try_exec (argc, (const char **)argv, 0);

        free (argv[0]);
        free (argv);
}

// returns new cmd
char *make_pass (char *cmd, char start, char end,
                char *(*parse_wd)(char *))
{
        if (cmd == NULL) return NULL;

        char *word = NULL;
        char *buf = cmd;
        int len = strlen(cmd);
        for (; buf <= cmd+len && *buf != '\0'; buf++) {
                if (*buf != start && *buf != end)  // don't care!
                        continue;

                if (buf > cmd && *(buf-1) == '\\') {
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
                                len = strlen(cmd);
                                buf--;
                        } else {                  // the hard part
                                *buf = '\0';
                                char *nword = parse_wd(word);
                                char *ncmd = NULL;
                                ncmd = vcombine_str ('\0', 3,
                                                cmd, nword, buf+1);
                                free (nword);
                                free (cmd);
                                buf = (word-1-cmd)+ncmd;
                                cmd = ncmd;
                                len = strlen(cmd);
                                word = NULL;
                        }
                }
        }

        if (word) {
                print_err ("Malformed input... continuing.");
        }
        return cmd;
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
        free (cmd);
        cmd = ncmd;

        if (!home) return cmd;

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

        free (home);
        return cmd;
}

char *alias_pass (char *cmd) {
        char *buf = cmd;
        char *alias = NULL;
        for (; *buf != '\0'; buf++) {
                if (is_separator (*buf)) break;
        }

        char delim = *buf;
        *buf = '\0';
        alias = get_alias (cmd);
        *buf = delim;

        char *ncmd = NULL;
        if (alias) {
                ncmd = vcombine_str (0, 2, alias, buf);
        } else {
                ncmd = vcombine_str (0, 1, cmd);
        }
//        free (cmd);
        return ncmd;
}

void free_ceval (void)
{
        free (lines[0]);
        free (lines);
        free (argv[0]);
        free (argv);

        free (cmdline);
}
