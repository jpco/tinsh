CC=gcc
CFLAGS=-ggdb -Wall -o tinsh
CFILES=$(wildcard *.c) $(wildcard posix/*.c) $(wildcard types/*.c) $(wildcard builtins/*.c) $(wildcard inter/*.c) $(wildcard util/*.c)

tinsh: $(CFILES)
	$(CC) $(CFLAGS) $(CFILES)
