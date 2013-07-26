#include <stdint.h>
#include <stdio.h>

#define ROTATE(x, b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define HALF_ROUND(a,b,c,d,s,t)			\
	a += b; c += d;				\
	b = ROTATE(b, s) ^ a;			\
	d = ROTATE(d, t) ^ c;			\
	a = ROTATE(a, 32);

#define DOUBLE_ROUND(v0,v1,v2,v3)		\
	HALF_ROUND(v0,v1,v2,v3,13,16);		\
	HALF_ROUND(v2,v1,v0,v3,17,21);		\
	HALF_ROUND(v0,v1,v2,v3,13,16);		\
	HALF_ROUND(v2,v1,v0,v3,17,21);



typedef uint64_t v4u64 __attribute__ ((vector_size (32)));


v4u64 siphash_round(uint64_t a, uint64_t b, uint64_t c, uint64_t d);



union pun {
	v4u64 v;
	uint64_t u[4];
};


/*  __attribute__((noinline))  v4u64 siphash_round(uint64_t a, uint64_t b, uint64_t c, uint64_t d){ */
/* 	union pun p; */
/* 	a *= a; */
/* 	b += a; */
/* 	c += a * 2; */
/* 	d = d + a; */
/* 	p.u[0] = a; */
/* 	p.u[1] = b; */
/* 	p.u[2] = c; */
/* 	p.u[3] = d; */
/* 	return p.v; */
/* } */


int main(int argc, char **argv) {
	uint64_t a,b,c,d;
	union pun p;

	a = argc; b = 10; c = 2; d = 3;
	printf("good:\n");
//	printf("%016lx %016lx\n%016lx %016lx\n", a, b, c, d);

	HALF_ROUND(a,b,c,d,13,16);
	HALF_ROUND(c,b,a,d,17,21);
	union pun r = {.v = {a,b,c,d}};
	printf("%016lx %016lx\n%016lx %016lx\n", r.v[0], r.v[1], r.v[2], r.v[3]);


	a = argc; b = 10; c = 2; d = 3;
	printf("bad:\n");
//	printf("%016lx %016lx\n%016lx %016lx\n", a,b,c,d);
	p.v = siphash_round(a,b,c,d);
	printf("%016lx %016lx\n%016lx %016lx\n", p.v[0], p.v[1], p.v[2], p.v[3]);

	return 0;
}
