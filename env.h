#ifndef JPSH_ENV_H
#define JPSH_ENV_H

typedef struct {
        char *key;
        char *value;
} var_t;

/**
 * Creates new scope within the current scope.
 */
void new_scope();
/**
 * Leaves the current scope, reverting to the parent.
 */
void leave_scope();

/**
 * Defines or overwrites the variable; if there is an
 * error, does not affect the array.
 */
void set_var (const char *key, const char *value);
// Undefines the variable.
void unset_var (const char *key);
// Returns 1 if the variable is defined, 0 otherwise.
int has_var (const char *key);
// Returns the value of the variable.
char *get_var (const char *key);
// Lists all of the variables.
void ls_vars ();

// Same as the above functions but with aliases.
void set_alias (const char *key, const char *value);
void unset_alias (const char *key);
int has_alias (const char *key);
char *get_alias (const char *key);
void ls_alias ();

// Frees the environment.
void free_env ();

// Initializes the environment with the default
// configuration paths.
void init_env ();

// Initializes the environment with the passed
// path.
void init_envp (const char *path);

#endif
