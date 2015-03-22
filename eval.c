#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "builtin.h"
#include "debug.h"
#include "parseline.h"
#include "def.h"
#include "eval.h"
#include "exec.h"
#include "env.h"
#include "str.h"

char *expand_vars(char *line)
{
        if (*line == '\0') return "";

        char *buf = line;
        int linelen = strlen(line);

        while (buf != NULL && (buf - line) < linelen) {
                char *nbuf = strchr(buf, ' ');
                if (nbuf == NULL) break;
                char *nwd = malloc((1 + nbuf - buf)*sizeof(char));
                *nbuf = '\0';
                strcpy(nwd, buf);
                *nbuf = ' ';
                if (has_alias(nwd)) {
                        char *exline;
                        nbuf = line+insert_str(line, &exline, unalias(nwd),
                                        buf - line, nbuf - line);
                        line = exline;
                }
                free (nwd);
                buf = nbuf+1;
        }

        linelen = strlen(line);
        buf = strchr(line, '(');
        while (buf != NULL && *(buf-1) == '\\') {

                rm_first_char(buf-1);
                linelen--;

                if (buf - line < linelen) {
                        buf = strchr(buf, '(');
                } else buf = NULL;
        }

        while (buf != NULL && (buf - line) < linelen) {
                char *nbuf = strchr(buf, ')');
                if (nbuf == NULL) break;
                while (*(nbuf-1) == '\\') {
                        rm_first_char(nbuf-1);
                        linelen--;

                        if (nbuf - line < linelen) {
                                nbuf = strchr(nbuf, ')');
                        } else nbuf = NULL;
                }

                char *nwd = malloc((nbuf - buf)*sizeof(char));
                *nbuf = '\0';
                strcpy(nwd, buf+1);
                *nbuf = ')';
                char *val = NULL;
                if (has_var(nwd)) {
                        val = unenvar(nwd);
                } else if (has_envvar(nwd)) {
                        val = getenv(nwd);
                }
                if (val) {
                        char *exline;
                        nbuf = line+insert_str(line, &exline, val, buf - line, nbuf - line + 1);
                        line = exline;
                }
                free (nwd);
                buf = nbuf+1;
        }

        return line;
}

void eval (char *cmdline) // cmdline always initialized w calloc()
{
        char *argv[MAX_ARGS];

        // this will probably expand
        if (!starts_with(cmdline, "set ")) {
                cmdline = expand_vars(cmdline);
        }
        int bg = parseline(cmdline, argv);
        pid_t pid;

        if (argv[0] == NULL) return;

        if (debug()) printjob (bg, argv);

        if (!builtin(argv, 0)) {
                pid = fork();
                int success;
                if (pid < 0) {
                        printf("Fork error\n");
                } else if (pid == 0) {
                        success = try_exec(argv, 0);
                }

                if (!bg || !success) {
                        int status;
                        if (waitpid(pid, &status, 0) < 0) {
                                exit(1);
                        }
                }
        }
        return;
}
