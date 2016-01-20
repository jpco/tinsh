#ifndef TINSH_SYMTABLE_H
#define TINSH_SYMTABLE_H

#include "types/hashtable.h"

void create_symtable();

typedef enum {
    SYM_BINARY = 8,
    SYM_BUILTIN = 4,
    SYM_FUNCTION = 2,
    SYM_VAR = 1
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
void free_sym(void *voidsym);
sym_t *sym_resolve (const char *key, int ptypes);

int add_sym (char *name, void *val, sym_type type);

#endif
