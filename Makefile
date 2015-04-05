CC=gcc
CFLAGS=-ggdb

jpsh: jpsh.c eval.c env.c str.c linebuffer.c cd.c exec.c
	$(CC) $(CFLAGS) -o jpsh jpsh.c eval.c env.c str.c linebuffer.c cd.c exec.c

