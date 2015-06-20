#ifndef JPSH_ENV_H
#define JPSH_ENV_H

// Frees the environment.
void free_env ();

// Initializes the environment with the default
// configuration paths.
void init_env ();

// Initializes the environment with the passed
// path.
void init_envp (const char *path);

#endif
