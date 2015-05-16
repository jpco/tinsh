#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// local includes
#include "defs.h"
#include "str.h"
#include "debug.h"

// self-include
#include "env.h"

// environment variables are stored
// more deeply, using setenv() and getenv() calls
static var_t *aliases[MAX_ALIASES];
static var_t *vars[MAX_VARS];
static int alias_len;
static int vars_len;

// enum to differentiate sections of the config file
typedef enum {NONE, ENV, VARS, ALIAS, STARTUP} config_section_t;

//
// SECTION: GETTERS & SETTERS
//

/**
 * Returns 1 if the passed array contains the passed key, 0 otherwise.
 */
int has_value (const char *key, var_t **arr, int arr_len)
{
        int i;
        for (i = 0; i < arr_len; i++)
                if (olstrcmp (key, arr[i]->key))
                        return 1;

        return 0;
}

/**
 * Returns a pointer to the value in the passed array associated with
 * the passed key, or NULL on error or if the value is not found.
 */
char *get_value (const char *key, var_t **arr, int arr_len)
{
        char *retvar = NULL;
        int i;
        for (i = 0; i < arr_len; i++) {
                if (strcmp(key, arr[i]->key) == 0) {
                        retvar = malloc (sizeof(char) * strlen(arr[i]->value)+1);
                        if (retvar == NULL && errno == ENOMEM) {
                                print_err ("Could not malloc var value.");
                                return NULL;
                        }
                        strcpy (retvar, arr[i]->value);
                        break;
                }
        }

        return retvar;
}

/**
 * Adds the key-value pair to the passed array; if the key is already
 * in the array, discards the old value and associates the key with the
 * new value. If there is an error, does not affect the array.
 */
void set_value (const char *key, const char *value,
                var_t **arr, int *arr_len, int max)
{
        char *l_key = trim_str (key);
        char *l_value;
        if (value) {
                l_value = trim_str (value);
        } else {
                l_value = trim_str ("");
        }

        int i;
        for (i = 0; i < *arr_len; i++) {
                if (olstrcmp(key, arr[i]->key)) {
                        free (l_key);
                        if (arr[i]->value != NULL) free (arr[i]->value);
                        arr[i]->value = l_value;
                        return;
                }
        }

        if (*arr_len < max) {
                arr[*arr_len] = malloc(sizeof(var_t));
                if (arr[*arr_len] == NULL && errno == ENOMEM) {
                        print_err ("Could not malloc new key-value pair.");
                }
                arr[*arr_len]->key = l_key;
                arr[(*arr_len)++]->value = l_value;
        } else {
                dbg_print_err ("Variable array is full.");
        }
}

int has_var (const char *key)
{
        return has_value (key, vars, vars_len);
}

char *get_var (const char *key)
{
        return get_value (key, vars, vars_len);
}

void set_var (const char *key, const char *value)
{
        set_value (key, value, vars, &vars_len, MAX_VARS);
}

/**
 * Removes the key-value pair from the array, or does nothing if
 * the passed key is not in the array.
 */
void unset_value (const char *key, var_t **arr, int *arr_len)
{
        int i;
        for (i = 0; i < *arr_len; i++) {
                if (strcmp (arr[i]->key, key) == 0) {
                        var_t *torm = arr[i];
                        int j;
                        for (j = i; j < *arr_len-1; j++) {
                                arr[j] = arr[j+1];
                        }
                        arr[--(*arr_len)] = NULL;
                        free (torm->key);
                        free (torm->value);
                        free (torm);
                }
        }
}


void unset_var (const char *key)
{
        unset_value (key, vars, &vars_len);
}

void ls_vars (void)
{
        printf ("Vars:\n");
        int i;
        for (i = 0; i < vars_len; i++) {
                if (strcmp(vars[i]->value, "") != 0) {
                        printf("%s = %s\n", vars[i]->key, vars[i]->value);
                } else {
                        printf("%s set\n", vars[i]->key);
                }
        }
}

void set_alias (const char *key, const char *value)
{
        set_value (key, value, aliases, &alias_len, MAX_ALIASES);
}

void unset_alias (const char *key)
{
        unset_value (key, aliases, &alias_len);
}

int has_alias (const char *key)
{
        return has_value (key, aliases, alias_len);
}

char *get_alias (const char *key)
{
        return get_value (key, aliases, alias_len);
}

void ls_alias (void)
{
        printf ("Aliases:\n");
        int i;
        for (i = 0; i < alias_len; i++)
                printf("%s = %s\n", aliases[i]->key, aliases[i]->value);
}

//
// SECTION: ENVIRONMENT DESTRUCTION
//

/**
 * Frees the static heap-allocated storage used to store
 * the environment.
 */
void free_env (void)
{
        int i;
        for (i = 0; i < vars_len; i++) {
                free (vars[i]->key);
                free (vars[i]->value);
                free (vars[i]);
        }
        for (i = 0; i < alias_len; i++) {
                free (aliases[i]->key);
                free (aliases[i]->value);
                free (aliases[i]);
        }
}

//
// SECTION: ENVIRONMENT INITIALIZATION
//

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
                        setenv (spline[0], spline[1], 1);
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
