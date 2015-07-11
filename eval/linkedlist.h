#ifndef JPSH_LINKEDLIST_H
#define JPSH_LINKEDLIST_H

#include <stddef.h>

typedef struct linkedlist_str linkedlist;
typedef struct ll_iter_str ll_iter;

linkedlist *ll_make();

// Note: this will not free/delete the elements of the linked list.
void ll_destroy(linkedlist *ll);

// All of the following functions will return 0 on success and some
// nonzero number on failure, also setting errno.
// Errors:
//  - ENOMEM on insert and append
//  - EINVAL on functions which specify an idx, if the idx is invalid;
//    or on any of these if ll is NULL.
int ll_insert(linkedlist *ll, void *elt, size_t idx);
int ll_append(linkedlist *ll, void *elt);
int ll_prepend(linkedlist *ll, void *elt);

int ll_rm(linkedlist *ll, void **elt, size_t idx);
int ll_rmtail(linkedlist *ll, void **elt);
int ll_rmhead(linkedlist *ll, void **elt);

int ll_get(linkedlist *ll, void **elt, size_t idx);
int ll_gethead(linkedlist *ll, void **elt);
int ll_gettail(linkedlist *ll, void **elt);

size_t ll_len(linkedlist *ll);

ll_iter *ll_makeiter(linkedlist *ll);
void *ll_iter_next (ll_iter *lli);
int ll_iter_hasnext (ll_iter *lli);
void ll_iter_rm (ll_iter *lli);

#endif
