/*
gcc -Wall -msse2 cextensions-vector2.c -o cextensions-vector2 && ./cextensions-vector2
*/
#include <stdio.h>

#include <xmmintrin.h>


typedef __m128i vec_int;

typedef union{
//	int vec __attribute__ ((vector_size (16)));
	int s[4];
	__m128i v;
} __vec_int;

inline vec_int   set_vec_int( int a, int b, int c, int d)                     { __vec_int temp; temp.s[0]=a; temp.s[1]=b; temp.s[2]=c; temp.s[3]=d; return temp.v; }

int main(){
	__vec_int a,b;
	a.v = set_vec_int(1,2,3,4);
	b.v = set_vec_int(4,3,2,1);
	
	/* compare greater than on __m128i */
        __vec_int c;
        c.v = _mm_cmpgt_epi32(a.v, b.v);	/* 0xFFFFFFFF if gt, 0 else*/
	
	printf("a=[%i,%i,%i,%i]\n", a.s[0], a.s[1], a.s[2], a.s[3]);
	printf("b=[%i,%i,%i,%i]\n", b.s[0], b.s[1], b.s[2], b.s[3]);
	printf("c=[%u,%u,%u,%u]\n", c.s[0] ? 1 : 0, c.s[1] ? 1 : 0, c.s[2] ? 1 : 0, c.s[3] ? 1 : 0);
	
	return(0);
}

