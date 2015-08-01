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

void **combine (void **first, void **second, size_t flen, size_t slen)
{
        void **to_return = malloc((flen + slen) * sizeof(void *));

        int i;
        for (i = 0; i < flen; i++) {
                to_return[i] = first[i];
        }

        int j;
        for (j = 0; j < slen; j++) {
                to_return[i+j] = second[j];
        }

        return to_return;
}
