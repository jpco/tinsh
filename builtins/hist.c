#include <stdlib.h>
#include <stdio.h>

#include "builtins.h"
#include "../inter/hist.h"

#include "../util/defs.h"

int bi_history (char **argv,  int argc)
{
	char histbuf[MAX_LINE];
	int i;
	printf ("History (most recent first):\n");
	for (i = 1; i <= 10; i++) {
		if (hist_up (histbuf)) break;
		printf ("(%d) %s\n", i, histbuf);
	}
	for (i = 1; i <= 10; i++) hist_down (histbuf);

	return 0;
}
