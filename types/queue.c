#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// self-include
#include "queue.h"

queue *q_make (void)
{
        queue *new_q = ll_make();
        return new_q;
}

void q_push (queue *q, void *ptr)
{
        ll_append ((linkedlist *)q, ptr);
}

void q_pop (queue *q, void **ptr)
{
        ll_rmhead ((linkedlist *)q, ptr);
}

void q_preempt (queue *q, queue *new)
{
        linkedlist *ll_new = (linkedlist *)new;

        void *elt = NULL;
        while ((ll_rmtail(new, elt)) == 0) {
                ll_prepend ((linkedlist *)q, elt);
        }
}

size_t q_len (queue *q)
{
        return ll_len ((linkedlist *)q);
}
