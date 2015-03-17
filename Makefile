CC=gcc
CFLAGS=-ggdb

jpsh: jpsh.c eval.c debug.c parseline.c builtin.c exec.c cd.c str.c env.c hist.c
	$(CC) $(CFLAGS) -o jpsh debug.c eval.c jpsh.c parseline.c builtin.c exec.c cd.c str.c env.c hist.c

