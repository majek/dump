#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>


#ifdef __x86_64__
#  define RDRAND_LONG ".byte 0x48,0x0f,0xc7,0xf0"
#else
#  define RDRAND_INT  ".byte 0x0f,0xc7,0xf0"
#  define RDRAND_LONG   RDRAND_INT
#endif

#define RDRAND_RETRY_LOOPS  10

static inline long rdrand_long(unsigned long *v) {
    int ok;
    asm volatile("1: " RDRAND_LONG "\n\t"
             "jc 2f\n\t"
             "decl %0\n\t"
             "jnz 1b\n\t"
             "2:"
             : "=r" (ok), "=a" (*v)
             : "0" (RDRAND_RETRY_LOOPS));
    return ok;
}

static void fill_random(void *buf, unsigned buf_sz) {
	unsigned long *b = buf;
	unsigned b_sz = buf_sz / sizeof(long);

	unsigned i;
	for (i = 0; i < b_sz; i++) {
		rdrand_long(&b[i]);
	}
}


#define ROTATE(x,b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define HALF_ROUND(a,b,c,d,s,t)			\
	a += b; c += d;				\
	b = ROTATE(b, s) ^ a;	      		\
	d = ROTATE(d, t) ^ c;			\
	a = ROTATE(a, 32);

#define DOUBLE_ROUND(v0,v1,v2,v3)		\
	HALF_ROUND(v0,v1,v2,v3,13,16);		\
	HALF_ROUND(v2,v1,v0,v3,17,21);		\
	HALF_ROUND(v0,v1,v2,v3,13,16);		\
	HALF_ROUND(v2,v1,v0,v3,17,21);

#define SINGLE_ROUND(v0,v1,v2,v3)		\
	HALF_ROUND(v0,v1,v2,v3,13,16);		\
	HALF_ROUND(v2,v1,v0,v3,17,21);



static void sipround(uint64_t v[4]) {
	SINGLE_ROUND(v[0],v[1],v[2],v[3]);
//	DOUBLE_ROUND(v[0],v[1],v[2],v[3]);
}


int main() {
	const int rounds = 200000;

	int bit;
	for (bit = 0; bit < 256; bit++) {
		uint64_t bitmask = 1ULL << (bit & 63);

		uint64_t c[256] = {0};
		int r;
		for (r = 0; r < rounds; r++) {
			uint64_t a[4], b[4], diff[4];
			fill_random(a, sizeof(a));
			memcpy(b, a, sizeof(b));

			a[bit / 64] |= bitmask;
			b[bit / 64] &= ~bitmask;
			sipround(a);
			sipround(b);

			unsigned i, j;
			for (i=0; i < 4; i++)
				diff[i] = a[i]^b[i];
			for (i=0; i < 64; i++)
				for (j=0; j < 4; j++)
					c[j*64+i] += !!(diff[j] & (1ULL << i));
		}

		unsigned i;
		for(i=0; i < 256; i++) {
			printf("%i\t%i\t%.20f\n",
			       bit, i,
			       c[i] / (double)rounds);
		}
		printf("\n");
	}
}
