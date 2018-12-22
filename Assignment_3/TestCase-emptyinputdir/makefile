CC=gcc
DBFLAGS=-g
THREAD=-pthread

make: votecounter

votecounter: champ142chen4714PA3.c util.h
	$(CC) $(THREAD) $(DBFLAGS) champ142chen4714PA3.c util.h -o votecounter

clean:
	rm votecounter
