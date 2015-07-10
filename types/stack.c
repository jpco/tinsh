#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// self-include
#include "stack.h"

struct stack_str {
        void *elts[MAX_STACK];
        size_t idx;
};

stack *s_make (void)
{
        stack *new_s = malloc(sizeof(stack));
        new_s->idx = 0;

        return new_s;
}

void s_push (stack *s, void *ptr)
{
        s->elts[s->idx] = ptr;
        if (s->idx < MAX_STACK - 1) {
                s->idx++;
        }
}

void s_pop (stack *s, void **ptr)
{
        if (s->idx > 0) {
                s->idx--;
        }
        *ptr = (void *)s->elts[s->idx];
}

size_t s_len (stack *s)
{
        return s->idx;
}
