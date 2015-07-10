#ifndef JPSH_QUEUE_H
#define JPSH_QUEUE_H

#include <unistd.h>

#define MAX_QUEUE 100

typedef struct queue_str queue;

// Creates a new queue.
queue *q_make();

void q_push (queue *q, void *ptr);

void q_pop (queue *q, void **ptr);

size_t q_len (queue *q);

#endif
