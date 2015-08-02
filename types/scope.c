#include <stdlib.h>
#include <errno.h>

// local includes
#include "../util/debug.h"
#include "hashtable.h"

// self-include
#include "scope.h"

scope_j *new_scope (scope_j *p)
{
        scope_j *nscope = malloc(sizeof(scope_j));
        if (nscope == NULL) {
                print_err_wno ("Could not allocate new scope.", errno);
                return NULL;
        }
        nscope->vars = ht_make();
        nscope->parent = p;
        if (p == NULL) {
                nscope->depth = 0;
        } else {
                nscope->depth = nscope->parent->depth + 1;
        }

        return nscope;
}

scope_j *leave_scope (scope_j *scope)
{
        scope_j *upscope = scope->parent;
        ht_destroy (scope->vars);
        free (scope);

        return upscope;
}
