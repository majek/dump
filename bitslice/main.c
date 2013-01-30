#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <emmintrin.h>

#include "timing.h"


typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint64_t vec __attribute__ ((vector_size (32)));


/* Bit-transposition code stolen from:
   http://mischasan.wordpress.com/2011/10/03/the-full-sse2-bit-matrix-transpose-routine/ */
#define BYTES(x) (((x) + 7) >> 3)
#define DOBLOCK(Rows, Bits)						\
        for (cc = 0; cc < ncols; cc += 8 ) {				\
		for (i = 0; i < Rows; ++i)				\
			minp.b[i] = inp[(rr + i)*cb + (cc >> 3)];	\
		for (i = 8; --i >= 0; ) {				\
			*(uint##Bits##_t*)&out[(rr >> 3) + (cc + i)*rb] = \
				_mm_movemask_epi8(minp.x);		\
			minp.x = _mm_slli_epi64(minp.x, 1);		\
		}							\
        }

static inline void transpose(uint8_t const *inp, uint8_t *out, int nrows, int ncols) {
	int rr, cc, i, cb = BYTES(ncols), rb = BYTES(nrows), left = nrows & 15;
	union { __m128i x; uint8_t b[16]; } minp;
	for (rr = 0; rr <= nrows - 16; rr += 16)
		DOBLOCK(16, 16);
	if (left > 8 ) {
		DOBLOCK(left, 16);
	} else if (left > 0) {
		DOBLOCK(left,  8 );
	}
}

static inline void transpose_to_vec(vec vec[8], u8 raw[256]) {
	transpose((u8*)raw, (u8*)vec, 256, 8);
}

static inline void transpose_to_raw(u8 raw[256], vec vec[8]) {
	transpose((u8*)vec, (u8*)raw, 8, 256);
}


static u8 formula_raw(u8 a, u8 b) {
	return a | b;
}

static void formula_bs(vec dst[8], vec a[8], vec b[8]) {
	int o;
	for (o = 0; o < 8; o++) {
		dst[o] = a[o] | b[o];
	}
}


#define ROUNDS 1000
int main() {
	int i, round;
	u64 cyc0, cyc1, cyc, trans;
	u8 a[256], b[256], c[256];
	memset(a, 0x01, sizeof(a));
	memset(b, 0x01, sizeof(b));


	cyc = -1LL;
	for (round = 0; round < ROUNDS; round++) {
		RDTSC_START(cyc0);

		for (i = 0; i < 256; i++) {
			c[i] = formula_raw(a[i], b[i]);
		}
		RDTSC_STOP(cyc1);
		cyc = MIN(cyc, cyc1 - cyc0);
	}
	fprintf(stderr, "non-bitsliced computation: %lli cycles\n", cyc);

	vec a_t[8], b_t[8], c_t[8];

	cyc = -1LL; trans = -1LL;

	for (round = 0; round < ROUNDS; round++) {
		RDTSC_START(cyc0);
		transpose_to_vec(a_t, a);
		transpose_to_vec(b_t, b);
		RDTSC_STOP(cyc1);
		u64 tr = cyc1-cyc0;

		RDTSC_START(cyc0);
		formula_bs(c_t, a_t, b_t);
		RDTSC_STOP(cyc1);
		cyc = MIN(cyc, cyc1-cyc0);

		RDTSC_START(cyc0);
		transpose_to_raw(c, c_t);
		RDTSC_STOP(cyc1);
		tr += cyc1-cyc0;
		trans = MIN(trans, tr);
	}
	fprintf(stderr, "bitsliced computation: %lli cycles,  transposition: %lli cycles\n", cyc, trans);

	return 0;
}
