CFLAGS=-Wall -Wextra -Wno-unused-parameter -g -O3 -funroll-loops

#CC=gcc -mtune=corei7-avx -march=corei7-avx
CC=clang -mtune=native -march=native -msse2 -msse3 -msse4 -msse4.1 -msse4.2 -mavx -mmmx


xxx:
	$(CC) $(CFLAGS) generate.c -o generate
	./generate > data.tsv
	gnuplot draw.gnuplot
