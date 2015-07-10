#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// self-include
#include "stack.h"

stack *s_make (void)
{
        return ll_make();
}

void s_push (stack *s, void *ptr)
{
        ll_prepend ((linkedlist *)s, ptr);
}

void s_pop (stack *s, void **ptr)
{
        ll_rmhead ((linkedlist *)s, ptr);
}

size_t s_len (stack *s)
{
        return ll_len ((linkedlist *)s);
}
