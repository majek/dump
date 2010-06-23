#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>

#include "hashes.h"

#define HAPROXY_BACKENDS 4

unsigned haproxy_uri_hash(unsigned char *uri, int uri_len){

  unsigned long hash = 0;
  int c;

  while (uri_len--) {
    c = *uri++;
    if (c == '?')
      break;
    hash = c + (hash << 6) + (hash << 16) - hash;
  }

  return hash%HAPROXY_BACKENDS; /* I assume 4 active backends */
} /* end haproxy_hash() */

unsigned sax_hash ( unsigned char *key, int len )
{
  unsigned char *p = key;
  unsigned h = 0;
  int i;

  for ( i = 0; i < len; i++ )
    h ^= ( h << 5 ) + ( h >> 2 ) + p[i];

  return h;
}


unsigned int hashpjw(unsigned char *key, int size) {

  const char         *ptr;
  unsigned int        val;
  /*********************************************************************
   *                                                                    *
   *  Hash the key by performing a number of bit operations on it.      *
   *                                                                    *
   *********************************************************************/

  val = 0;
  ptr = key;
  
  while (*ptr != '\0') {

    int tmp;
    
    val = (val << 4) + (*ptr);
    
    if((tmp = (val & 0xf0000000))) {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
    ptr++;
  }/* end while */

  return val;
}/* end hashpjw */


unsigned int
hash_djbx33(
    register unsigned char *key,
    register int len)
{
    register unsigned long hash = 5381;

    /* the hash unrolled eight times */
    for (; len >= 8; len -= 8) {
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
    }
    switch (len) {
        case 7: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 6: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 5: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 4: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 3: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 2: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 1: hash = ((hash << 5) + hash) + *key++; break;
        default: /* case 0: */ break;
    }
    return hash;
}

typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;   /* unsigned 1-byte quantities */

unsigned int bernstein(ub1 *key, int len){
  ub4 hash = 4;
  ub4 i;
  for (i=0; i<len; ++i) hash = 33*hash + key[i];
  return hash;
}

/*
 * http://www.azillionmonkeys.com/qed/hash.html
 */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const u_int16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((u_int32_t)(((const u_int8_t *)(d))[1])) << 8)\
                       +(u_int32_t)(((const u_int8_t *)(d))[0]) )
#endif

u_int32_t SuperFastHash (unsigned char * data, int len) {
u_int32_t hash = len, tmp;
int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (u_int16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (u_int16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

/*
 * This variant is about 15% faster.
 */
u_int32_t SuperFastHash2 (unsigned char * data, int len) {
u_int32_t hash = len, tmp;
int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
	register u_int32_t next;
	next   = get16bits(data+2);
        hash  += get16bits(data);
        tmp    = (next << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (u_int16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (u_int16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

/*
 * 32 bit FNV-0 hash type
 */
typedef unsigned int Fnv32_t;

/*
 * fnv_32a_str - perform a 32 bit Fowler/Noll/Vo FNV-1a hash on a string
 *
 * input:
 *	str	- string to hash
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	32 bit hash as a static hash type
 *
 * NOTE: To use the recommended 32 bit FNV-1a hash, use FNV1_32A_INIT as the
 *  	 hval arg on the first call to either fnv_32a_buf() or fnv_32a_str().
 */
unsigned
fnv_32a_str(unsigned char *str,  int len)
{
      Fnv32_t hval=1;
    unsigned char *s = (unsigned char *)str;	/* unsigned string */

    /*
     * FNV-1a hash each octet in the buffer
     */
    while (*s) {

	/* xor the bottom with the current octet */
	hval ^= (Fnv32_t)*s++;

/* #define NO_FNV_GCC_OPTIMIZATION */
	/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
	/*
	 * 32 bit magic FNV-1a prime
	 */
#define FNV_32_PRIME ((Fnv32_t)0x01000193)
	hval *= FNV_32_PRIME;
#else
	hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
    }

    /* return our new hash value */
    return hval;
}

/*
 * from lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 */

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose 
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/*
--------------------------------------------------------------------
 This works on all machines.  To be useful, it requires
 -- that the key be an array of uint32_t's, and
 -- that the length be the number of uint32_t's in the key

 The function hashword() is identical to hashlittle() on little-endian
 machines, and identical to hashbig() on big-endian machines,
 except that the length has to be measured in uint32_ts rather than in
 bytes.  hashlittle() is more complicated than hashword() only because
 hashlittle() has to dance around fitting the key bytes into registers.
--------------------------------------------------------------------
*/
u_int32_t hashword(
unsigned char *kb,                   /* the key, an array of uint32_t values */
int          length               /* the length of the key, in uint32_ts */
)         /* the previous hash, or an arbitrary value */
{
	u_int32_t        initval = 4;
  u_int32_t a,b,c;

  length = length/sizeof(u_int32_t);
  u_int32_t *k = (u_int32_t)*kb;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + (((u_int32_t)length)<<2) + initval;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch(length)                     /* all the case statements fall through */
  { 
  case 3 : c+=k[2];
  case 2 : b+=k[1];
  case 1 : a+=k[0];
    final(a,b,c);
  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  return c;
}

/* from K&R book site 139 */
#define HASHSIZE 101

unsigned kr_hash(unsigned char *s, int len){
  unsigned hashval;
  
  for(hashval = 0; *s != '\0';s++)
    hashval = *s + 31 * hashval;

  return hashval % HASHSIZE;
    
} /* end kr_hash() */

unsigned fnv_hash ( unsigned char *key, int len )
{
  unsigned char *p = key;
  unsigned h = 2166136261;
  int i;

  for ( i = 0; i < len; i++ )
    h = ( h * 16777619 ) ^ p[i];

  return h;
}

unsigned oat_hash ( unsigned char *key, int len )
{
  unsigned char *p = key;
  unsigned h = 0;
  int i;

  for ( i = 0; i < len; i++ ) {
    h += p[i];
    h += ( h << 10 );
    h ^= ( h >> 6 );
  }

  h += ( h << 3 );
  h ^= ( h >> 11 );
  h += ( h << 15 );

  return h;
}

unsigned wt_hash ( unsigned char *key, int len )
{
  unsigned char *p = key;
  unsigned h = 0x783c965aUL;
  unsigned step = 16;

  for (; len > 0; len--) {
      h ^= *p * 9;
      p++;
      h = (h << step) | (h >> (32-step));
      step ^= h;
      step &= 0x1F;
  }

  return h;
}


unsigned sha1 (unsigned char *key, int len)
{
	static SHA_CTX sha;
	static unsigned char digest[20];

	SHAInit(&sha);
	SHAUpdate(&sha, key, len);
	SHAFinal(digest, &sha);

	unsigned *a = (unsigned*)digest;
	return a[0] ^ a[1] ^ a[2] ^ a[3];
}
