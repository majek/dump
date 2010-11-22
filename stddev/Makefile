CC=gcc
PROJ=stddev
LIBS=-lm

all: test

test: $(PROJ)-test
	./$(PROJ)-test
	gcov -bc $(PROJ)-test.c 2>&1 >/dev/null
	python gcovst.py "" .

$(PROJ).h: Makefile
$(PROJ)-test.c: $(PROJ).h
$(PROJ)-test: $(PROJ)-test.c
	$(CC) -Wall -o $@ $^ -O3 -g --coverage $(LIBS)

clean:
	rm -f $(PROJ)-test *.gcov *.gcda *.gcno
