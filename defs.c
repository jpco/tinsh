// self-include
#include "defs.h"

const char separators[NUM_SEPARATORS] = {' ', '|', '&', '>', '<', ';', '\0'};

int is_separator (char c)
{
        int i;
        for (i = 0; i < NUM_SEPARATORS; i++) {
                if (c == separators[i]) return 1;
        }
        return 0;
}
