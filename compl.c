#include <stdlib.h>
#include <string.h>

// local includes
#include "debug.h"
#include "str.h"

// self-include
#include "compl.h"

char *l_compl (char *line, int idx)
{
        if (idx > strlen(line))
                return line;

        char idx_c = line[idx];        
}
