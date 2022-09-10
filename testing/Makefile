CC=g++
CFLAGS=-O3
EXTRAFLAGS=-lpqxx -lpq -Wall -Wextra -lpthread

TARGETS=client


all: client $(TARGETS)

client: client.cpp
	$(CC) $(CFLAGS) -o client client.cpp $(EXTRAFLAGS)

clean:
	rm -f *~ *.o client $(TARGETS)

clobber:
	rm -f *~ *.o
