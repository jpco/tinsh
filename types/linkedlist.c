#include <stddef.h>
#include <errno.h>
#include <stdlib.h>

// self-include
#include "linkedlist.h"

typedef struct ll_elt_str {
        void *data;
        struct ll_elt_str *next;
        struct ll_elt_str *prev;
} ll_elt;

struct linkedlist_str {
        ll_elt *head;
        ll_elt *tail;
        size_t len;
};

struct ll_iter_str {
        linkedlist *list;
        ll_elt *celt;
};

linkedlist *ll_make (void)
{
        linkedlist *nll = malloc(sizeof(linkedlist));
        if (nll == NULL && errno == ENOMEM) {
                return NULL;
        }
        nll->head = NULL;
        nll->tail = NULL;
        nll->len = 0;

        return nll;
}

void ll_destroy (linkedlist *ll)
{
        if (ll == NULL) return;
        
        void *trash;
        while (ll->head != NULL) {
                ll_rmhead (ll, &trash);
        }
        free (ll);
}

int ll_insert (linkedlist *ll, void *elt, size_t idx)
{
        if (ll == NULL || idx > ll->len) {
                errno = EINVAL;
                return 1;
        }

        ll_elt *n_elt = malloc(sizeof(ll_elt));
        if (n_elt == NULL && errno == ENOMEM) {
                return 1;
        }
        n_elt->data = elt;
        n_elt->next = NULL;
        n_elt->prev = NULL;

        if (ll->len == 0) {
                ll->head = n_elt;
                ll->tail = n_elt;
        } else if (idx == 0) {
                n_elt->next = ll->head;
                ll->head->prev = n_elt;
                ll->head = n_elt;
        } else {
                size_t i;
                ll_elt *c_elt = ll->head;
                for (i = 1; i < idx; i++) {
                        c_elt = c_elt->next;
                }

                n_elt->prev = c_elt;
                n_elt->next = c_elt->next;
                if (c_elt->next != NULL)
                        c_elt->next->prev = n_elt;
                c_elt->next = n_elt;
        }

        ll->len++;
        return 0;
}

int ll_append (linkedlist *ll, void *elt)
{
        if (ll == NULL) {
                errno = EINVAL;
                return 1;
        }

        ll_elt *n_elt = malloc(sizeof(ll_elt));
        if (n_elt == NULL && errno == ENOMEM) {
                return 1;
        }
        n_elt->data = elt;
        n_elt->next = NULL;
        
        if (ll->len == 0) {
                ll->tail = n_elt;
                ll->head = n_elt;
                n_elt->prev = NULL;
        } else {
                ll->tail->next = n_elt;
                n_elt->prev = ll->tail;
                ll->tail = n_elt;
        }

        ll->len++;
        return 0;
}

int ll_prepend (linkedlist *ll, void *elt)
{
        if (ll == NULL) {
                errno = EINVAL;
                return 1;
        }

        ll_elt *n_elt = malloc(sizeof(ll_elt));
        if (n_elt == NULL && errno == ENOMEM) {
                return 1;
        }
        n_elt->data = elt;
        n_elt->prev = NULL;
        
        if (ll->len == 0) {
                ll->head = n_elt;
                ll->tail = n_elt;
                n_elt->next = NULL;
        } else {
                ll->head->prev = n_elt;
                n_elt->next = ll->head;
                ll->head = n_elt;
        }

        ll->len++;
        return 0;
}

int ll_rm (linkedlist *ll, void **elt, size_t idx)
{
        // this >= is correct.
        if (ll == NULL || idx >= ll->len) {
                errno = EINVAL;
                return 1;
        }

        if (idx == 0) {
                return ll_rmhead (ll, elt);
        } else {
                size_t i;
                ll_elt *c_elt = ll->head;
                for (i = 1; i <= idx; i++) {
                        c_elt = c_elt->next;
                }

                *elt = c_elt->data;
                c_elt->prev->next = c_elt->next;
                if (c_elt->next != NULL)
                        c_elt->next->prev = c_elt->prev;

                free (c_elt);
                ll->len--;
        }

        return 0;
}

int ll_rmtail (linkedlist *ll, void **elt)
{
        if (ll == NULL || ll->tail == NULL) {
                elt = NULL;
                errno = EINVAL;
                return 1;
        }

        ll_elt *rm_elt = ll->tail;
        ll->tail = rm_elt->prev;
        if (ll->tail != NULL) ll->tail->next = NULL;

        ll->len--;
        *elt = rm_elt->data;
        free (rm_elt);
        
        return 0;
}

int ll_rmhead (linkedlist *ll, void **elt)
{
        if (ll == NULL || ll->head == NULL) {
                elt = NULL;
                errno = EINVAL;
                return 1;
        }

        ll_elt *rm_elt = ll->head;
        ll->head = rm_elt->next;
        if (ll->head != NULL) ll->head->prev = NULL;

        ll->len--;
        *elt = rm_elt->data;
        free (rm_elt);
        
        return 0;
}

int ll_get (linkedlist *ll, void **elt, size_t idx)
{
        // this >= is correct.
        if (ll == NULL || idx >= ll->len) {
                errno = EINVAL;
                return 1;
        }

        if (idx == 0) {
                return ll_gethead (ll, elt);
        } else {
                size_t i;
                ll_elt *c_elt = ll->head;
                for (i = 1; i <= idx; i++) {
                        c_elt = c_elt->next;
                }

                *elt = c_elt->data;
        }

        return 0;
}

int ll_gethead (linkedlist *ll, void **elt)
{
        if (ll == NULL || ll->head == NULL) {
                errno = EINVAL;
                return 1;
        }

        ll_elt *head = ll->head;
        *elt = head->data;

        return 0;
}

int ll_gettail (linkedlist *ll, void **elt)
{
        if (ll == NULL || ll->tail == NULL) {
                errno = EINVAL;
                return 1;
        }

        ll_elt *tail = ll->tail;
        *elt = tail->data;

        return 0;

}

size_t ll_len (linkedlist *ll)
{
        return ll->len;
}

// ll_iter functions

ll_iter *ll_makeiter (linkedlist *ll)
{
        ll_iter *nlli = malloc(sizeof(ll_iter));
        if (nlli == NULL) return NULL;
        
        nlli->list = ll;
        nlli->celt = ll->head;

        return nlli;
}

void *ll_iter_next (ll_iter *lli)
{
        if (lli == NULL) return NULL;
        if (lli->celt != NULL) lli->celt = lli->celt->next;
        return lli->celt->data;
}

int ll_iter_hasnext (ll_iter *lli)
{
        if (lli == NULL) return 0;
        if (lli->celt == NULL) return 0;
        if (lli->celt->next == NULL) return 0;
        return 1;
}

void ll_iter_rm (ll_iter *lli)
{
        if (lli == NULL) return;
        if (lli->list == NULL) return;
        if (lli->celt == NULL) return;

        if (lli->celt->prev != NULL) lli->celt->prev->next = lli->celt->next;
        if (lli->celt->next->prev != NULL) lli->celt->next->prev = lli->celt->prev;
        lli->celt = lli->celt->next;
        free (lli->celt);
}
