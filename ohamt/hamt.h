#ifndef _HAMT_H
#define _HAMT_H
/* Custom HAMT implementation.
 *   - hash is 128 bits
 *   - items and pointers are 40 bits (39 actually, last bit is reserved)
 *
 *
 * Unless differently stated:
 *   hash - a pointer to 16 bytes forming the 128 bit hash.
 *   item - a pointer to user data.
 */

/* You need:
#include <stdint.h>
 */

#ifndef __uint128_t_defined
# define __uint128_t_defined
typedef __uint128_t uint128_t;
#endif

#ifndef unlikely
# define unlikely(x)     __builtin_expect((x),0)
#endif

struct hamt_slot {
	uint64_t off:40;
};// __attribute__ ((packed));

struct hamt_node {
	uint64_t mask;
	struct hamt_slot slots[];
};

struct hamt_root {
	struct hamt_slot slot;

	void *(*mem_alloc)(size_t size);
	void (*mem_free)(void *ptr, size_t size);
        uint128_t *(*hash)(void *ud, void *item);
	void *hash_ud;
};


struct hamt_state{
	int level;
	uint128_t hash;
	struct hamt_slot *ptr[23]; /* ptr root->slot and 22 levels more  */
};


#define HAMT_ROOT(alloc, free, hash, hash_ud)			\
	(struct hamt_root) {{0}, alloc, free, hash, hash_ud}


/* Find the item that matches the hash.*/
void *hamt_search(struct hamt_root *root, uint128_t *hash);

/* Insert an item, unless there is one with the same hash already.
 * Return an item that matches the hash. */
void *hamt_insert(struct hamt_root *root, void *item);

/* Remove the item. */
void *hamt_delete(struct hamt_root *root, uint128_t *hash);


void *hamt_first(struct hamt_root *root, struct hamt_state *s);


#endif // _HAMT_H
