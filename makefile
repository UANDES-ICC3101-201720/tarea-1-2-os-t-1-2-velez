CC=gcc
CFLAGS=-std=gnu11 -Wall -Werror -pthread

all: quicksort datagen

datagen: datagen.c 
	$(CC) -o datagen datagen.c $(CFLAGS) -lm

quicksort: quicksort.c util.o
	$(CC) -o bank quicksort.c util.o $(CFLAGS)  -lm -lpthread

util.o:
	$(CC) -c -o util.o util.c $(CFLAGS)

clean:
	rm datagen quicksort *.o
 
