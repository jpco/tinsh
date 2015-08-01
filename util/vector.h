#ifndef JPSH_UTIL_VECTOR_H
#define JPSH_UTIL_VECTOR_H

#include <unistd.h>
#include "../types/m_str.h"

void rm_element(m_str **argv, size_t idx, size_t *argc);
void add_element(m_str **argv, m_str *na, size_t idx, size_t *argc);
void **combine(void **first, void **second, size_t flen, size_t slen);

#endif
