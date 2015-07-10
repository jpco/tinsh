#ifndef JPSH_HASHTABLE_H
#define JPSH_HASHTABLE_H

#include <stddef.h>

typedef struct hashtable_str hashtable;

hashtable *ht_make();
void ht_destroy(hashtable *ht);

int ht_add (hashtable *ht, char *key, void *elt);

int ht_get (hashtable *ht, char *key, void **elt);

int ht_rm (hashtable *ht, char *key, void **elt);

size_t ht_size (hashtable *ht);

#endif
