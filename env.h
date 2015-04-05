#ifndef JPSH_ENV_H
#define JPSH_ENV_H

typedef struct {
        char *key;
        char *value;
} var_t;

// getters & setters
void set_var (char *key, char *value);
int has_var (char *key);
char *get_var (char *key);
void ls_vars ();

void set_alias (char *key, char *value);
int has_alias (char *key);
char *get_alias (char *key);
void ls_alias ();

// environment destruction
void free_env ();

// environment initialization
void init_env ();
void init_envp (char *path);

#endif
