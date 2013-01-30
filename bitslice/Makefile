CFLAGS=-Wall -Wextra -Wno-unused-parameter -msse2 -msse3 -msse4 -msse4.1 -msse4.2 -mmmx -g -O3 -funroll-loops

#CC=gcc -mtune=core2 -march=core2
CC=clang -mavx -maes -mtune=corei7-avx -march=corei7-avx

all:
	$(CC) $(CFLAGS) main.c -o test
	./test
