#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>

// for readdir et al.
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// local includes

#include "../util/str.h"
#include "../util/defs.h"
#include "../builtins/builtins.h"

// self-include
#include "compl.h"

char cpl_line[MAX_LINE];
size_t cpl_idx;

// Completes a command from PATH.
char *path_compl (const char *wd);

// Completes a file. If exec = 0, any file works, if exec = 1,
// must be an executable file.
char *f_compl (char *wd, int exec);

void cpl_cpy (const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        cpl_line[cpl_idx++] = src[i];
    }
    cpl_line[cpl_idx] = 0;
}

#define CPL_USEPATH 2
#define CPL_EXEC    1

// perform the completion.
// cplmask:
//  cplmask & 1: require executable
//  cplmask & 2: enable entries from PATH
int compl (char *wd, int cplmask);

int l_compl (char *line, char *start, char *end)
{
    // Safety checks
    errno = 0;
    if (line == NULL) return -3; // E_INVAL

    cpl_idx = 0;

    int linelen = strlen(line);
    if (start == NULL) start = line;
    if (end == NULL) end = line + linelen;

    // load the pre-word stuff into the buffer
    cpl_cpy (line, start - line);

    // Save the character which will be temporarily smashed
    char lchr = 0;
    if (end < line+linelen) {
        lchr = end[1];
        end[1] = 0;
    }

    // Do the completion
    int r;
    if ((r = w_compl (start, (start == line ? 1 : 0)))) {
        return r;
    }
    if (lchr) end[1] = lchr;

    // clean up
    cpl_cpy (end+1, linelen - (end - line));
    return 0;
}


int w_compl (char *owd, int first)
{
    // TODO: bash autocompletion 
    // TODO: var completion/expansion
    char wd[MAX_LINE];
    strncpy (wd, owd, MAX_LINE);

    // Cut out backslashes from word
    char *buf = wd;
    for (; *buf != '\0' && *(buf+1) != '\0'; buf++) {
        if (*buf == '\\' && is_separator(*(buf+1))) {
            rm_char (buf--);
        }
    }

    // perform the completion
    int r;
    if (first) {
        if (!strchr (wd, '/')) {
            r = compl (wd, CPL_USEPATH | CPL_EXEC);
        } else {
            r = compl (wd, CPL_EXEC);
        }
    } else {
        r = compl (wd, 0);
    }

    // TODO: re-insert backslashes, probably
    return r;
}

int compl (char *wd, int cplmask)
{
    if (cplmask & CPL_USEPATH) {
        char *pcl = path_compl (wd);
        if (pcl) {
            int pclen = strlen (pcl);
            cpl_cpy (pcl, pclen);
            free (pcl);
            return 0;
        }
    }

    char *fcl = f_compl (wd, cplmask & CPL_EXEC);
    if (fcl) {
        int fclen = strlen (fcl);
        cpl_cpy (fcl, fclen);
        free (fcl);
        return 0;
    }

    return -1;
}

char *path_compl (const char *wd)
{
    // 1. builtins
    int i;
    int uniqfound = 0;
    char *nwd = calloc(100, sizeof(char));
    for (i = 0; i < NUMBUILTINS-1; i++) {
        if (strncmp(wd, builtins[i].name, strlen(wd)) == 0) {
            if (*nwd != '\0') {
                int j;
                int wlen = strlen(nwd);
                int blen = strlen(builtins[i].name);
                for (j = 0; j < wlen || j < blen; j++) {
                    if (j > blen || nwd[j] != builtins[i].name[j]) {
                        nwd[j] = '\0';
                    }
                }
                uniqfound = 0;
            } else {
                strcpy(nwd, builtins[i].name);
                uniqfound = 1;
            }
        }
    }

    // 2. path
    char *path = getenv ("PATH");
    if (path == NULL)
        path = "/bin:/usr/bin";
    char *cpath = path;
    char *buf = path;
    while ((buf = strchr (cpath, ':')) || 1) {
        if (buf) *buf = '\0';

        // TODO: sloppy. should malloc outside of the loop.
        char *dirpath = realpath(cpath, NULL);
        if (dirpath == NULL) continue;

        DIR *dir = opendir(dirpath);
        if (dir == NULL) {
            free (dirpath);
            continue;
        }

        struct dirent *elt;
        while ((elt = readdir(dir))) {
            if (strncmp(wd, elt->d_name, strlen(wd)) != 0)
                continue;

            if (*nwd != '\0') {
                int j;
                int wlen = strlen(nwd);
                int elen = strlen(elt->d_name);
                for (j = 0; j < wlen || j < elen; j++) {
                    if (j > elen ||
                        nwd[j] != elt->d_name[j]) {
                        nwd[j] = '\0';
                    }
                }
                uniqfound = 0;
            } else {
                strcpy(nwd, elt->d_name);
                uniqfound = 1;
            }
        }
        closedir (dir);

        free (dirpath);
        if (buf) *buf = ':';
        if (buf) {
            cpath = buf + 1;
        } else {
            break;
        }
    }

    if (uniqfound) {
        nwd[strlen(nwd)] = ' ';
    }

    if (*nwd == '\0') {
        free (nwd);
        return NULL;
    }

    return nwd;
}

char *f_compl (char *wd, int exec)
{
    errno = 0;

    // malloc madness just to get dirname & basename
    char *wdcpy = strdup(wd);

    // get dirname & make a copy we own
    char *dirpath_ = dirname(wdcpy);
    char *dirpath = strdup (dirpath_);
    // get basename & make a copy we own
    strcpy(wdcpy, wd);
    char *base_ = basename(wdcpy);
    char *base = strdup (base_);

    // other variables
    int baselen = strlen(base);
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
        return NULL;
    }

    // max len of dir element name (based on man readdir)
    char *file = calloc(256, sizeof(char));

    // states:
    // 0 = none so far
    // 1 = one non-dir found
    // 2 = one dir found
    // 3 = more than one thing found
    int state = 0;
    
    struct dirent *elt;
    while ((elt = readdir(dir))) {
        if (strncmp(base, elt->d_name, baselen) != 0) continue;
        if (exec & 2) {
            if (elt->d_type != DT_DIR)
                continue;
        }
        if (exec & 1) {
            char *pwd = getenv("PWD");
            char *fi = vcombine_str('/', 3, pwd,
                    dirpath, elt->d_name);
            struct stat fistat;
            if (stat (fi, &fistat) < 0 ||
                    !(fistat.st_mode & S_IXUSR)) {
                free (fi);
                continue;
            }
            free (fi);
        }

        if (*file != '\0') {
            int i;
            int flen = strlen(file);
            int elen = strlen(elt->d_name);
            for (i = 0; i < flen || i < elen; i++) {
                if (file[i] != elt->d_name[i]) {
                    file[i] = '\0';
                }
            }
            state = 3;
        } else {
            strcpy(file, elt->d_name);

            if (elt->d_type == DT_DIR) {
                state = 2;
            } else {
                state = 1;
            }
        }
    }
    closedir(dir);

    if (state == 0) {
        return NULL;
    } else if (state == 1) {
        file[strlen(file)] = ' ';
    } else if (state == 2) {
        file[strlen(file)] = '/';
    }

    if (strcmp(dirpath, ".") != 0 ||
        strncmp(wd, "./", 2) == 0) {
        if (strcmp(dirpath, ".") == 0 && strcmp(file, ".") == 0) return NULL;
        char *nfile;
        if (strcmp(dirpath, "/") == 0) {
            nfile = vcombine_str(0, 2, dirpath, file);
        } else {
            nfile = vcombine_str('/',2, dirpath, file);
        }
        free (file);
        file = nfile;
    }

    free (wdcpy);
    free (dirpath);
    free (base);

    return file;
}
