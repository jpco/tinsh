#ifndef JPSH_QUEUE_H
#define JPSH_QUEUE_H

#include "linkedlist.h"
#include <unistd.h>

#define MAX_QUEUE 100

typedef struct linkedlist_str queue;

// Creates a new queue.
queue *q_make();
void q_destroy (queue *q);

void q_push (queue *q, void *ptr);

void q_pop (queue *q, void **ptr);

void q_preempt (queue *q, queue *new);

size_t q_len (queue *q);

#endif
