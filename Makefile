CC=gcc
CFLAGS=-ggdb -Wall -O2 -o tinsh

tinsh: $(wildcard *.c) $(wildcard exec/builtin/*.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c) $(wildcard util/*.c) $(wildcard types/*.c) $(wildcard tinf/*.c)
	$(CC) $(CFLAGS) $(wildcard *.c) $(wildcard exec/builtin/*.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c) $(wildcard types/*.c) $(wildcard tinf/*.c) $(wildcard util/*.c)
