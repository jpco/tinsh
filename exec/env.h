#ifndef JPSH_EXEC_ENV_H
#define JPSH_EXEC_ENV_H

#include "../types/m_str.h"

// Variable utilities. Should be fairly simple
// variables are stored internally as m_str*s
int has_var(const char *key);
void ls_vars();

void set_var(const char *key, const char *value);
void set_msvar(const char *key, m_str *value);
void unset_var(const char *key);

m_str *get_var(const char *key);

// Alias utilities. Aliases have no scope!
int has_alias(const char *key);
void ls_alias();

void set_alias(const char *key, const char *value);
void set_msalias(const char *key, m_str *value);
void unset_alias(const char *key);

m_str *get_alias(const char *key);

// environment manipulation. The former uses defaults
// from .jpshrc et al, while the second works with an
// arbitrary path.
void init_env();
void init_envp(const char *path);

void free_env();

#endif
