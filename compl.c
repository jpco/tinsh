#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <stdio.h>

#include <sys/types.h>
#include <dirent.h>

// local includes
#include "debug.h"
#include "str.h"

// self-include
#include "compl.h"

char *l_compl (char *line, char *start, char *end)
{
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
                return NULL;
        } else {
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

                // max len of dir element name (based on man readdir)
                char *file = calloc(256, sizeof(char));

                struct dirent *elt = readdir(dir);
                do {
                        if (strncmp(base, elt->d_name, baselen) == 0) {
                                if (*file != '\0') {
                                        // TODO: handle this better
                                        int i;
                                        int flen = strlen(file);
                                        int elen = strlen(elt->d_name);
                                        for (i = 0; i < flen || i < elen; i++) {
                                                if (file[i] != elt->d_name[i]) {
                                                        file[i] = '\0';
                                                }
                                        }
                                } else {
                                        strcpy(file, elt->d_name);
                                        if (elt->d_type == DT_DIR) {
                                                char *nfile = vcombine_str(0, 2, file, "/");
                                                free (file);
                                                file = nfile;
                                        }
                                }
                        }
                } while ((elt = readdir(dir)) != NULL);

                closedir(dir);
                return vcombine_str('/', 2, dirpath, file);
        }
}
