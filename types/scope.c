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
        nscope->fns = ht_make();
        nscope->parent = p;

        return nscope;
}

scope_j *leave_scope (scope_j *scope)
{
        scope_j *upscope = scope->parent;
        ht_destroy (scope->vars);
        ht_destroy (scope->fns);
        free (scope);

        return upscope;
}
