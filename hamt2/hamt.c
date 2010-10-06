/* Hash Array Mapped Trie */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hamt.h"

typedef unsigned int uint;

static inline uint popcount(u64 v)
{
	return __builtin_popcountll(v);
}

static inline uint bits_for_level(u64 hash, int level)
{
	assert(level <= 10);
	return (hash >> ((10-level)*6LL)) & 0x3FLL;
}

static inline int following_bit_slice(u64 mask, int bitoffset)
{
	assert(mask);
	// TODO: optimize
	int i;
	for (i=bitoffset+1; i < 64; i++) {
		if (mask & (1LL << i)) {
			return i;
		}
	}
	return -1;
}

static inline int first_bit_slice(u64 mask)
{
	assert(mask);
	int r = following_bit_slice(mask, -1);
	if (r == -1) {
		assert(0);
	}
	return r;
}

static inline int last_bit_slice(u64 mask, int bitoffset)
{
	return following_bit_slice(mask, bitoffset) == -1;
}

static inline uint slot_number(u64 mask, uint bitoffset)
{
	assert(bitoffset < 64);
	assert(mask & (1LL << bitoffset));

	u64 cleared = mask & ~((1LL << bitoffset) | ((1LL << bitoffset)-1));
	return popcount(cleared);
}

static int slot_exists(u64 mask, uint bitoffset)
{
	assert(bitoffset <= 0x3f);
	return (mask & (1LL << (u64)bitoffset)) != 0;
}


static struct hamt_node *__hamt_new_node(struct hamt_root *root,
					 u64 mask, int len);
static struct hamt_node *__hamt_new_node2(struct hamt_root *root,
					  void *item1, u64 item1_hash,
					  void *item2, u64 item2_hash,
					  int level);
static struct hamt_node *__hamt_add_slot(struct hamt_root *root,
					 struct hamt_node **node_ptr,
					 void *item, u64 item_hash,
					 int level);


static inline int is_leaf(struct hamt_node *node)
{
	return !!(((unsigned long)node) & 0x1);
}

static inline void *get_leaf(struct hamt_node *node)
{
	return (void*)(((unsigned long)node) & ~0x1);
}

static inline struct hamt_node *to_node(void *item)
{
	void *ptr = (void*)(((unsigned long)item) | 0x1);
	return (struct hamt_node *)ptr;
}


static void __hamt_search2(struct hamt_root *root,
			   u64 hash,
			   struct hamt_state *state)
{
	int level = 0;
	struct hamt_node **node_ptr = &root->node;
	state->node_ptrs[level] = node_ptr;
	while (*node_ptr) {
		if (is_leaf(*node_ptr)) {
			break;
		}
		int bit_slice = bits_for_level(hash, level);
		state->bit_slices[level] = bit_slice;
		if (slot_exists((*node_ptr)->mask, bit_slice)) {
			int slot = slot_number((*node_ptr)->mask, bit_slice);
			node_ptr = &(*node_ptr)->slots[slot];
			state->node_ptrs[level+1] = node_ptr;
			level++;
		} else {
			break;
		}
	}
	state->level = level;
}


static void __hamt_search(struct hamt_root *root,
			  u64 hash,
			  struct hamt_node ***node_pptr,
			  int *level_ptr)
{
	int level = 0;
	struct hamt_node **node_ptr = &root->node;
	while (*node_ptr) {
		if (is_leaf(*node_ptr)) {
			break;
		}
		int bit_slice = bits_for_level(hash, level);
		if (slot_exists((*node_ptr)->mask, bit_slice)) {
			int slot = slot_number((*node_ptr)->mask, bit_slice);
			node_ptr = &(*node_ptr)->slots[slot];
			level++;
		} else {
			break;
		}
	}
	*level_ptr = level;
	*node_pptr = node_ptr;
}

DLL_PUBLIC void *hamt_search(struct hamt_root *root, u64 hash)
{
	int level = 0;
	struct hamt_node **node_ptr;
	__hamt_search(root, hash, &node_ptr, &level);
	if (is_leaf(*node_ptr)) {
		return get_leaf(*node_ptr);
	}
	return NULL;
}

DLL_PUBLIC void *hamt_insert(struct hamt_root *root, void *item)
{
	struct hamt_node **node_ptr;
	u64 item_hash = root->hash(item);
	int level;
	__hamt_search(root, item_hash, &node_ptr, &level);
	if (!*node_ptr) {
		int bit_slice = bits_for_level(item_hash, 0);
		*node_ptr =  __hamt_new_node(root, 1LL << bit_slice, 1);
		(*node_ptr)->slots[0] = to_node(item);
	} else
	if (is_leaf(*node_ptr)) {
		void *leaf = get_leaf(*node_ptr);
		u64 leaf_hash = root->hash(leaf);
		if (leaf_hash == item_hash) {
			return leaf;
		}
		while (bits_for_level(leaf_hash, level) ==
		       bits_for_level(item_hash, level)) {
			int bit_slice = bits_for_level(item_hash, level) ;
			*node_ptr =  __hamt_new_node(root, 1LL << bit_slice, 1);
			node_ptr = &(*node_ptr)->slots[0];
			level++;
		}
		*node_ptr = __hamt_new_node2(root,
					     item, item_hash,
					     leaf, leaf_hash,
					     level);
	} else { // not leaf -> node.
		__hamt_add_slot(root, node_ptr, item, item_hash, level);
	}
	return item;
}


static struct hamt_node *__hamt_new_node(struct hamt_root *root, u64 mask, int len)
{
	assert(len <= 64);
	assert(popcount(mask) == len);
	int size = sizeof(struct hamt_node) + len * sizeof(void*);
	struct hamt_node *node = \
		(struct hamt_node *)root->mem_alloc(size);
	node->mask = mask;
	return node;
}

static void __hamt_free_node(struct hamt_root *root, struct hamt_node *node)
{
	int len = popcount(node->mask);
	int size = sizeof(struct hamt_node) + len * sizeof(void*);
	root->mem_free(size, node);
}


static struct hamt_node *__hamt_new_node2(struct hamt_root *root,
					  void *item1, u64 item1_hash,
					  void *item2, u64 item2_hash,
					  int level)
{
	u64 bit_slice1 = bits_for_level(item1_hash, level);
	u64 bit_slice2 = bits_for_level(item2_hash, level);

	struct hamt_node *node = __hamt_new_node(root, (1LL << bit_slice1) | (1LL << bit_slice2), 2);
	node->slots[ slot_number(node->mask, bit_slice1) ] = to_node(item1);
	node->slots[ slot_number(node->mask, bit_slice2) ] = to_node(item2);
	/* printf("3e -> %016lx %016lx %016lx\n", */
	/*        node->mask, */
	/*        node->slots[0], */
	/*        node->slots[1]); */
	return node;
}

static struct hamt_node *__hamt_add_slot(struct hamt_root *root,
					 struct hamt_node **node_ptr,
					 void *item, u64 item_hash,
					 int level)
{
	u64 bit_slice = bits_for_level(item_hash, level);

	struct hamt_node *old_node = *node_ptr;
	int old_size = popcount(old_node->mask);
	int new_size = old_size + 1;
	struct hamt_node *node = __hamt_new_node(root,
						 old_node->mask | (1LL << bit_slice),
						 new_size);
	*node_ptr = node;

	int slot = slot_number(node->mask, bit_slice);

	assert((old_node->mask & (1LL << bit_slice)) == 0);
	memcpy(&node->slots[0], &old_node->slots[0],
	       sizeof(void*)*slot);
	memcpy(&node->slots[slot+1], &old_node->slots[slot],
	       sizeof(void*)*(old_size-slot));
	node->slots[slot] = to_node(item);

	__hamt_free_node(root, old_node);
	return node;
}

static struct hamt_node *__hamt_del_node(struct hamt_root *root,
					 struct hamt_node *old_node,
					 int bit_slice)
{
	int slot = slot_number(old_node->mask, bit_slice);
	int old_size = popcount(old_node->mask);
	int new_size = old_size - 1;
	struct hamt_node *node = __hamt_new_node(root,
						 old_node->mask & ~(1LL << bit_slice),
						 new_size);
	assert((node->mask & (1LL << bit_slice)) == 0);
	memcpy(&node->slots[0], &old_node->slots[0],
	       sizeof(void*)*slot);
	memcpy(&node->slots[slot], &old_node->slots[slot+1],
	       sizeof(void*)*(old_size-slot-1));

	__hamt_free_node(root, old_node);
	return node;
}

static inline struct hamt_node *get_other_node(struct hamt_node *node,
					       struct hamt_node *a)
{
	assert(popcount(node->mask) == 2);
	if (node->slots[0] == a) {
		return node->slots[1];
	} else {
		return node->slots[0];
	}
}

DLL_PUBLIC void *hamt_delete(struct hamt_root *root, u64 hash)
{
	struct hamt_state state;
	__hamt_search2(root, hash, &state);

	int level = state.level;
	struct hamt_node *matching_node = *state.node_ptrs[level];
	if (matching_node == NULL
	    || !is_leaf(matching_node)
	    || root->hash(get_leaf(matching_node)) != hash) {
		return NULL;
	}
	level--;
	if (level < 0) {
		assert(root->node == matching_node);
		root->node = NULL;
		return get_leaf(matching_node);
	}

	struct hamt_node **node_ptr = state.node_ptrs[level];
	int bit_slice = bits_for_level(hash, level);
	assert(!is_leaf(*node_ptr));
	if (popcount((*node_ptr)->mask) == 2) {
		struct hamt_node *other_node =			\
			get_other_node(*node_ptr, matching_node);
		if(!is_leaf(other_node)) {
			u64 other_mask = (*node_ptr)->mask & (~(1LL << bit_slice));
			assert(popcount(other_mask) != 0);
			assert(popcount(other_mask) != 2);
			assert(popcount(other_mask) != 3);
			assert(popcount(other_mask) == 1);
			__hamt_free_node(root, *node_ptr);
			*node_ptr =  __hamt_new_node(root, other_mask, 1);
			(*node_ptr)->slots[0] = other_node;
			return get_leaf(matching_node);
		} else {
			while (1) {
				__hamt_free_node(root, *node_ptr);
				level--;
				if (level < 0) {
					root->node = other_node;
					return get_leaf(matching_node);
				}
				node_ptr = state.node_ptrs[level];
				bit_slice = bits_for_level(hash, level);
				if (popcount((*node_ptr)->mask) != 1) {
					break;
				}
			}
			int slot = slot_number((*node_ptr)->mask, bit_slice);
			(*node_ptr)->slots[slot] = other_node;
		}
	} else {
		*node_ptr = __hamt_del_node(root, *node_ptr, bit_slice);
	}
	return get_leaf(matching_node);
}


static void __hamt_down(struct hamt_state *state)
{
	int level = state->level;
	struct hamt_node **node_ptr = state->node_ptrs[level];
	if (*node_ptr == NULL) {
		return;
	}
	while (!is_leaf(*node_ptr)) {
		int bit_slice = first_bit_slice((*node_ptr)->mask);
		state->bit_slices[level] = bit_slice;

		int slot = slot_number((*node_ptr)->mask, bit_slice);
		node_ptr = &(*node_ptr)->slots[slot];
		state->node_ptrs[level+1] = node_ptr;
		level++;
	}
	state->level = level;
}

DLL_PUBLIC void *hamt_first(struct hamt_root *root, struct hamt_state *state)
{
	state->level = 0;
	state->node_ptrs[0] = &root->node;
	__hamt_down(state);
	struct hamt_node **node_ptr = state->node_ptrs[state->level];
	if (*node_ptr != NULL) {
		assert(is_leaf(*node_ptr));
		return get_leaf(*node_ptr);
	} else {
		return NULL;
	}
}


static int __hamt_up(struct hamt_state *state)
{
	int level = state->level;
	while (level >= 0) {
		struct hamt_node **node_ptr = state->node_ptrs[level];
		if (is_leaf(*node_ptr)) {
			level--;
		} else {
			int bit_slice = state->bit_slices[level];
			if (last_bit_slice((*node_ptr)->mask, bit_slice)) {
				level--;
			} else {
				break;
			}
		}
	}
	state->level = level;
	return level;
}

static void __hamt_next(struct hamt_state *state)
{
	int level = state->level;
	struct hamt_node **node_ptr = state->node_ptrs[level];
	int prev_bit_slice = state->bit_slices[level];
	int next_bit_slice =					\
		following_bit_slice((*node_ptr)->mask, prev_bit_slice);
	assert(next_bit_slice != -1);
	state->bit_slices[level] = next_bit_slice;
	int slot = slot_number((*node_ptr)->mask, next_bit_slice);
	state->node_ptrs[level+1] = &(*node_ptr)->slots[slot];
	state->level++;
}

DLL_PUBLIC void *hamt_next(struct hamt_root *root, struct hamt_state *state)
{
	if (__hamt_up(state) == -1) {
		return NULL;
	}
	__hamt_next(state);
	__hamt_down(state);
	struct hamt_node **node_ptr = state->node_ptrs[state->level];
	if (*node_ptr != NULL) {
		assert(is_leaf(*node_ptr));
		return get_leaf(*node_ptr);
	} else {
		return NULL;
	}
}


