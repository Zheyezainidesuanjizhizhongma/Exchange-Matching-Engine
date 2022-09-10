CC=g++
CFLAGS=-O3
EXTRAFLAGS=-lpqxx -lpq -Wall -Wextra -lpthread

TARGETS=server


all: server $(TARGETS)


server: server.cpp tinyxml2.cpp handle.cpp db.cpp
	$(CC) $(CFLAGS) -o server server.cpp tinyxml2.cpp handle.cpp db.cpp $(EXTRAFLAGS)

clean:
	rm -f *~ *.o server $(TARGETS)

clobber:
	rm -f *~ *.o
