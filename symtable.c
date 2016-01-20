#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "types/hashtable.h"
#include "symtable.h"

extern char **environ;

void create_symtable (void)
{
    bintable = ht_make();
    hash_bins();

    gscope = malloc (sizeof (struct scope));
    gscope->parent = NULL;
    gscope->symtable = ht_make();

    cscope = gscope;
}

void hash_bins (void)
{
    // populate symtable with binaries from PATH
    char path[PATH_MAX];
    strcpy (path, getenv ("PATH"));
    
    char *cpath = strtok (path, ":");
    for (; cpath; cpath = strtok (NULL, ":")) {

        DIR *cdir = opendir (cpath);

        if (!cdir) perror ("create_symtable");

        // FIXME: change to readdir_r
        struct dirent *celt = readdir (cdir);

        char fcpath[PATH_MAX];
        for (; celt; celt = readdir (cdir)) {
            snprintf (fcpath, PATH_MAX, "%s/%s", cpath, celt->d_name);

            if (!strcmp (celt->d_name, ".") || !strcmp (celt->d_name, "..")) continue;

            struct stat sb;
            if(!stat(fcpath, &sb)) {
                if(S_ISREG(sb.st_mode) && !access(fcpath, X_OK)) {
                    sym_t *nbin = malloc (sizeof (sym_t));
                    nbin->type = SYM_BINARY;
                    char *fcpcpy = malloc (1 + strlen (fcpath));
                    strcpy (fcpcpy, fcpath);
                    nbin->value = fcpcpy;
                    
                    ht_add (bintable, celt->d_name, nbin);
                }
            }
        }

        closedir (cdir);
    }
}

void free_sym (void *voidsym)
{
    sym_t *sym = (sym_t *)voidsym;

	// builtin data is loaded in program text, not the heap
	if (sym->type != SYM_BUILTIN)
		free (sym->value);
    free (sym);
}

void rehash_bins (void)
{
    ht_empty (bintable, free_sym);
    hash_bins();
}

int add_sym (char *name, void *val, sym_type type)
{
    sym_t *nsym = malloc (sizeof (sym_t));
    if (!nsym) return -1;
    nsym->type = type;
    nsym->value = (char *)val;

    ht_add (cscope->symtable, name, nsym);

    return 0;
}

sym_t *sym_resolve (const char *key, int ptypes)
{
    sym_t *res;
    struct scope *cs = cscope;

    for (; cs; cs = cs->parent) {
        if (!ht_get (cs->symtable, key, (void **)&res)) continue;
        if (ptypes && res && !(res->type & ptypes)) continue;

        return res;
    }

    ht_get (bintable, key, (void **)&res);
    if (ptypes && res && !(res->type & ptypes)) return NULL;

    return res;
}
