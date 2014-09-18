/*
gcc -Wall -mmmx -msse -msse2 -msse3 cextensions-vector1.c -o cextensions-vector1 && ./cextensions-vector1
*/
#include <stdio.h>

typedef union{
	int v __attribute__ ((vector_size (16)));
	int i[4];
//	__m128i m;
} __vec_int;


int main(){
	__vec_int a, b, c;
	a.i[0] = 1; a.i[1] = 1; a.i[2] = 1; a.i[3] = 1;
	b.i[0] = 1; b.i[1] = 2; b.i[2] = 3; b.i[3] = 4;
	
	/* do OR here on 128 bits */
	c.v = b.v | a.v;
	
	printf("a=[%i,%i,%i,%i]\n", a.i[0], a.i[1], a.i[2], a.i[3]);
	printf("b=[%i,%i,%i,%i]\n", b.i[0], b.i[1], b.i[2], b.i[3]);
	printf("c=[%i,%i,%i,%i]\n", c.i[0], c.i[1], c.i[2], c.i[3]);

	return(0);
}
