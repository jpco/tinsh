CC=gcc
CFLAGS=-ggdb -Wall -O2

jpsh: $(wildcard *.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c) $(wildcard util/*.c) $(wildcard types/*.c) $(wildcard jpcf/*.c)
	$(CC) $(CFLAGS) -o jpsh $(wildcard *.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c) $(wildcard types/*.c) $(wildcard jpcf/*.c) $(wildcard util/*.c)
