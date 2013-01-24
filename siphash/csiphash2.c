#include <stdio.h>
#include <stdint.h>
#include <string.h>

#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define le64toh(x) OSSwapLittleToHostInt64(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#elif defined(__linux__)
#include <endian.h>
#else
# error no le64toh
#endif


typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;


#define ROTATE(x,b) (u64)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define HALF_ROUND(a,b,c,d,s,t)			\
	a += b; c += d;				\
	b = ROTATE(b, s) ^ a;	      		\
	d = ROTATE(d, t) ^ c;			\
	a = ROTATE(a,32);

#define DOUBLEROUND(v0,v1,v2,v3)			\
	HALF_ROUND(v0,v1,v2,v3,13,16);			\
	HALF_ROUND(v2,v1,v0,v3,17,21);			\
	HALF_ROUND(v0,v1,v2,v3,13,16);			\
	HALF_ROUND(v2,v1,v0,v3,17,21);


#define PRINTV							\
	printf( "(%3d) v0 %016llx\n", ( int )inlen, v0 );	\
	printf( "(%3d) v1 %016llx\n", ( int )inlen, v1 );	\
	printf( "(%3d) v2 %016llx\n", ( int )inlen, v2 );	\
	printf( "(%3d) v3 %016llx\n", ( int )inlen, v3 );



uint64_t siphash(const char *in, unsigned long inlen, const char k[16]) {
	u64 k0 = le64toh(*(u64 *) (k));
	u64 k1 = le64toh(*(u64 *) (k + 8));
	u64 b = (u64)inlen << 56;
	u64 *ineight = (u64*)in;

	u64 v0 = k0 ^ 0x736f6d6570736575ULL;
	u64 v1 = k1 ^ 0x646f72616e646f6dULL;
	u64 v2 = k0 ^ 0x6c7967656e657261ULL;
	u64 v3 = k1 ^ 0x7465646279746573ULL;

	while (inlen >= 8) {
		u64 mi = le64toh(*ineight);
#ifdef DEBUG
		PRINTV;
		printf( "(%3d) compress %016llx\n", ( int )inlen, mi);
#endif
		v3 ^= mi;
		DOUBLEROUND(v0,v1,v2,v3);
		v0 ^= mi;
		ineight += 1; inlen -= 8;
	}

	u8 *m = (u8 *)ineight;

	switch( inlen ) {
	case 7: b |= ((u64)m[6]) << 48;
	case 6: b |= ((u64)m[5]) << 40;
	case 5: b |= ((u64)m[4]) << 32;
	case 4: b |= ((u64)le32toh(*(u32 *)m)); break;
	case 3: b |= ((u64)m[2]) << 16;
	case 2: b |= ((u64)m[1]) <<  8;
	case 1: b |= ((u64)m[0]); break;
	case 0: break;
	}

#ifdef DEBUG
	PRINTV;
	printf( "(%3d) padding	 %016llx\n", ( int )inlen, b );
#endif
	v3 ^= b;
	DOUBLEROUND(v0,v1,v2,v3);
	v0 ^= b;
#ifdef DEBUG
	PRINTV;
#endif
	v2 ^= 0xff;
	DOUBLEROUND(v0,v1,v2,v3);
	DOUBLEROUND(v0,v1,v2,v3);
	return (v0 ^ v1) ^ (v2 ^ v3);
}
