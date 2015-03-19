#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "def.h"
#include "str.h"
#include "env.h"

static var_t *aliases[MAX_ALIASES];
static var_t *vars[MAX_VARS];
static int alias_len;
static int vars_len;

typedef enum {NONE, ENV, VARS, ALIAS, STARTUP} config_section_t;

void default_jpsh_env()
{
        setenv("SHELL", "/home/jpco/bin/jpsh", 1);
        setenv("LS_COLORS", "rs=0:di=01;34:ln=01;36:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:su=37;41:sg=30;43:ca=30;41:tw=30;42:ow=34;42:st=37;44:ex=01;32", 1);
}

char *trim_line (char *line)
{
        char *oline = line;
        while (*line == ' ' || *line == '\t') line++;
        int i;
        if (strlen(line) == 0) return "";
        for (i = strlen(line)-1; line[i] == '\n' || 
                                 line[i] == ' '; i--)
                line[i] = '\0';

        char *nline = malloc ((1 + strlen(line)) * sizeof(char));
        strcpy(nline, line);
        return nline;
}

void ls_alias()
{
        int i;
        printf("Aliases set:\n");
        for (i = 0; i < alias_len; i++) {
                printf("%s=%s\n", aliases[i]->key, aliases[i]->value);
        }
}

void ls_vars()
{
        int i;
        printf("Vars set:\n");
        for (i = 0; i < vars_len; i++) {
                printf("%s=%s\n", vars[i]->key, vars[i]->value);
        }
}

char *arr_out (char *in, var_t **arr, int *len)
{
        int i;
        for (i = 0; i < *len; i++) {
                if(strcmp(arr[i]->key, in) == 0) {
                        return arr[i]->value;
                }
        }
        return NULL;

}

char *unalias (char *in)
{
        return arr_out(in, aliases, &alias_len);
}

char *unenvar (char *in)
{
        return arr_out(in, vars, &vars_len);
}

void arr_in (char *key, char *value, var_t **arr, int *len)
{
        char *l_key = malloc((1 + strlen(key))*sizeof(char));
        char *l_value = malloc((1 + strlen(value))*sizeof(char));
        strcpy(l_value, value);
        strcpy(l_key, key);

        if (*len < MAX_VARS) {
                arr[*len] = malloc(sizeof(var_t));
                arr[*len]->key = l_key;
                arr[*len]->value = l_value;
                (*len)++;
        } else {
                free (l_key);
                free (l_value);
        }
}

void alias (char *key, char *value)
{
        arr_in (key, value, aliases, &alias_len);
}

void envar (char *key, char *value)
{
        arr_in (key, value, vars, &vars_len);
}

int has_elt (char *l_key, var_t **arr, int *len)
{
        int i;
        for (i = 0; i < *len; i++) {
                if (strcmp(arr[i]->key, l_key) == 0) return 1;
        }
        return 0;
}

int has_alias (char *l_key)
{
        return has_elt (l_key, aliases, &alias_len);
}

int has_var (char *l_key)
{
        return has_elt (l_key, vars, &vars_len);
}

void free_env()
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

void jpsh_env()
{
        FILE *fp;
        // TODO: change this
        fp = fopen("/home/jpco/.jpshrc", "r");
        if (fp == NULL) {
                printf("could not find ~/.jpshrc, using defaults...");
                default_jpsh_env();
                return;
        }

        if (debug()) printf("\e[2;35m");
        size_t n = 2000;
        char *rline = malloc((1+n)*sizeof(char));
        char *line;
        config_section_t sect = NONE;
        int read;
        read = getline (&rline, &n, fp);
        for (; read >= 0; read = getline (&rline, &n, fp)) {
//                printf(" > line: %s", rline);
                line = trim_line(rline);
                if (*line == '#' || *line == '\0') {
                        free (line);
                        continue;
                }

                int linelen = strlen(line);
                // section line
                if (*line == '[' && line[linelen-1] == ']') {
                        if (strcmp(line, "[env]") == 0) {
                                if (debug()) printf ("section env\n");
                                sect = ENV;
                        } else if (strcmp(line,"[vars]") == 0) {
                                if (debug()) printf ("section vars\n");
                                sect = VARS;
                        } else if (strcmp(line,"[alias]") == 0) {
                                if (debug()) printf ("section alias\n");
                                sect = ALIAS;
                        } else if (strcmp(line,"[startup]") == 0) {
                                if (debug()) printf ("section startup\n");
                                sect = STARTUP;
                        } else sect = NONE;
                        free (line);
                        continue;
                }

                // if it's not a section line it needs a section!
                if (sect == NONE) {
                        free (line);
                        continue;
                }

                if (sect == STARTUP) {
                        if (debug()) printf("exec '%s'\n", line);
                        free (line);
                        continue;
                }

                // value lines
                char *spline[2];
                spline[0] = line;
                spline[1] = strchr(line, '=');
                if (spline[1] == NULL) {
                        read = getline (&rline, &n, fp);
                        continue;
                }
                *spline[1] = '\0';
                spline[1]++;
                if (debug()) printf("key '%s' and val '%s'\n", spline[0], spline[1]);
                if (sect == ENV) {
                        setenv(spline[0], spline[1], 1);
                } else if(sect == ALIAS) {
                        alias(spline[0], spline[1]);
                } else if(sect == VARS) {
                        envar(spline[0], spline[1]);
                }
                free (line);
        }
        if (debug()) printf("\e[0m");
        free (rline);
        fclose (fp);
}
