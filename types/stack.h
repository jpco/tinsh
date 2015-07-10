#ifndef JPSH_STACK_H
#define JPSH_STACK_H

#include <unistd.h>

#define MAX_STACK 100

typedef struct stack_str stack;

// Creates a new stack.
stack *s_make();

void s_push (stack *s, void *ptr);

void s_pop (stack *s, void **ptr);

size_t s_len (stack *s);

#endif
