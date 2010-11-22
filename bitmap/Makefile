CC=gcc

all: test

test: bitmap-test
	./bitmap-test
	gcov -bc bitmap-test.h
	python gcovst.py "" .

bitmap.h: Makefile
bitmap.c: bitmap.h
bitmap-test.c: bitmap.c
bitmap-test: bitmap-test.c
	$(CC) -Wall -o $@ $^ -O3 -g --coverage

clean:
	rm -f bitmap-test *.gcov *.gcda *.gcno
