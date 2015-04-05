#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// local includes
#include "defs.h"
#include "str.h"

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

int has_value (char *key, var_t **arr, int arr_len)
{
        int i;
        for (i = 0; i < arr_len; i++)
                if (strcmp(key, arr[i]->key) == 0)
                        return 1;

        return 0;
}

char *get_value (char *key, var_t **arr, int arr_len)
{
        char *retvar = NULL;
        int i;
        for (i = 0; i < arr_len; i++) {
                if (strcmp(key, arr[i]->key) == 0) {
                        retvar = malloc (sizeof(char) * strlen(arr[i]->value)+1);
                        strcpy (retvar, arr[i]->value);
                        break;
                }
        }

        return retvar;

}

void set_value (char *key, char *value, var_t **arr, int *arr_len, int max)
{
        char *l_key = malloc ((strlen(key)+1)*sizeof(char));
        char *l_value = malloc ((strlen(value)+1)*sizeof(char));
        strcpy(l_key, key);
        strcpy(l_value, value);

        int i;
        for (i = 0; i < *arr_len; i++) {
                if (strcmp(key, arr[i]->key) == 0) {
                        free (l_key);
                        free (arr[i]->value);
                        arr[i]->value = l_value;
                        return;
                }
        }

        if (*arr_len < max) {
                arr[*arr_len] = malloc(sizeof(var_t));
                arr[*arr_len]->key = l_key;
                arr[(*arr_len)++]->value = l_value;
        }
        // I should probably do something if there are too many elts :)
}

int has_var (char *key)
{
        return has_value (key, vars, vars_len);
}

char *get_var (char *key)
{
        return get_value (key, vars, vars_len);
}

/**
 * Note: Argument-safe!
 * If the variable array already contains key, replaces it.
 * Otherwise, adds the variable's value to the variable array.
 */
void set_var (char *key, char *value)
{
        set_value (key, value, vars, &vars_len, MAX_VARS);
}

void ls_vars (void)
{
        printf ("Vars:\n");
        int i;
        for (i = 0; i < vars_len; i++)
                printf("%s = %s\n", vars[i]->key, vars[i]->value);
}

void set_alias (char *key, char *value)
{
        set_value (key, value, aliases, &alias_len, MAX_ALIASES);
}

int has_alias (char *key)
{
        return has_value (key, aliases, alias_len);
}

char *get_alias (char *key)
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
                        if (strcmp(line, "[env]") == 0) {
                                sect = ENV;
                        } else if (strcmp(line, "[vars]") == 0) {
                                sect = VARS;
                        } else if (strcmp(line, "[alias]") == 0) {
                                sect = ALIAS;
                        } else if (strcmp(line, "[startup]") == 0) {
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
                        // TODO: "debug" var handling
                        eval (line);
                        continue;
                }

                // case: key-value being set.
                char *spline[2];
                spline[0] = line;
                spline[1] = strchr(line, '=');
                if (spline[1] == NULL) {
                        free (line);
                        continue;
                }
                *spline[1] = '\0';
                spline[1]++;

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
                printf("jpsh: Could not open config file: %s\n", strerror(err));
                printf("jpsh: Using default settings...");
                init_env_defaults();
                return;
        }

        init_env_wfp (fp);
}

/**
 * Attempts to open a file descriptor from the given path, and then
 * uses that as the file when calling init_env_wfp().
 */
void init_envp (char *path)
{
        FILE *fp = fopen (path, "r");
        int err = errno;
        if (fp == NULL) {
                printf("jpsh: Could not open config file: %s", strerror(err));
                exit (1);
        }

        init_env_wfp (fp);
}
