#ifndef _JPSH_QUEUE_H_
#define _JPSH_QUEUE_H_

#include <unistd.h>

#define MAX_QUEUE 100

typedef struct {
        void *elts[MAX_QUEUE];
        size_t q_last;
        size_t q_first;
} queue;

queue *q_make();

void q_push (queue *q, void *ptr);

void q_pop (queue *q, void **ptr);

size_t q_len (queue *q);

#endif
