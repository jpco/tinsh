CC=gcc
CFLAGS=-ggdb -Wall -O2

jpsh: $(wildcard *.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c)
	$(CC) $(CFLAGS) -o jpsh $(wildcard *.c) $(wildcard exec/*.c) $(wildcard eval/*.c) $(wildcard inter/*.c)
