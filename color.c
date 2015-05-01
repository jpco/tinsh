#include <unistd.h>
#include <stdio.h>

// local includes
#include "exec.h"

// self-include
#include "color.h"

void color_line (int argc, const char **argv)
{
        int i;
        if (argc == 0) return;

        int ex = chk_exec(argv[0]);
        for (i = 0; i < argc; i++) {
                if (i == 0 && ex == 2) {
                        printf("\e[1;32m%s\e[0m ", argv[i]);
                } else if (i == 0 && ex == 1) {
                        printf("\e[0;32m%s\e[0m ", argv[i]);
                } else if (!access(argv[i], F_OK)) {
                        printf("\e[0;35m%s\e[0m ", argv[i]);
                } else {
                        printf("%s ", argv[i]);
                }
        }
        printf("\n");
}
