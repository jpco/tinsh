CC=gcc
CFLAGS=-ggdb -Wall -o tinsh
CFILES=$(wildcard *.c) $(wildcard posix/*.c)

tinsh: $(CFILES)
	$(CC) $(CFLAGS) $(CFILES)
