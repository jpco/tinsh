#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>

// for readdir et al.
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// local includes
#include "debug.h"
#include "str.h"
#include "exec.h"

// self-include
#include "compl.h"

// Completes a command from PATH.
char *path_compl (const char *wd);

// Completes a file. If exec = 0, any file works, if exec = 1,
// must be an executable file.
char *f_compl (char *wd, int exec);

char *l_compl (char *line, char *start, char *end)
{
        errno = 0;
        if (line == NULL) return NULL;
        if (start == NULL) return NULL;
        if (end == NULL) end = line + strlen(line);
        int linelen = strlen(line);
        char lchr = '\0';
        if (end < line+linelen)
                lchr = end[1];

        char *compl_wd = w_compl(start, (start == line ? 1 : 0));
        if (compl_wd == NULL || *compl_wd == '\0') {
                char *cline = vcombine_str(0, 1, line);
                return cline;
        }
        if (end < line+linelen)
                end[1] = lchr;

        char fchr = *start;
        *start = '\0';
        char *compl_line = vcombine_str(0, 3, line, compl_wd, end+1);
        free (compl_wd);
        return compl_line;
}

char *w_compl (char *wd, int first)
{
        // TODO: bash autocompletion 
        if (first) {
                if (strchr(wd, '/') == NULL) {
                        char *pathpl = path_compl(wd);
                        if (!pathpl) {
                                return f_compl(wd, 3);
                        }
                } else {
                        return f_compl(wd, 1);
                }
        } else {
                return f_compl(wd, 0);
        }
}

char *path_compl (const char *wd)
{
        // 1. builtins
        int i;
        int uniqfound = 0;
        char *nwd = calloc(100, sizeof(char));
        for (i = 0; i < NUM_BUILTINS-1; i++) {
                if (strncmp(wd, builtins[i], strlen(wd)) == 0) {
                        if (*nwd != '\0') {
                                int j;
                                int wlen = strlen(nwd);
                                int blen = strlen(builtins[i]);
                                for (j = 0; j < wlen || j < blen; j++) {
                                        if (j > blen || nwd[j] != builtins[i][j]) {
                                                nwd[j] = '\0';
                                        }
                                }
                                uniqfound = 0;
                        } else {
                                strcpy(nwd, builtins[i]);
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
        char *wdcpy = malloc((strlen(wd)+1)*sizeof(char));
        strcpy(wdcpy, wd);

        // get dirname & make a copy we own
        char *dirpath_ = dirname(wdcpy);
        char *dirpath = malloc((strlen(dirpath_)+1)*sizeof(char));
        strcpy(dirpath, dirpath_);
        // get basename & make a copy we own
        strcpy(wdcpy, wd);
        char *base_ = basename(wdcpy);
        char *base = malloc((strlen(base_)+1)*sizeof(char));
        strcpy(base, base_);

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
        return file;
}
