CC=gcc
CFLAGS=-ggdb -Wall -lpthread -o tinsh
CFILES=$(wildcard *.c) $(wildcard posix/*.c) $(wildcard types/*.c) $(wildcard builtins/*.c)

tinsh: $(CFILES)
	$(CC) $(CFLAGS) $(CFILES)
