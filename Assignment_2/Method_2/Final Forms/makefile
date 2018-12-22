CC=gcc
CLFAGS=-std=c99
DBFLAGS=-g

make: Leaf_Counter Aggregate_Votes Vote_Counter

Leaf_Counter: Leaf_Counter.c
	$(CC) $(CFLAGS) Leaf_Counter.c -o Leaf_Counter

Aggregate_Votes: Aggregate_Votes.c
	$(CC) $(CFLAGS) Aggregate_Votes.c -o Aggregate_Votes

Vote_Counter: Vote_Counter.c
	$(CC) $(CFLAGS) Vote_Counter.c -o Vote_Counter

clean:
	rm Vote_Counter
	rm Aggregate_Votes
	rm Leaf_Counter
