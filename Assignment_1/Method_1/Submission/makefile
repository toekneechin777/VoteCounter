CC=gcc
CLFAGS=-std=c99
DBFLAGS=-g

make: votecounter.c
	$(CC) $(CFLAGS) votecounter.c open_read.o -o votecounter.o

debug: intro.c
	$(CC) $(CFLAGS) $(DBFLAGS) votecounter.c open_read.o -o votecounter.o

clean:
	rm votecounter.o
	rm Output*
