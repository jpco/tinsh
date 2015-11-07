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

// TODO: rehash() function

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
    char *path = getenv ("PATH");
    char *cpath = strtok (path, ":");
    for (; cpath; cpath = strtok (NULL, ":")) {
        DIR *cdir = opendir (cpath);
        if (!cdir) perror ("create_symtable");
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
                    char *fcpcpy = malloc (PATH_MAX);
                    strcpy (fcpcpy, fcpath);
                    nbin->value = fcpcpy;
                    
                    ht_add (bintable, celt->d_name, nbin);
                }
            }
        }

        closedir (cdir);
    }
}

sym_t *sym_resolve (char *key)
{
    sym_t *res;
    ht_get (bintable, key, (void **)&res);

    return res;
}
