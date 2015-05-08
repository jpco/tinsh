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
        if (compl_wd == NULL) return line;
        if (end < line+linelen)
                end[1] = lchr;

        char fchr = *start;
        *start = '\0';
//        printf("%x, %x, %x\n", line, compl_wd, end+1);
//        printf("%s, %s, %s\n", line, compl_wd, end+1);
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
                char *dirpath = dirname(wd);
                char *base = basename(wd);
                int baselen = strlen(base);
                DIR *dir = opendir(dirpath);

                // max len of dir element name (based on man readdir)
                char *file = calloc(256, sizeof(char));

                struct dirent *elt = readdir(dir);
                do {
                        if (strncmp(base, elt->d_name, baselen) == 0) {
                                if (*file != '\0') {
                                        strcpy(file, wd);
                                        closedir(dir);
                                        return file;
                                } else {
                                        strcpy(file, elt->d_name);
                                }
                        }
                } while ((elt = readdir(dir)) != NULL);

                closedir(dir);
                return file;
        }
}
