#include <stdlib.h>
#include <stddef.h>

#include "../types/m_str.h"

#include "vector.h"

void rm_element (m_str **argv, size_t idx, size_t *argc)
{
        int i;
        for (i = idx; i < *argc-1; i++) {
               argv[i] = argv[i+1];
        }
        argv[*argc-1] = NULL;

        (*argc)--;
}

void add_element (m_str **argv,
                m_str *na, size_t idx, size_t *argc)
{
        int i;
        for (i = *argc; i > idx; i--) {
                argv[i] = argv[i-1];
        }
        argv[idx] = na;

        (*argc)++;
}

