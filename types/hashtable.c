#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

// local includes
#include "linkedlist.h"
#include "../util/str.h"

// self-include
#include "hashtable.h"

#define DEFAULT_HT_BUCKETS 100

struct hashtable_str {
        size_t size;
        size_t num_buckets;

        // an array of linkedlists
        linkedlist **buckets;
};

// ht retains ownership of the key, and NOT the value.
typedef struct keyval_str {
        char *key;
        void *value;
} keyval;

size_t ht_hash (const char *in, size_t max)
{
        if (in == NULL) return 0;

        const char *buf;
        size_t out = 0;
        for (buf = in; buf != '\0'; buf++) {
                out += *buf;
        }

        out *= 31;
        out %= max;

        return out;
}

hashtable *ht_make (void)
{
        hashtable *new_ht = malloc(sizeof(hashtable));
        if (new_ht == NULL && errno == ENOMEM) {
                return NULL;
        }

        new_ht->size = 0;
        new_ht->num_buckets = DEFAULT_HT_BUCKETS;
        new_ht->buckets = malloc(sizeof(hashtable *) * new_ht->num_buckets);
        if (new_ht->buckets == NULL && errno == ENOMEM) {
                free (new_ht);
                return NULL;
        }

        int i;
        for (i = 0; i < new_ht->num_buckets; i++) {
                new_ht->buckets[i] = NULL;
        }

        return new_ht;
}

void ht_destroy (hashtable *ht)
{
        int i;
        for (i = 0; i < ht->num_buckets; i++) {
                if (ht->buckets[i] != NULL) {
                        linkedlist *cbucket = ht->buckets[i];

                        keyval *ckv;
                        while (ll_rmhead(cbucket, (void **)&ckv) == 0) {
                                free (ckv->key);
                                free (ckv);
                        }
                        free (cbucket);
                }
        }

        free (ht->buckets);
        free (ht);
}

// TODO: resizability
void ht_add (hashtable *ht, const char *key, void *elt)
{
        size_t knum = ht_hash(key, ht->num_buckets);

        if (ht->buckets[knum] == NULL) {
                ht->buckets[knum] = ll_make();
        }

        linkedlist *cbucket = ht->buckets[knum];
        keyval *ckv = NULL;
        ll_iter *cbucket_iter = ll_makeiter (cbucket);
        while ((ckv = (keyval *)ll_iter_next (cbucket_iter)) != NULL) {
                if (olstrcmp(ckv->key, key)) {
                        ckv->value = elt;
                        break;
                }
        }
        free (cbucket_iter);
        
        if (ckv == NULL) {
                char *lstr = strdup (key);
                keyval *nkv = malloc (sizeof(keyval));
                if (nkv == NULL) {
                        return;
                }
                nkv->key = lstr;
                nkv->value = elt;
                ht->size++;      
        }
}

int ht_get (hashtable *ht, const char *key, void **elt)
{
        if (ht == NULL) return 0;
        size_t knum = ht_hash(key, ht->num_buckets);

        if (ht->buckets[knum] == NULL) {
                *elt = NULL;
                return 0;
        }

        linkedlist *bucket = ht->buckets[knum];
        ll_iter *bucket_iter = ll_makeiter (bucket);
        keyval *ckv = NULL;
        *elt = NULL;
        while ((ckv = (keyval *)ll_iter_next (bucket_iter)) != NULL) {
                if (olstrcmp(ckv->key, key)) {
                        *elt = ckv->value;
                        break;
                }
        }
        free (bucket_iter);
        return (elt != NULL);
}

int ht_rm (hashtable *ht, const char *key, void **elt)
{
        if (ht == NULL) return 0;
        size_t knum = ht_hash(key, ht->num_buckets);
        if (ht->buckets[knum] == NULL) {
                *elt = NULL;
                return 0;
        }

        linkedlist *bucket = ht->buckets[knum];
        ll_iter *bucket_iter = ll_makeiter (bucket);
        keyval *ckv = NULL;
        *elt = NULL;
        while ((ckv = (keyval *)ll_iter_next (bucket_iter)) != NULL) {
                if (olstrcmp(ckv->key, key)) {
                        *elt = ckv->value;
                        free (ckv->key);
                        free (ckv);
                        ll_iter_rm (bucket_iter);
                        break;
                }
        }
        free (bucket_iter);
        return 1;
}

size_t ht_size (hashtable *ht)
{
        return ht->size;
}
