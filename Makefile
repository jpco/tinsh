CC=gcc
CFLAGS=-ggdb

jpsh: jpsh.c eval.c env.c str.c linebuffer.c cd.c exec.c hist.c term.c debug.c color.c defs.c compl.c
	$(CC) $(CFLAGS) -o jpsh jpsh.c eval.c env.c str.c linebuffer.c cd.c exec.c hist.c term.c debug.c color.c defs.c compl.c

