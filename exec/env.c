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

#include "exec.h"

// self-include
#include "env.h"

scope_j *gscope;
scope_j *cscope;  // defines full scope stack within scope type as well

void set_var (const char *key, const char *value)
{
        m_str *mval = ms_mask (value);
        set_msvar (key, mval);
}

void set_msvar (const char *key, m_str *value)
{
        var_j *nvar = malloc(sizeof(var_j));
        nvar->is_fn = 0;
        nvar->v.value = value;
        ht_add (cscope->vars, key, nvar);
}

void unset_var (const char *key)
{
        var_j *trash = NULL;
        scope_j *csc = cscope;
        while (csc != NULL) {
                if (ht_rm (cscope->vars, key, (void **)&trash)) {
                        break;
                }
                csc = csc->parent;
        }
        if (trash->is_fn) {
                // TODO: this
        } else {
                ms_free (trash->v.value);
                free (trash);
        }
}

var_j *get_var (const char *key)
{
        var_j *retval = NULL;
        scope_j *csc = cscope;
        while (csc != NULL) {
                if (ht_get (csc->vars, key, (void **)&retval)) {
                        break;
                }
                csc = csc->parent;
        }
        return retval;
}

// === === === === === === ===
// ENVIRONMENT INITIALIZATION
// === === === === === === ===

// enum to differentiate sections of the config file
typedef enum {NONE, ENV, VARS, STARTUP} config_section_t;

/*
 * Sets the environment to the defaults, which is pretty much "nothing".
 */
void init_env_defaults (void)
{
        set_var ("__imp_~home", "(HOME)");
        setenv ("SHELL", "/home/jpco/bin/impsh", 1);
        setenv ("LS_COLORS", "rs=0:di=01;34:ln=01;36:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:su=37;41:sg=30;43:ca=30;41:tw=30;42:ow=34;42:st=37;44:ex=01;32", 1);
}

void init_env_vars (void)
{
        gscope = new_scope(NULL);
        cscope = gscope;
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
 *    [env], [vars], and [startup].
 *    - [env] defines environment variables, which affect programs
 *      executed by the shell.
 *    - [vars] defines regular variables, which are used within the
 *      shell instance only.
 *    - [startup] is a line-delimited list of commands to be run on
 *      startup
 *
 *  - the [env] and [vars] sections are "key=value" formatted,
 *    while the [startup] section is any valid impsh command.
 *    - everything between opening whitespace and the first = is a key.
 *      everything after the first = is a value. No quotes necessary.
 *
 *  - all leading and trailing whitespace is ignored, but no other.
 */
void init_env_wfp (FILE *fp)
{
        init_env_vars();

        size_t n = MAX_LINE;
        char *rline = malloc((1+n)*sizeof(char));
        if (rline == NULL && errno == ENOMEM) {
                print_err ("Could not allocate memory for environment parsing.");
                return;
        }
        int read;
        queue *envq = q_make();
        read = getline (&rline, &n, fp);
        for (; read >= 0; read = getline (&rline, &n, fp)) {
                char *line = strdup (rline);
                q_push (envq, line);
        }

        job_queue *jq = eval (envq);
        exec (jq);

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
                char *epath = vcombine_str('\0', 2, home, "/.impshrc");
                fp = fopen (epath, "r");
                err = errno;
                free (epath);
        }

        if (fp == NULL) {
                fp = fopen ("/etc/impsh.rc", "r");
                err = errno;
        }

        if (fp == NULL) {
                print_err_wno ("Could not open config file.", err);
                print_err ("Using default settings...");
                init_env_vars();
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
}
