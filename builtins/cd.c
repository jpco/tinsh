#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

#include "builtins.h"

int cd (const char *arg);

int bi_cd (char **argv, int argc)
{
	if (argv[1] == NULL) {
		return cd (getenv("HOME"));
	}
	return cd (argv[1]);
}

int cd (const char *arg)
{
	// "cd with no args" is handled elsewhere.
	if (arg == NULL || *arg == '\0') return 1;
	char *pwd = getenv("PWD");

	// Builds the (rough) path of the passed argument.
	char newwd[PATH_MAX];
	if (*arg == '/') {
		strcpy(newwd, arg);
	} else {
		if (pwd == NULL) {
			perror ("cd");
			return 1;
		}
		snprintf(newwd, PATH_MAX, "%s/%s", pwd, arg);
	}

	// Canonicalizes the path.
	char newwd_c[PATH_MAX];
	if (!realpath(newwd, newwd_c)) {
		perror ("cd");
		return 1;
	}

	// Need to change the environment variable of PWD, AS WELL AS
	// the internal cwd dir.
	if (setenv("PWD", newwd_c, 1) != 0) {
		perror ("cd");
		return 1;
	}
	if (chdir(newwd_c) != 0) {
		perror ("cd");
		setenv("PWD", pwd, 1);
		return 1;
	}

	// Done.
	return 0;
}
