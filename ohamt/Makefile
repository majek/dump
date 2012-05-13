
all:
	gcc -ggdb -g -O0 -march=native -mtune=native -Wall hamt.c hamt-sort.c -lssl -o hamt-sort
	gcc -ggdb -g -O0 -march=native -mtune=native -Wall hamt-test.c 
	valgrind ./a.out
