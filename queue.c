#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "queue.h"

/* typedef struct {
        void *elts[MAX_QUEUE];
        size_t q_last;
        size_t q_first;
} queue; */

queue *q_make (void)
{
        queue *new_q = malloc(sizeof(queue));
        new_q->q_last = 0;
        new_q->q_first = 0;

        return new_q;
}

void q_push (queue *q, void *ptr)
{
        q->elts[q->q_last] = ptr;
        if (q->q_last < MAX_QUEUE - 1) {
                q->q_last++;
        } else {
                q->q_last = 0;
        }
}

void q_pop (queue *q, void **ptr)
{
        *ptr = (void *)q->elts[q->q_first];
        if (q->q_first < MAX_QUEUE - 1) {
                q->q_first++;
        } else {
                q->q_first = 0;
        }
}

size_t q_len (queue *q)
{
        if (q->q_last >= q->q_first) {
                return q->q_last - q->q_first;
        } else {
                return MAX_QUEUE - q->q_first + q->q_last;
        }
}
