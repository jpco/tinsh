#ifndef TINSH_SYMTABLE_H
#define TINSH_SYMTABLE_H

#include "types/hashtable.h"

void create_symtable();

typedef enum {
    SYM_BINARY,
    SYM_FUNCTION,
    SYM_VAR
} sym_type;

struct scope {
    struct scope *parent;

    hashtable *symtable;
};

typedef struct {
    char *value;
    sym_type type;
} sym_t;

struct scope *gscope;
struct scope *cscope;
hashtable *bintable;

void hash_bins();
void rehash_bins();
sym_t *sym_resolve (char *key);

#endif
