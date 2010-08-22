#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hamt.h"

typedef unsigned int uint;


#define set_count(set) __builtin_popcountll(set)
#define __set_cleared_lower(set, item)				\
	((set) & ~((1ULL << (item)) | ((1ULL << (item))-1)))
#define set_next(set, item)					\
	(__builtin_ffsll(__set_cleared_lower((set), (item)))-1)
#define set_first(set) (__builtin_ffsll(set)-1)
#define set_last(set) (63 - __builtin_clzll(set))
#define set_contains(set, item) (((set) & (1ULL << (item))) != 0)
#define set_count_bigger(set, item)					\
	__builtin_popcountll(__set_cleared_lower((set), (item)))
#define set_slot_number(set, item)					\
	set_count_bigger(set, item)

#define set_add(set, item) ((set) | (1ULL << (item)))
#define set_del(set, item) ((set) & ~(1ULL << (item)))

#define slice_get(hash, level)				\
	(((hash) >> (10U-(level))*6U) & 0x3fULL)

#define slice_set(hash, slice, level)				\
	(((hash) & ~(0x3fULL << (10U-(level))*6U)) |		\
	 (((u64)(slice) & 0x3fULL) << (10U-(level))*6U))


//#define MASK (0x8000000000000000ULL)
#define MASK (0x1ULL)
static inline int is_leaf(struct hamt_node *node)
{
	return !!(((unsigned long)node) & MASK);
}

static inline void *to_leaf(struct hamt_node *node)
{
	return (void*)(((unsigned long)node) & ~MASK);
}

static inline struct hamt_node *to_node(void *item)
{
	void *ptr = (void*)(((unsigned long)item) | MASK);
	return (struct hamt_node *)ptr;
}


static inline void __hamt_search(struct hamt_root *root,
				 u64 hash,
				 struct hamt_state *s)
{
	int level = 0;
	struct hamt_node **ptr = &root->node;
	s->hash = 0;
	s->ptr[level] = ptr;
	while (!is_leaf(*ptr)) {
		int slice = slice_get(hash, level);
		if (set_contains((*ptr)->mask, slice)) {
			s->hash = slice_set(s->hash, slice, level);
			level++;

			int slot = set_count_bigger((*ptr)->mask, slice);
			ptr = &(*ptr)->slots[slot];
			s->ptr[level] = ptr;
		} else {
			break;
		}
	}
	s->level = level;
}

static inline void *__hamt_found_leaf(struct hamt_root *root,
				      u64 hash,
				      struct hamt_state *s)
{
	struct hamt_node **ptr = s->ptr[s->level];
	if (is_leaf(*ptr)) {
		void *leaf = to_leaf(*ptr);
		if (/*s->level == 11  || */root->hash(leaf) == hash) {
			return leaf;
		}
	}
	return NULL;
}

static struct hamt_node *__hamt_new_node(struct hamt_root *root, u64 mask, int len)
{
	int size = sizeof(struct hamt_node) + len * sizeof(void*);
	struct hamt_node *node = \
		(struct hamt_node *)root->mem_alloc(size);
	node->mask = mask;
	return node;
}

static void __hamt_free_node(struct hamt_root *root, struct hamt_node *node)
{
	int len = set_count(node->mask);
	int size = sizeof(struct hamt_node) + len * sizeof(void*);
	root->mem_free(size, node);
}

struct hamt_node *__hamt_new_node1(struct hamt_root *root,
				   int slice,
				   struct hamt_node *leaf)
{
	struct hamt_node *node = \
		__hamt_new_node(root, set_add(0LL, slice), 1);
	node->slots[0] = leaf;
	return node;
}

static struct hamt_node *__hamt_new_node2(struct hamt_root *root,
					  void *item1, int slice1,
					  void *item2, int slice2)
{
	struct hamt_node *node = \
		__hamt_new_node(root, set_add(set_add(0ULL, slice1), slice2), 2);
	node->slots[ set_slot_number(node->mask, slice1) ] = to_node(item1);
	node->slots[ set_slot_number(node->mask, slice2) ] = to_node(item2);
	return node;
}

static struct hamt_node *__hamt_add_slot(struct hamt_root *root,
					 struct hamt_node **node_ptr,
					 void *item, u64 item_hash,
					 int level)
{
	u64 slice = slice_get(item_hash, level);

	struct hamt_node *old_node = *node_ptr;
	int old_size = set_count(old_node->mask);
	int new_size = old_size + 1;
	struct hamt_node *node = __hamt_new_node(root,
						 set_add(old_node->mask, slice),
						 new_size);
	*node_ptr = node;

	int slot = set_slot_number(node->mask, slice);

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
					 int slice)
{
	int slot = set_slot_number(old_node->mask, slice);
	int old_size = set_count(old_node->mask);
	int new_size = old_size - 1;
	struct hamt_node *node = __hamt_new_node(root,
						 set_del(old_node->mask, slice),
						 new_size);
	memcpy(&node->slots[0], &old_node->slots[0],
	       sizeof(void*)*slot);
	memcpy(&node->slots[slot], &old_node->slots[slot+1],
	       sizeof(void*)*(old_size-slot-1));

	__hamt_free_node(root, old_node);
	return node;
}

DLL_PUBLIC void *hamt_insert(struct hamt_root *root, void *item)
{
	u64 item_hash = root->hash(item);
	assert(!is_leaf(item));
	assert(item);

	if (unlikely(root->node == NULL)) {
		root->node = to_node(item);
		return item;
	}

	struct hamt_state s;
	__hamt_search(root, item_hash, &s);
	int level = s.level;
	struct hamt_node **ptr = s.ptr[s.level];

	if (!is_leaf(*ptr)) {
		__hamt_add_slot(root, ptr, item, item_hash, level);
	} else {
		void *leaf = to_leaf(*ptr);
		u64 leaf_hash = root->hash(leaf);
		if (leaf_hash == item_hash) {
			return leaf;
		}

		int leaf_slice;
		int item_slice;
		while (1) {
			leaf_slice = slice_get(leaf_hash, level);
			item_slice = slice_get(item_hash, level);
			if (leaf_slice != item_slice) {
				break;
			}
			*ptr =  __hamt_new_node1(root, item_slice, NULL);
			ptr = &(*ptr)->slots[0];
			level++;
		}
		*ptr = __hamt_new_node2(root,
					item, item_slice,
					leaf, leaf_slice);
	}
	return item;
}


DLL_PUBLIC void *hamt_search(struct hamt_root *root, u64 hash)
{
	if (unlikely(root->node == NULL)) {
		return NULL;
	}

	struct hamt_state s;
	__hamt_search(root, hash, &s);
	return __hamt_found_leaf(root, hash, &s);
}


static inline struct hamt_node *__hamt_get_other_node(struct hamt_node *node,
						      struct hamt_node *a)
{
	if (node->slots[0] == a) {
		return node->slots[1];
	} else {
		return node->slots[0];
	}
}

DLL_PUBLIC void *hamt_delete(struct hamt_root *root, u64 hash)
{
	if (unlikely(root->node == NULL)) {
		return NULL;
	}

	struct hamt_state s;
	__hamt_search(root, hash, &s);
	int level = s.level;
	struct hamt_node *matching_node = *s.ptr[s.level];

	if (!__hamt_found_leaf(root, hash, &s)) {
		return NULL;
	}
	if (unlikely(level == 0)) {
		root->node = NULL;
		goto done;
	}
	level--;

	struct hamt_node **ptr = s.ptr[level];
	int slice = slice_get(hash, level);
	if (set_count((*ptr)->mask) != 2) {
		*ptr = __hamt_del_node(root, *ptr, slice);
		goto done;
	} else { // set_count == 2
		struct hamt_node *other_node =			\
			__hamt_get_other_node(*ptr, matching_node);
		if(!is_leaf(other_node)) {
			u64 other_slice = set_first(set_del((*ptr)->mask, slice));
			__hamt_free_node(root, *ptr);
			*ptr =  __hamt_new_node1(root, other_slice, other_node);
			goto done;
		} else {
			while (1) {
				__hamt_free_node(root, *ptr);
				if (unlikely(level == 0)) {
					root->node = other_node;
					goto done;
				}

				level--;
				ptr = s.ptr[level];
				if (set_count((*ptr)->mask) != 1) {
					break;
				}
			}
			slice = slice_get(hash, level);
			int slot = set_slot_number((*ptr)->mask, slice);
			(*ptr)->slots[slot] = other_node;
			goto done;
		}
	}
done:
	return to_leaf(matching_node);
}


static inline void __hamt_down(struct hamt_state *s)
{
	int level = s->level;
	struct hamt_node **ptr = s->ptr[level];
	if (*ptr == NULL) {
		return;
	}
	while (!is_leaf(*ptr)) {
		int slice = set_first((*ptr)->mask);
		s->hash = slice_set(s->hash, slice, level);
		s->ptr[level] = ptr;

		int slot = set_slot_number((*ptr)->mask, slice);
		ptr = &(*ptr)->slots[slot];
		level++;
	}
	s->ptr[level] = ptr;
	s->level = level;
}

DLL_PUBLIC void *hamt_first(struct hamt_root *root, struct hamt_state *s)
{
	s->level = 0;
	s->ptr[0] = &root->node;
	__hamt_down(s);

	struct hamt_node **ptr = s->ptr[s->level];
	if (*ptr != NULL) {
		return to_leaf(*ptr);
	}
	return NULL;
}


static int __hamt_up(struct hamt_state *s)
{
	int level = s->level;

	struct hamt_node **ptr = s->ptr[level];
	level--; // ptr must be a leaf.

	while (level >= 0) {
		ptr = s->ptr[level];
		int slice = slice_get(s->hash, level);;
		if (set_last((*ptr)->mask) == slice) {
			level--;
		} else {
			break;
		}
	}
	s->level = level;
	return level;
}

static void __hamt_next(struct hamt_state *s)
{
	int level = s->level;
	struct hamt_node **ptr = s->ptr[level];
	int prev_slice = slice_get(s->hash, level);
	int next_slice = set_next((*ptr)->mask, prev_slice);
	s->ptr[level] = ptr;
	s->hash = slice_set(s->hash, next_slice, level);

	level++;
	int slot = set_slot_number((*ptr)->mask, next_slice);
	s->ptr[level] = &(*ptr)->slots[slot];
	s->level = level;
}

DLL_PUBLIC void *hamt_next(struct hamt_root *root, struct hamt_state *s)
{
	if (__hamt_up(s) == -1) {
		return NULL;
	}
	__hamt_next(s);
	__hamt_down(s);
	struct hamt_node **ptr = s->ptr[s->level];
	return to_leaf(*ptr);
}
