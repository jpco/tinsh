CC=gcc
CFLAGS=-ggdb -Wall -O2

impsh: $(wildcard *.c) $(wildcard exec/builtin/*.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c) $(wildcard util/*.c) $(wildcard types/*.c) $(wildcard jpcf/*.c)
	$(CC) $(CFLAGS) -o impsh $(wildcard *.c) $(wildcard exec/builtin/*.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c) $(wildcard types/*.c) $(wildcard impf/*.c) $(wildcard util/*.c)
