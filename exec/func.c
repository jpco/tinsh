#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// local includes
#include "../inter/color.h"
#include "../eval/eval_utils.h"

#include "../eval.h"
#include "../defs.h"
#include "../str.h"
#include "../var.h"
#include "../debug.h"

#include "cd.h"

// self-include
#include "func.h"

int func (int argc, const char **argv)
{
        if (strchr(argv[0], '/')) return 0;

        if (olstrcmp (argv[0], "exit")) {
                exit (0);
        } else if (olstrcmp (argv[0], "cd")) {
                if (argv[1] == NULL) { // going HOME
                        if (cd (getenv("HOME")) > 0) return 2;
                }
                if (cd (argv[1]) > 0) return 2;
        } else if (olstrcmp (argv[0], "pwd")) {
                printf("%s\n", getenv ("PWD"));
        } else if (olstrcmp (argv[0], "lsvars")) {
                ls_vars();
        } else if (olstrcmp (argv[0], "lsalias")) {
                ls_alias();
        } else if (olstrcmp (argv[0], "set")) {
                if (argc == 1) {
                        print_err ("Too few args.\n");
                } else {
                        char *keynval = combine_str(argv+1, argc-1, '\0');
                        if (strchr(keynval, '=') == NULL) {
                                set_var (keynval, NULL);
                        } else {
                                char **key_val = split_str (keynval, '=');

                                set_var (key_val[0], key_val[1]);
                                free (key_val[0]);
                                free (key_val);
                        }
                        free (keynval);
                }
        } else if (olstrcmp (argv[0], "setenv")) {
                if (argc == 1) {
                        print_err ("Too few args.\n");
                } else {
                        char *keynval = combine_str(argv+1, argc-1, '\0');
                        if (strchr(keynval, '=') == NULL) {
                                print_err ("Malformed \"setenv\".");
                        } else {
                                char **key_val = split_str (keynval, '=');
                                free (keynval);

                                setenv (key_val[0], key_val[1], 1);
                        }
                }
        } else if (olstrcmp (argv[0], "unset")) {
                if (argc < 2) {
                        print_err ("Too few args.\n");
                } else {
                        unset_var (argv[1]);
                }
        } else if (olstrcmp (argv[0], "unenv")) {
                if (argc < 2) {
                        print_err ("Too few args.\n");
                } else {
                        unsetenv (argv[1]);
                }
        } else if (olstrcmp (argv[0], "alias")) {
                if (argc < 3) {
                        print_err ("Too few args.\n");
                } else {
                        set_alias (argv[1], argv[2]);
                }
        } else if (olstrcmp (argv[0], "unalias")) {
                if (argc < 2) {
                        print_err ("Too few args.\n");
                } else {
                        unset_alias (argv[1]);
                }
        } else if (olstrcmp (argv[0], "color")) {
                if (argc < 2) {
                        print_err ("Too few args.\n");
                } else {
                        color_line (argc-1, argv+1);
                }
        } else {
                return 0;
        }

        return 1;

}
