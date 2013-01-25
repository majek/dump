/* <MIT License>
 Copyright (c) 2013  Marek Majkowski <marek@popcount.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 </MIT License>


 Solution based on code from:
    Samuel Neves (supercop/crypto_auth/siphash24/little)
    djb (supercop/crypto_auth/siphash24/little2)
*/

#include <stdint.h>

#if defined(__APPLE__)
#  include <libkern/OSByteOrder.h>
#  define le64toh(x) OSSwapLittleToHostInt64(x)
#else
#  include <endian.h>
#endif


typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

#define ROTATE(x,b) (u64)( ((x) << (b)) | ( (x) >> (64 - (b))) )

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


uint64_t siphash24(const char *src, unsigned long inlen, const char k[16]) {
	u64 k0 = le64toh(*(u64 *)(k));
	u64 k1 = le64toh(*(u64 *)(k + 8));
	u64 b = (u64)inlen << 56;
	u64 *in = (u64*)src;

	u64 v0 = k0 ^ 0x736f6d6570736575ULL;
	u64 v1 = k1 ^ 0x646f72616e646f6dULL;
	u64 v2 = k0 ^ 0x6c7967656e657261ULL;
	u64 v3 = k1 ^ 0x7465646279746573ULL;

	while (inlen >= 8) {
		u64 mi = le64toh(*in);
		in += 1; inlen -= 8;
		v3 ^= mi;
		DOUBLE_ROUND(v0,v1,v2,v3);
		v0 ^= mi;
	}

	u64 t = 0; u8 *pt = (u8 *)&t; u8 *m = (u8 *)in;
	switch (inlen) {
	case 7: pt[6] = m[6];
	case 6: pt[5] = m[5];
	case 5: pt[4] = m[4];
	case 4: *((u32*)&pt[0]) = *((u32*)&m[0]); break;
	case 3: pt[2] = m[2];
	case 2: pt[1] = m[1];
	case 1: pt[0] = m[0];
	}
	b |= le64toh(t);

	v3 ^= b;
	DOUBLE_ROUND(v0,v1,v2,v3);
	v0 ^= b; v2 ^= 0xff;
	DOUBLE_ROUND(v0,v1,v2,v3);
	DOUBLE_ROUND(v0,v1,v2,v3);
	return (v0 ^ v1) ^ (v2 ^ v3);
}
