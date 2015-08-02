#ifndef JPSH_EXEC_ENV_H
#define JPSH_EXEC_ENV_H

#include "../types/m_str.h"
#include "../types/var.h"

// Variable utilities. Should be fairly simple
// variables are stored internally as m_str*s

void set_var(const char *key, const char *value);
void set_msvar(const char *key, m_str *value);
void unset_var(const char *key);

var_j *get_var(const char *key);

// Alias utilities. Aliases have no scope!

// environment manipulation. The former uses defaults
// from .impshrc et al, while the second works with an
// arbitrary path.
void init_env();
void init_envp(const char *path);

void free_env();

#endif
