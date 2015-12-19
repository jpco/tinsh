#include <stdlib.h>
#include <stdio.h>

#include "builtins.h"
#include "../hist.h"

#ifndef CMD_MAX
#define CMD_MAX 100
#endif

int bi_history (char **argv,  int argc)
{
	char histbuf[CMD_MAX];
	int i;
	printf ("History (most recent first):\n");
	for (i = 1; i <= 10; i++) {
		hist_up (histbuf);
		printf ("(%d) %s", i, histbuf);
	}
	for (i = 1; i <= 10; i++) hist_down (histbuf);

	return 0;
}
