#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// local includes
#include "defs.h"
#include "str.h"
#include "debug.h"
#include "eval.h"

// self-include
#include "var.h"

typedef struct scope_st {
        var_t *vars[MAX_VARS];
        int vars_len;
        struct scope_st *parent;
} scope_t;

// environment variables are stored
// more deeply in the system, using setenv() and getenv() calls
static var_t *aliases[MAX_ALIASES];
static int alias_len;

// Current scope & global scope
static scope_t *gscope = NULL;
static scope_t *cscope = NULL;

//
// SECTION: SCOPE MODIFIERS
//
void new_scope (void)
{
        scope_t *nscope = malloc(sizeof(scope_t));
        nscope->parent = cscope;
        nscope->vars_len = 0;
        cscope = nscope;
        if (gscope == NULL) gscope = cscope;
}

void leave_scope (void)
{
        if (cscope->parent == NULL) {
                print_err ("Cannot leave to NULL scope");
                return;
        }

        scope_t *rmscope = cscope;
        cscope = cscope->parent;

        int i;
        for (i = 0; i < rmscope->vars_len; i++) {
                free (rmscope->vars[i]->key);
                free (rmscope->vars[i]->value);
                free (rmscope->vars[i]);
        }

        free (rmscope);
}

//
// SECTION: GETTERS & SETTERS
//

/**
 * Returns 1 if the passed array contains the passed key, 0 otherwise.
 */
int has_value (const char *key, var_t **arr, int arr_len)
{
        int i;
        for (i = 0; i < arr_len; i++)
                if (olstrcmp (key, arr[i]->key))
                        return 1;

        return 0;
}

/**
 * Returns a pointer to the value in the passed array associated with
 * the passed key, or NULL on error or if the value is not found.
 */
char *get_value (const char *key, var_t **arr, int arr_len)
{
        char *retvar = NULL;
        int i;
        for (i = 0; i < arr_len; i++) {
                if (strcmp(key, arr[i]->key) == 0) {
                        retvar = malloc (sizeof(char) * strlen(arr[i]->value)+1);
                        if (retvar == NULL && errno == ENOMEM) {
                                print_err ("Could not malloc var value.");
                                return NULL;
                        }
                        strcpy (retvar, arr[i]->value);
                        break;
                }
        }

        return retvar;
}

/**
 * Adds the key-value pair to the passed array; if the key is already
 * in the array, discards the old value and associates the key with the
 * new value. If there is an error, does not affect the array.
 */
void set_value (const char *key, const char *value,
                var_t **arr, int *arr_len, int max)
{
        char *l_key = trim_str (key);
        char *l_value;
        if (value) {
                l_value = trim_str (value);
        } else {
                l_value = trim_str ("");
        }

        int i;
        for (i = 0; i < *arr_len; i++) {
                if (olstrcmp(key, arr[i]->key)) {
                        free (l_key);
                        if (arr[i]->value != NULL) free (arr[i]->value);
                        arr[i]->value = l_value;
                        return;
                }
        }

        if (*arr_len < max) {
                arr[*arr_len] = malloc(sizeof(var_t));
                if (arr[*arr_len] == NULL && errno == ENOMEM) {
                        print_err ("Could not malloc new key-value pair.");
                }
                arr[*arr_len]->key = l_key;
                arr[(*arr_len)++]->value = l_value;
        } else {
                dbg_print_err ("Variable array is full.");
        }
}

int has_var (const char *key)
{
        scope_t *lscope = cscope;
        for (; lscope != NULL; lscope = lscope->parent) {
                if (has_value (key, lscope->vars, lscope->vars_len))
                        return 1;
        }

        return 0;
}

char *get_var (const char *key)
{
        scope_t *lscope = cscope;
        for (; lscope != NULL; lscope = lscope->parent) {
                if (has_value (key, lscope->vars, lscope->vars_len))
                        return get_value (key,
                                          lscope->vars,
                                          lscope->vars_len);
        }

        return NULL;
}

void set_var (const char *key, const char *value)
{
        set_value (key, value, cscope->vars, &(cscope->vars_len),
                        MAX_VARS);
}

/**
 * Removes the key-value pair from the array, or does nothing if
 * the passed key is not in the array.
 */
void unset_value (const char *key, var_t **arr, int *arr_len)
{
        int i;
        for (i = 0; i < *arr_len; i++) {
                if (strcmp (arr[i]->key, key) == 0) {
                        var_t *torm = arr[i];
                        int j;
                        for (j = i; j < *arr_len-1; j++) {
                                arr[j] = arr[j+1];
                        }
                        arr[--(*arr_len)] = NULL;
                        free (torm->key);
                        free (torm->value);
                        free (torm);
                }
        }
}


void unset_var (const char *key)
{
        scope_t *lscope = cscope;
        for (; lscope != NULL; lscope = lscope->parent) {
                if (has_value (key, lscope->vars, lscope->vars_len))
                        unset_value (key, lscope->vars,
                                        &(lscope->vars_len));
        }
}

void ls_vars (void)
{
        printf ("Vars:\n");
        int i;
        scope_t *lscope = cscope;
        for (; lscope != NULL; lscope = lscope->parent) {
                for (i = 0; i < lscope->vars_len; i++) {
                        if (strcmp(lscope->vars[i]->value, "") != 0) {
                                printf("%s = %s\n", lscope->vars[i]->key,
                                                lscope->vars[i]->value);
                        } else {
                                printf("%s set\n", lscope->vars[i]->key);
                        }
                }
        }
}

void set_alias (const char *key, const char *value)
{
        set_value (key, value, aliases, &alias_len, MAX_ALIASES);
}

void unset_alias (const char *key)
{
        unset_value (key, aliases, &alias_len);
}

int has_alias (const char *key)
{
        return has_value (key, aliases, alias_len);
}

char *get_alias (const char *key)
{
        return get_value (key, aliases, alias_len);
}

void ls_alias (void)
{
        printf ("Aliases:\n");
        int i;
        for (i = 0; i < alias_len; i++)
                printf("%s = %s\n", aliases[i]->key, aliases[i]->value);
}

//
// SECTION: ENVIRONMENT DESTRUCTION
//

/**
 * Frees the static heap-allocated storage used to store
 * the environment.
 */
void free_env (void)
{
        if (cscope != NULL) {
                while (cscope->parent != NULL) {
                        leave_scope();
                }
                cscope = NULL;
        }

        if (gscope != NULL) {
                int i;
                for (i = 0; i < gscope->vars_len; i++) {
                        free (gscope->vars[i]->key);
                        free (gscope->vars[i]->value);
                        free (gscope->vars[i]);
                }
                free (gscope);
                gscope = NULL;
        }

        int i;
        for (i = 0; i < alias_len; i++) {
                var_t *torm = aliases[i];

                if (torm == NULL) continue;
                free (torm->key);
                free (torm->value);
                free (torm);

                aliases[i] = NULL;
        }
}
