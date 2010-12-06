#ifndef _BITMAP_H
#define _BITMAP_H
/*
 * Implementation of a bitmap (bitarray, bitfield or whatever you call it).
 *
 * Requires the following includes:

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

 */

#ifndef unlikely
# define unlikely(x) (x)
#endif

#ifndef __uint64__ffs
# define __uint64__ffs(i)			\
 	((int)__builtin_ffsll(i)-1)
#endif

struct bitmap {
	int map_cnt;
	uint64_t map[];
};

static inline struct bitmap *bitmap_new(int items) {
	int i = (items/64)+1;
	struct bitmap *root = (struct bitmap *)	\
		malloc(sizeof(struct bitmap) +
		       sizeof(uint64_t)*i);

	root->map_cnt = i;
	memset(root->map, 0, sizeof(uint64_t)*i);
	return root;
}

static inline void bitmap_free(struct bitmap *root) {
	free(root);
}

static inline void bitmap_set(struct bitmap *root, unsigned int pos) {
	root->map[pos/64] |= 1ULL << (pos % 64);
}

static inline void bitmap_clear(struct bitmap *root, unsigned int pos) {
	root->map[pos/64] &= ~(1ULL << (pos % 64));
}

static inline int bitmap_get(struct bitmap *root, unsigned int pos) {
	return !!(root->map[pos/64] & (1ULL << (pos % 64)));
}

static inline int bitmap_next(struct bitmap *root, int prev_pos) {
	int i;
	unsigned int pos = prev_pos+1;
	uint64_t r = root->map[pos/64] >> (pos % 64);
	int ffs = __uint64__ffs(r);
	if (unlikely(ffs >= 0)) {
		return pos + ffs;
	}

	for (i=pos/64+1; i < root->map_cnt; i++) {
		int ffs = __uint64__ffs(root->map[i]);
		if (unlikely(ffs >= 0)) {
			return i*64 + ffs;
		}
	}
	return -1;
}


#endif // _BITMAP_H
