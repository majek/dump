#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hamt.h"

static unsigned long
hash_djbx33(
    register unsigned char *key,
    register size_t len)
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


#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const u_int16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((u_int32_t)(((const u_int8_t *)(d))[1])) << 8)\
                       +(u_int32_t)(((const u_int8_t *)(d))[0]) )
#endif

u_int32_t SuperFastHash2 (const char * data, int len) {
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

#define HASHSIZE 101

unsigned kr_hash(char *s){
  unsigned hashval;

  for(hashval = 0; *s != '\0';s++)
    hashval = *s + 31 * hashval;

  return hashval % HASHSIZE;

} /* end kr_hash() */

unsigned fnv_hash ( void *key, int len )
{
  unsigned char *p = key;
  unsigned h = 2166136261;
  int i;

  for ( i = 0; i < len; i++ )
    h = ( h * 16777619 ) ^ p[i];

  return h;
}

unsigned oat_hash ( void *key, int len )
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

unsigned sax_hash ( void *key, int len )
{
  unsigned char *p = key;
  unsigned h = 0;
  int i;

  for ( i = 0; i < len; i++ )
    h ^= ( h << 5 ) + ( h >> 2 ) + p[i];

  return h;
}

typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;   /* unsigned 1-byte quantities */

ub4 bernstein(ub1 *key, ub4 len, ub4 level){
  ub4 hash = level;
  ub4 i;
  for (i=0; i<len; ++i) hash = 33*hash + key[i];
  return hash;
}


int strip(char *buf)
{
	int len = strlen(buf) + 1;
	while((len-2 > 0) && (buf[len-2] == '\n')) {
		buf[len-2] = '\0';
		len--;
	}
	return len;
}

struct item {
	struct hamt_item hamt;
	int value;
};

int main()
{
	int items = 0;
	struct hamt_root root = HAMT_ROOT;

	while (1) {
		char buf[256];
		char *r = fgets(buf, sizeof(buf), stdin);
		if (r == NULL) {
			break;
		}
		int len = strip(buf);

		u64 hash = //bernstein((unsigned char*)buf, len, 4);
			sax_hash(buf, len);
			//SuperFastHash2(buf, len);
			//hash_djbx33((unsigned char*)buf, len);
		//printf("%lu\n", hash);

		struct item *item = \
			(struct item *)malloc(sizeof(struct item));
		item->hamt = HAMT_ITEM(hash);
		item->value = hash;
		struct hamt_item *n2 = hamt_insert(&root, &item->hamt);
		if (n2 == &item->hamt) {
			items ++;
		} else {
			free(item);
		}
	}

	/* for (i=start_item; i<items; i++) { */
	/* 	struct hamt_item *entry = hamt_search(&root, i); */
	/* 	if (entry == NULL) { */
	/* 		printf("%i -> %p\n", i, entry); */
	/* 	} else { */
	/* 		struct item *item = hamt_entry(entry, struct item, hamt); */
	/* 		printf("%i -> %p %i\n", i, entry, item->value); */
	/* 	} */
	/* } */
	int i, sum=0;
	for (i=0; i<65; i++) {
		int used = (int)sizeof(void*)*i* (root.mem_histogram[i]);
		printf("[%i] -> %i (%i bytes)\n",
		       i,
		       root.mem_histogram[i],
		       used);
		sum += used;
	}

	printf("%i/%i = %.3f bytes/item\n", sum, items, sum/(float)items);
	return 0;
}
