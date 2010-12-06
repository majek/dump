CC=gcc
PROJ=bitmap

all: test

test: $(PROJ)-test
	./$(PROJ)-test
	gcov -bc $(PROJ)-test.c
	python gcovst.py "" .

$(PROJ).h: Makefile
$(PROJ)-test.c: $(PROJ).h
$(PROJ)-test: $(PROJ)-test.c
	$(CC) -Wall -o $@ $^ -O3 -g --coverage

clean:
	rm -f $(PROJ)-test *.gcov *.gcda *.gcno
