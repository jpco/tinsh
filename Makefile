CC=gcc
CFLAGS=-ggdb -Wall -O2

tinsh: $(wildcard *.c) $(wildcard exec/builtin/*.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c) $(wildcard util/*.c) $(wildcard types/*.c) $(wildcard tinf/*.c)
	$(CC) $(CFLAGS) -o tinsh $(wildcard *.c) $(wildcard exec/builtin/*.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c) $(wildcard types/*.c) $(wildcard tinf/*.c) $(wildcard util/*.c)
