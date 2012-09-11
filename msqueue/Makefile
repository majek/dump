
CC:=gcc
CFLAGS:=-O3 -g -pthread -Wall
LIBS:=-lrt -lm

all: blocking semiblocking

clean:
	rm -f blocking semiblocking

blocking: main.c queue_lock_mutex.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

semiblocking: main.c queue_semiblocking.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
