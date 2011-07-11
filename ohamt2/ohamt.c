#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#include "config.h"
#include "queue.h"
#include "list.h"
#include "ohamt.h"
#include "ohamt_mem.h"

#define ITEM_MASK (0xFFFFFF0000000001ULL) /* 40 bits */
#define LEAF_MASK (0x1ULL)

/**************************************************************************/

void INIT_OHAMT_ROOT(struct ohamt_root *root, hash_fun hash, void *hash_ud)
{
	*root = (struct ohamt_root) {
		.mem = mem_new(),
		.slot = {{0,0,0,0,0}},
		.hash = hash,
		.hash_ud = hash_ud};
}

void FREE_OHAMT_ROOT(struct ohamt_root *root)
{
	ohamt_erase(root);
	mem_free(root->mem);
}

/**************************************************************************/


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
#define set_slot_number(set, item)              \
	set_count_bigger(set, item)

#define set_add(set, item) ((set) | (1ULL << (item)))
#define set_del(set, item) ((set) & ~(1ULL << (item)))

static inline uint128_t slice_get(uint128_t hash, int level)
{
	return ((hash) >> ((level)*6)) & 0x3fULL;
}

static inline uint128_t slice_set(uint128_t hash, int slice, int level)
{
	uint128_t __s = (slice) & 0x3fULL;
	uint128_t __c = 0x3fULL;
	__s = __s << ((level)*6);
	__c = __c << ((level)*6);
	return ((hash) & ~__c) | __s;
}

/* static inline int slot_is_leaf(struct ohamt_slot *slot) */
/* { */
/* 	return !!(slot->off & LEAF_MASK); */
/* } */

/* static inline uint64_t slot_to_leaf(struct ohamt_slot *slot) */
/* { */
/* 	return slot->off & ~LEAF_MASK; */
/* } */

/* static inline struct ohamt_slot leaf_to_slot(uint64_t item) */
/* { */
/*         return (struct ohamt_slot){(uint64_t)item | LEAF_MASK}; */
/* } */

/* static inline struct ohamt_node *slot_to_node(struct ohamt_slot slot) */
/* { */
/* 	return (struct ohamt_node *)(uint64_t)slot.off; */
/* } */

/* static inline struct ohamt_slot ntos(struct ohamt_node *node) */
/* { */
/* 	return (struct ohamt_slot){(uint64_t)node}; */
/* } */

/**************************************************************************/

static inline int ohamt_is_empty(struct ohamt_root *root)
{
	struct ohamt_slot null_slot = {{0,0,0,0,0}};
	return memcmp(&root->slot, &null_slot, sizeof(struct ohamt_slot)) == 0;
}

static inline uint64_t __ohamt_search(struct ohamt_root *root,
				      uint128_t hash,
				      struct ohamt_state *s)
{
        if (!slot_is_leaf(*s->ptr[s->level])) {
                struct ohamt_node *node = slot_to_node(root->mem, *s->ptr[s->level]);

		int slice = slice_get(hash, s->level);
		if (set_contains(node->mask, slice)) {
                        s->hash = slice_set(s->hash, slice, s->level);
			int slot = set_slot_number(node->mask, slice);
                        s->ptr[s->level + 1] = &node->slots[slot];
                        s->level += 1;
                        return __ohamt_search(root, hash, s);
                }
                return OHAMT_NOT_FOUND;
        } else {
                uint64_t item = slot_to_leaf(*s->ptr[s->level]);
                if (root->hash(root->hash_ud, item) == hash) {
                        return item;
                } else {
                        return OHAMT_NOT_FOUND;
                }
        }
}

uint64_t ohamt_search(struct ohamt_root *root, uint128_t hash)
{
	if (unlikely(ohamt_is_empty(root))) {
		return OHAMT_NOT_FOUND;
	}

        struct ohamt_state s;
        s.level = 0;
        s.ptr[0] = &root->slot;

	return __ohamt_search(root, hash, &s);
}

/**************************************************************************/
/* static struct ohamt_slot __ohamt_new_slot(struct ohamt_root *root, int len) */
/* { */
/* 	return slot_alloc(root->mem, len); */
/* } */

/* static struct ohamt_node *__ohamt_new_node(struct ohamt_root *root, */
/* 					   uint64_t mask, int len) */
/* { */
/* 	int size = sizeof(struct ohamt_node) + len * sizeof(struct ohamt_slot); */
/* 	struct ohamt_node *node = \ */
/* 		(struct ohamt_node *)mem_alloc(root->mem, size); */
/* 	assert(((unsigned long)node & ITEM_MASK) == 0); */
/* 	node->mask = mask; */
/* 	return node; */
/* } */

/* static void __ohamt_free_node(struct ohamt_root *root, struct ohamt_node *node) */
/* { */
/* 	int len = set_count(node->mask); */
/* 	int size = sizeof(struct ohamt_node) + len * sizeof(struct ohamt_slot); */
/* 	mem_free(root->mem, node, size); */
/* } */

static void __ohamt_free_slot(struct ohamt_root *root, struct ohamt_slot slot)
{
	slot_free(root->mem, slot);
}

static struct ohamt_slot __ohamt_new_slot1(struct ohamt_root *root, int slice,
					   struct ohamt_slot slot1)
{
	struct ohamt_slot slot = slot_alloc(root->mem, 1);
	struct ohamt_node *node = slot_to_node(root->mem, slot);
	node->mask = set_add(0ULL, slice);
	node->slots[0] = slot1;
	return slot;
}

static struct ohamt_slot __ohamt_new_slot2(struct ohamt_root *root,
					   uint64_t item1, int slice1,
					   uint64_t item2, int slice2)
{
	struct ohamt_slot slot = slot_alloc(root->mem, 2);
	struct ohamt_node *node = slot_to_node(root->mem, slot);
	node->mask = set_add(set_add(0ULL, slice1), slice2);
	node->slots[set_slot_number(node->mask, slice1)] = leaf_to_slot(item1);
	node->slots[set_slot_number(node->mask, slice2)] = leaf_to_slot(item2);
	return slot;
}

static struct ohamt_slot __ohamt_add_slot(struct ohamt_root *root,
					  struct ohamt_slot *slot_ptr,
					  uint64_t item, uint128_t item_hash,
					  int level)
{
	uint64_t slice = slice_get(item_hash, level);

	struct ohamt_slot old_slot = *slot_ptr;
	struct ohamt_node *old_node = slot_to_node(root->mem, *slot_ptr);
	int old_size = set_count(old_node->mask);
	int new_size = old_size + 1;

	struct ohamt_slot slot_a = slot_alloc(root->mem, new_size);
	struct ohamt_node *node = slot_to_node(root->mem, slot_a);
	node->mask = set_add(old_node->mask, slice);
	*slot_ptr = slot_a;

	int slot = set_slot_number(node->mask, slice);

	memcpy(&node->slots[0], &old_node->slots[0],
	       sizeof(struct ohamt_slot)*slot);
	memcpy(&node->slots[slot+1], &old_node->slots[slot],
	       sizeof(struct ohamt_slot)*(old_size-slot));
	node->slots[slot] = leaf_to_slot(item);

	__ohamt_free_slot(root, old_slot);
	return slot_a;
}

/**************************************************************************/

static void __ohamt_insert_leaf(struct ohamt_root *root,
			       struct ohamt_state *s,
			       uint128_t item_hash, uint64_t item,
			       uint128_t leaf_hash, uint64_t leaf)
{
        int leaf_slice = slice_get(leaf_hash, s->level);
        int item_slice = slice_get(item_hash, s->level);
        if (leaf_slice != item_slice) {
		*s->ptr[s->level] = __ohamt_new_slot2(root,
						      item, item_slice,
						      leaf, leaf_slice);
        } else {
		struct ohamt_slot slot = __ohamt_new_slot1(root,
							   item_slice,
							   leaf_to_slot(OHAMT_NOT_FOUND));
		struct ohamt_node *node =  slot_to_node(root->mem, slot);
                *s->ptr[s->level] = slot;
                s->ptr[s->level + 1] = &node->slots[0];
                s->level += 1;
                __ohamt_insert_leaf(root, s, item_hash, item, leaf_hash, leaf);
        }
}

uint64_t ohamt_insert(struct ohamt_root *root, uint64_t item)
{
	uint128_t item_hash = root->hash(root->hash_ud, item);
	assert(((unsigned long)item & ITEM_MASK) == 0);
	assert(item);

	if (unlikely(ohamt_is_empty(root))) {
		root->slot = leaf_to_slot(item);
		return item;
	}

	struct ohamt_state s;
        s.level = 0;
        s.ptr[0] = &root->slot;
        uint64_t found_item = __ohamt_search(root, item_hash, &s);

        if (unlikely(found_item != OHAMT_NOT_FOUND)) {
                return found_item;
        }

        if (!slot_is_leaf(*s.ptr[s.level])) {
                __ohamt_add_slot(root, s.ptr[s.level], item, item_hash, s.level);
        } else {
                uint64_t leaf = slot_to_leaf(*s.ptr[s.level]);
                uint128_t leaf_hash = root->hash(root->hash_ud, leaf);

                __ohamt_insert_leaf(root, &s, item_hash, item, leaf_hash, leaf);
        }
	return item;
}

uint64_t ohamt_replace(struct ohamt_root *root, uint64_t new_item)
{
	uint128_t item_hash = root->hash(root->hash_ud, new_item);
	assert(((unsigned long)new_item & ITEM_MASK) == 0);
	assert(new_item);

	if (unlikely(ohamt_is_empty(root))) {
		return OHAMT_NOT_FOUND;
	}

	struct ohamt_state s;
        s.level = 0;
        s.ptr[0] = &root->slot;

	uint64_t found_item = __ohamt_search(root, item_hash, &s);
	if (unlikely(found_item == OHAMT_NOT_FOUND)) {
		return OHAMT_NOT_FOUND;
	}

	struct ohamt_slot *found_slot = s.ptr[s.level];
	*found_slot = leaf_to_slot(new_item);

	return found_item;
}


/**************************************************************************/

static struct ohamt_slot __ohamt_del_slot(struct ohamt_root *root,
					  struct ohamt_slot *old_slot,
					  int slice)
{
	struct ohamt_node *old_node = slot_to_node(root->mem, *old_slot);

	int slot_no = set_slot_number(old_node->mask, slice);
	int old_size = set_count(old_node->mask);
	int new_size = old_size - 1;

	struct ohamt_slot slot = slot_alloc(root->mem, new_size);
	struct ohamt_node *node = slot_to_node(root->mem, slot);
	node->mask = set_del(old_node->mask, slice);


	memcpy(&node->slots[0], &old_node->slots[0],
	       sizeof(struct ohamt_slot)*slot_no);
	memcpy(&node->slots[slot_no], &old_node->slots[slot_no+1],
	       sizeof(struct ohamt_slot)*(old_size-slot_no-1));

	__ohamt_free_slot(root, *old_slot);
	return slot;
}

static struct ohamt_slot __ohamt_get_other_slot(struct ohamt_node *node,
					       struct ohamt_slot *a)
{
	if (memcmp(&node->slots[0], a, sizeof(struct ohamt_slot)) == 0) {
		return node->slots[1];
	} else {
		return node->slots[0];
	}
}

static void __ohamt_delete(struct ohamt_root *root, uint128_t hash,
			   struct ohamt_state *s)
{
	if (unlikely(s->level == 0)) {
		memset(&root->slot, 0, sizeof(struct ohamt_slot));
		goto done;
	}
	struct ohamt_slot *found_slot = s->ptr[s->level];
	s->level--;

	struct ohamt_node *node = slot_to_node(root->mem, *s->ptr[s->level]);

	int slice = slice_get(hash, s->level);
	if (set_count(node->mask) != 2) {
		*s->ptr[s->level] = __ohamt_del_slot(root, s->ptr[s->level], slice);
		goto done;
	} else { // set_count == 2
		struct ohamt_slot other_slot = \
			__ohamt_get_other_slot(node, found_slot);
		if(!slot_is_leaf(other_slot)) {
			uint64_t other_slice = set_first(set_del(node->mask, slice));
			__ohamt_free_slot(root, *s->ptr[s->level]);
			*s->ptr[s->level] = __ohamt_new_slot1(root, other_slice, other_slot);
			goto done;
		} else {
			while (1) {
				__ohamt_free_slot(root, *s->ptr[s->level]);
				if (unlikely(s->level == 0)) {
					root->slot = other_slot;
					goto done;
				}

				s->level--;
				node = slot_to_node(root->mem, *s->ptr[s->level]);
				if (set_count(node->mask) != 1) {
					break;
				}
			}
			slice = slice_get(hash, s->level);
			int slot = set_slot_number(node->mask, slice);
			node->slots[slot] = other_slot;
			goto done;
		}
	}
done:
	return;
}

uint64_t ohamt_delete(struct ohamt_root *root, uint128_t hash)
{
	if (unlikely(ohamt_is_empty(root))) {
		return OHAMT_NOT_FOUND;
	}

	struct ohamt_state s;
        s.level = 0;
        s.ptr[0] = &root->slot;

	uint64_t found_item = __ohamt_search(root, hash, &s);
	if (unlikely(found_item == OHAMT_NOT_FOUND)) {
		return OHAMT_NOT_FOUND;
	}

	__ohamt_delete(root, hash, &s);
	return found_item;
}

/**************************************************************************/

static uint64_t __ohamt_down(struct ohamt_root *root, struct ohamt_state *s)
{
	if (!slot_is_leaf(*s->ptr[s->level])) {
                struct ohamt_node *node = slot_to_node(root->mem, *s->ptr[s->level]);
		int slice = set_first(node->mask);
		s->hash = slice_set(s->hash, slice, s->level);

		int slot = set_slot_number(node->mask, slice);
		s->ptr[s->level + 1] = &node->slots[slot];
		s->level += 1;
		return __ohamt_down(root, s);
	} else {
                return slot_to_leaf(*s->ptr[s->level]);
	}
}

uint64_t ohamt_first(struct ohamt_root *root, struct ohamt_state *s)
{
	if (unlikely(ohamt_is_empty(root))) {
		return OHAMT_NOT_FOUND;
	}

	s->level = 0;
	s->ptr[0] = &root->slot;
	return __ohamt_down(root, s);
}

/**************************************************************************/

void ohamt_delete_all(struct ohamt_root *root,
		      void (*fun)(void *ud, uint64_t item), void *ud)
{
	struct ohamt_state s;
	while (slot_to_node(root->mem, root->slot) != NULL) {
		s.level = 0;
		s.ptr[0] = &root->slot;
		uint64_t found_item = __ohamt_down(root, &s);
		uint128_t hash = root->hash(root->hash_ud, found_item);
		__ohamt_delete(root, hash, &s);

		fun(ud, found_item);
	}
}

/**************************************************************************/

static inline void __ohamt_erase(struct ohamt_root *root,
				 struct ohamt_slot *slot)
{
	if (!slot_is_leaf(*slot)) {
		struct ohamt_node *node = slot_to_node(root->mem, *slot);
		int i;
		for (i=0; i < set_count(node->mask); i++) {
			__ohamt_erase(root, &node->slots[i]);
		}
		__ohamt_free_slot(root, *slot);
		memset(slot, 0, sizeof(struct ohamt_slot));
	}
}

void ohamt_erase(struct ohamt_root *root)
{
	if (unlikely(ohamt_is_empty(root))) {
		return;
	}

	__ohamt_erase(root, &root->slot);
}

void ohamt_allocated(struct ohamt_root *root,
		     uint64_t *allocated_ptr, uint64_t *wasted_ptr)
{
	mem_allocated(root->mem, allocated_ptr, wasted_ptr);
}
