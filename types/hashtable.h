#ifndef JPSH_HASHTABLE_H
#define JPSH_HASHTABLE_H

#include <stddef.h>

typedef struct hashtable_str hashtable;

hashtable *ht_make();
void ht_destroy(hashtable *ht);

void ht_add (hashtable *ht, const char *key, void *elt);

// nonzero on success
int ht_get (hashtable *ht, const char *key, void **elt);

// nonzero on success
int ht_rm (hashtable *ht, const char *key, void **elt);

size_t ht_size (hashtable *ht);

#endif
