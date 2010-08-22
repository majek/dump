CC=gcc

all: test

test: hamt-test
	./hamt-test
	gcov -bc hamt-test.c
	python gcovst.py "" .

hamt.h: Makefile
hamt.c: hamt.h
hamt-test.c: hamt.c
hamt-test: hamt-test.c
	$(CC) -Wall -o $@ $^ -O3 -g --coverage

clean:
	rm -f hamt-test *.gcov *.gcda *.gcno

hamt-sort: hamt.c hamt-sort.c
	$(CC) -Wall -o $@ $^ -O3 -g -lssl