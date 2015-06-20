#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "defs.h"
#include "var.h"
#include "debug.h"
#include "str.h"
#include "eval.h" 
// enum to differentiate sections of the config file
typedef enum {NONE, ENV, VARS, ALIAS, STARTUP} config_section_t;

/*
 * Sets the environment to the defaults, which is pretty much "nothing".
 */
void init_env_defaults (void)
{
        setenv ("SHELL", "/home/jpco/bin/jpsh", 1);
        setenv ("LS_COLORS", "rs=0:di=01;34:ln=01;36:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:su=37;41:sg=30;43:ca=30;41:tw=30;42:ow=34;42:st=37;44:ex=01;32", 1);
}

/**
 * Given the passed config file descriptor, parses the file and sets the
 * shell environment.
 *
 * Config files have a very basic syntax, based on INI syntax:
 *
 *  - lines starting with ; or # are comments.
 *
 *  - sections are defined by [<word>], lower-case. sections are
 *    [env], [vars], [alias], and [startup].
 *    - [env] defines environment variables, which affect programs
 *      executed by the shell.
 *    - [vars] defines regular variables, which are used within the
 *      shell instance only.
 *    - [alias] defines aliases, which are essentially variables which
 *      can be dereferenced without special characters used for running
 *      programs
 *    - [startup] is a line-delimited list of commands to be run on
 *      startup
 *
 *  - the [env], [vars], and [alias] sections are "key=value" formatted,
 *    while the [startup] section is any valid jpsh command.
 *    - everything between opening whitespace and the first = is a key.
 *      everything after the first = is a value. No quotes necessary.
 *
 *  - all leading and trailing whitespace is ignored, but no other.
 */
void init_env_wfp (FILE *fp)
{
        new_scope();

        size_t n = MAX_LINE;
        char *rline = malloc((1+n)*sizeof(char));
        if (rline == NULL && errno == ENOMEM) {
                print_err ("Could not allocate memory for environment parsing.");
                return;
        }
        char *line;
        config_section_t sect = NONE;
        int read;
        read = getline (&rline, &n, fp);
        for (; read >= 0; read = getline (&rline, &n, fp)) {
                line = trim_str (rline);

                // case: comment or blank line
                if (*line == ';' || *line == '#' || *line == '\0') {
                        free (line);
                        continue;
                }

                int linelen = strlen(line);

                // case: section line
                if (*line == '[' && line[linelen-1] == ']') {
                        if (olstrcmp(line, "[env]")) {
                                sect = ENV;
                        } else if (olstrcmp(line, "[vars]")) {
                                sect = VARS;
                        } else if (olstrcmp(line, "[alias]")) {
                                sect = ALIAS;
                        } else if (olstrcmp(line, "[startup]")) {
                                sect = STARTUP;
                        } else sect = NONE;
                        free (line);
                        continue;
                }

                // if there is no section, nothing else can happen!
                if (sect == NONE) {
                        free (line);
                        continue;
                }

                // case: startup line
                if (sect == STARTUP) {
                        line[linelen] = '\0';
                        char *cdbg = get_var ("__jpsh_debug");
                        unset_var ("debug");
                        eval (line);
                        if (cdbg != NULL) {
                                set_var ("__jpsh_debug", cdbg);
                                free (cdbg);
                        }
                        continue;
                }

                // case: key-value being set.
                char *spline[2];
                spline[0] = line;
                spline[1] = strchr(line, '=');
                if (spline[1]) {
                        *spline[1] = '\0';
                        spline[1]++;
                }

                if (sect == ENV) {
                        if (!spline[1]) {
                                free (line);
                                continue;
                        }
                        char *tr_spline0 = trim_str(spline[0]);
                        char *tr_spline1 = trim_str(spline[1]);
                        setenv (tr_spline0, tr_spline1, 1);
                        free (tr_spline0);
                        free (tr_spline1);
                } else if (sect == ALIAS) {
                        set_alias (spline[0], spline[1]);
                } else if (sect == VARS) {
                        set_var (spline[0], spline[1]);
                }
                free (line);
        }

        free (rline);
        fclose (fp);
}

/**
 * Determines the appropriate config file to use (when "-c" argument
 * is not set). Then calls init_env_wfp().
 */
void init_env (void)
{
        FILE *fp = NULL;
        int err = -1;
        char *home = getenv("HOME");
        if (home != NULL) {
                char *epath = vcombine_str('\0', 2, home, "/.jpshrc");
                fp = fopen (epath, "r");
                err = errno;
                free (epath);
        }

        if (fp == NULL) {
                fp = fopen ("/etc/jpsh.rc", "r");
                err = errno;
        }

        if (fp == NULL) {
                print_err_wno ("Could not open config file.", err);
                printf("Using default settings...");
                init_env_defaults();
                return;
        }

        init_env_wfp (fp);
}

/**
 * Attempts to open a file descriptor from the given path, and then
 * uses that as the file when calling init_env_wfp().
 */
void init_envp (const char *path)
{
        FILE *fp = fopen (path, "r");
        int err = errno;
        if (fp == NULL) {
                print_err_wno ("Could not open config file.", err);
                exit(1);
        }

        init_env_wfp (fp);
}
