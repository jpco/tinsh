

// self-include
#include "symbol.h"

typedef enum { ST_BIN, ST_FN, ST_BLOCK, ST_VAL } sym_type;

struct symbol_str {
    sym_type type;

    char *name;
    void *value;
};
