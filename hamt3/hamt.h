/* Implementation of a Hash Array Mappe Tree.
 *   reference: http://hamt.sourceforge.net/
 * Features:
 *  - Optimized for low memory footprint. On 64 bit host it uses about 13 bytes
 *    per item.
 *  - Hash values doesn't have to stored on the item (to save memory).
 *    But you might be asked for hashes during a lookup - ie. CPU hungry.
 *  - Relies on a good implementation of malloc.
 *  - Doesn't support multiple keys with the same hash (no duplicates)
 *  - 64-bit hash; max tree depth is 11.
 *
 * For optimal performance use a good hash function.
 *
 * This structure should be used to hold pointers. It uses the last bit of
 * the pointer,  so make sure you have data aligned to 2 bytes and keep the
 * last bit cleared.
 */
#ifndef _HAMT_H
#define _HAMT_H

#ifndef DLL_PUBLIC
# define DLL_PUBLIC
#endif

#ifndef unlikely
# define unlikely(a) (a)
#endif

#define warn(...)							\
	do {								\
		char buf[512];						\
		snprintf(buf, sizeof(buf), __VA_ARGS__);		\
		printf("%s:%i %s()  %s\n", __FILE__, __LINE__, __func__, buf); \
	} while (0)

#include <sys/types.h>
typedef u_int64_t u64;


struct hamt_node;

struct hamt_node {
	u64 mask;
	struct hamt_node *slots[];
};

struct hamt_state{
	int level;
	u64 hash;
	struct hamt_node **ptr[12];
};


struct hamt_root {
	struct hamt_node *node;
	void *(*mem_alloc)(int size);
	void (*mem_free)(int size, void *ptr);
	u64 (*hash)(void *item);
};

struct hamt_state;

#define HAMT_ROOT(alloc, free, hash) \
	(struct hamt_root) {NULL, alloc, free, hash}


/* Finds an item that matches the hash.*/
DLL_PUBLIC void *hamt_search(struct hamt_root *root, u64 hash);

/* Inserts an item, unless there are items with the same hash.
 * Returns an item that matches the hash. */
DLL_PUBLIC void *hamt_insert(struct hamt_root *root, void *item);

/* Remove item with the hash. */
DLL_PUBLIC void *hamt_delete(struct hamt_root *root, u64 hash);

/* Returns the first item from the root (with the smallest hash). Initializes
 * the 'state' data structure. */
DLL_PUBLIC void *hamt_first(struct hamt_root *root, struct hamt_state *state);

/* Iterates over the tree. Returns NULL when there are no more items.
 * Between calls to hamt_first and hamt_next you can not modify the tree,
 * you can't delete nor add any items. */
DLL_PUBLIC void *hamt_next(struct hamt_root *root, struct hamt_state *state);

/* Remove item currently pointed by 'state' and move to 'state' to next item.
 * That's the only function that can be called during iteration. */
DLL_PUBLIC void *hamt_next_delete(struct hamt_root *root, struct hamt_state *state);

#endif //_HAMT_H
