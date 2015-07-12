#ifndef JPSH_TYPES_M_STR_H
#define JPSH_TYPES_M_STR_H

#include <stddef.h>

typedef struct {
        char *str;
        char *mask;
        size_t len;
} m_str;

m_str *ms_make (size_t len);
m_str *ms_mask (char *str);

char *ms_strip (m_str *ms);
char *ms_unmask (m_str *ms);

char *ms_strchr (m_str *ms);

int ms_mstrcmp (m_str *first, m_str *second);
int ms_ustrcmp (m_str *first, m_str *second);

size_t ms_len (m_str *ms);
void ms_trim (m_str **ms);

char ms_rmchar (m_str *ms);

#endif
