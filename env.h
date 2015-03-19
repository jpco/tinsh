#ifndef JPSH_ENV_H
#define JPSH_ENV_H

#define MAX_VARS 1000
#define MAX_ALIASES 100

typedef struct {
        char *key;
        char *value;
} var_t;

void jpsh_env();
void default_jpsh_env();

char *unenvar(char *in);
void envar(char *key, char *value);
int has_var(char *key);
void ls_vars();

char *unalias(char *in);
void alias(char *key, char *value);
int has_alias(char *key);

void free_env();

#endif
