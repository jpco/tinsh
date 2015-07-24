#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

// local includes
#include "../util/defs.h"
#include "../util/debug.h"
#include "../util/str.h"

#include "../types/var.h"
#include "../types/m_str.h"
#include "../types/scope.h"
#include "../types/queue.h"

#include "../eval/eval.h"

// self-include
#include "env.h"

hashtable *alias_tab;
scope_j *gscope;
scope_j *cscope;  // defines full scope stack within scope type as well

int has_var(const char *key)
{
        m_str *trash;
        scope_j *csc = cscope;
        while (csc != NULL) {
                if (ht_get (csc->vars, key, (void **)&trash)) return 1;
                csc = csc->parent;
        }
        return 0;
}

// TODO: this
void ls_vars (void)
{
        return;
}

void set_var (const char *key, const char *value)
{
        m_str *mval = ms_mask (value);
        set_msvar (key, mval);
}

void set_msvar (const char *key, m_str *value)
{
        ht_add (cscope->vars, key, value);
}

void unset_var (const char *key)
{
        m_str *trash;
        scope_j *csc = cscope;
        while (csc != NULL) {
                if (ht_rm (cscope->vars, key, (void **)&trash)) {
                        break;
                }
        }
        ms_free (trash);
}

m_str *get_var (const char *key)
{
        m_str *retval = NULL;
        scope_j *csc = cscope;
        while (csc != NULL) {
                if (ht_rm (cscope->vars, key, (void **)&retval)) {
                        break;
                }
        }
        return retval;
}

int has_alias (const char *key)
{
        m_str *trash;
        return ht_get (alias_tab, key, (void **)&trash);
}

// TODO: this
void ls_alias (void)
{
        return;
}

void set_alias (const char *key, const char *value)
{
        m_str *mval = ms_mask (value);
        set_msalias (key, mval);
}

void set_msalias (const char *key, m_str *value)
{
        ht_add (alias_tab, key, value);
}

void unset_alias (const char *key)
{
        m_str *trash;
        ht_rm (alias_tab, key, (void **)&trash);
        ms_free (trash);
}

m_str *get_alias (const char *key)
{
        m_str *retval;
        ht_rm (alias_tab, key, (void **)&retval);
        return retval;
}

// === === === === === === ===
// ENVIRONMENT INITIALIZATION
// === === === === === === ===

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
        gscope = new_scope(NULL);
        cscope = gscope;

        size_t n = MAX_LINE;
        char *rline = malloc((1+n)*sizeof(char));
        if (rline == NULL && errno == ENOMEM) {
                print_err ("Could not allocate memory for environment parsing.");
                return;
        }
        char *line;
        config_section_t sect = NONE;
        int read;
        queue *envq = q_make();
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
                        q_push (envq, line);
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

        m_str *cdbg = get_var ("__jpsh_debug");
        unset_var ("debug");

        eval (envq);

        if (cdbg != NULL) {
                set_msvar ("__jpsh_debug", cdbg);
                free (cdbg);
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

void free_env (void)
{
        while (cscope != NULL) {
                cscope = leave_scope (cscope);
        }

        ht_destroy (alias_tab);
}
