#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#include "hamt.h"

#define ITEM_MASK (0xFFFFFF0000000001ULL) /* 40 bits */
#define LEAF_MASK (0x1ULL)

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

#define slice_get(hash, level)						\
        ({                                                              \
                assert(__builtin_types_compatible_p(typeof(hash), uint128_t)); \
                (((hash) >> ((level)*6)) & 0x3fULL);                   \
        })

#define slice_set(hash, slice, level)					\
        ({                                                              \
                assert(__builtin_types_compatible_p(typeof(hash), uint128_t)); \
                uint128_t __s = (slice) & 0x3fULL;                      \
                uint128_t __c = 0x3fULL;                                \
                __s = __s << ((level)*6);                               \
                __c = __c << ((level)*6);                               \
                (((hash) & ~__c) | __s);                                \
        })

static inline int is_leaf(struct hamt_slot *slot)
{
	return !!(slot->off & LEAF_MASK);
}

static inline void *to_leaf(struct hamt_slot *slot)
{
	return (void*)(slot->off & ~LEAF_MASK);
}

static inline struct hamt_slot item_to_slot(void *item)
{
        return (struct hamt_slot){(uint64_t)item | LEAF_MASK};
}

static inline struct hamt_node *ston(struct hamt_slot slot)
{
	return (struct hamt_node *)(uint64_t)slot.off;
}

static inline struct hamt_slot ntos(struct hamt_node *node)
{
	return (struct hamt_slot){(uint64_t)node};
}

/**************************************************************************/

static inline void *__hamt_search(struct hamt_root *root,
                                  uint128_t *hash,
                                  struct hamt_state *s)
{
        if (!is_leaf(s->ptr[s->level])) {
                struct hamt_node *node = ston(*s->ptr[s->level]);

		int slice = slice_get(*hash, s->level);
		if (set_contains(node->mask, slice)) {
                        s->hash = slice_set(s->hash, slice, s->level);
			int slot = set_slot_number(node->mask, slice);
                        s->ptr[s->level + 1] = &node->slots[slot];
                        s->level += 1;
                        return __hamt_search(root, hash, s);
                }
                return NULL;
        } else {
                void *item = to_leaf(s->ptr[s->level]);
                if (*root->hash(root->hash_ud, item) == *hash) {
                        return item;
                } else {
                        return NULL;
                }
        }
}

void *hamt_search(struct hamt_root *root, uint128_t *hash)
{
	if (unlikely(ston(root->slot) == NULL)) {
		return NULL;
	}

        struct hamt_state s;
        s.level = 0;
        s.ptr[0] = &root->slot;

	return __hamt_search(root, hash, &s);
}

/**************************************************************************/

static struct hamt_node *__hamt_new_node(struct hamt_root *root, uint64_t mask,
                                         int len)
{
	int size = sizeof(struct hamt_node) + len * sizeof(struct hamt_slot);
	struct hamt_node *node = \
		(struct hamt_node *)root->mem_alloc(size);
	assert(((unsigned long)node & ITEM_MASK) == 0);
	node->mask = mask;
	return node;
}

static void __hamt_free_node(struct hamt_root *root, struct hamt_node *node)
{
	int len = set_count(node->mask);
	int size = sizeof(struct hamt_node) + len * sizeof(struct hamt_slot);
	root->mem_free(node, size);
}

static struct hamt_node *__hamt_new_node1(struct hamt_root *root, int slice,
                                          struct hamt_slot slot)
{
	struct hamt_node *node =  __hamt_new_node(root,
						  set_add(0ULL, slice),
						  1);
	node->slots[0] = slot;
	return node;
}

static struct hamt_node *__hamt_new_node2(struct hamt_root *root,
					  void *item1, int slice1,
					  void *item2, int slice2)
{
	struct hamt_node *node =                \
		__hamt_new_node(root,
				set_add(set_add(0ULL, slice1), slice2),
				2);
	node->slots[set_slot_number(node->mask, slice1)] = item_to_slot(item1);
	node->slots[set_slot_number(node->mask, slice2)] = item_to_slot(item2);
	return node;
}

static struct hamt_node *__hamt_add_slot(struct hamt_root *root,
					 struct hamt_slot *slot_ptr,
					 void *item, uint128_t *item_hash,
					 int level)
{
	uint64_t slice = slice_get(*item_hash, level);

	struct hamt_node *old_node = ston(*slot_ptr);
	int old_size = set_count(old_node->mask);
	int new_size = old_size + 1;
	struct hamt_node *node = __hamt_new_node(root,
						 set_add(old_node->mask, slice),
						 new_size);
	*slot_ptr = ntos(node);

	int slot = set_slot_number(node->mask, slice);

	memcpy(&node->slots[0], &old_node->slots[0],
	       sizeof(struct hamt_slot)*slot);
	memcpy(&node->slots[slot+1], &old_node->slots[slot],
	       sizeof(struct hamt_slot)*(old_size-slot));
	node->slots[slot] = item_to_slot(item);

	__hamt_free_node(root, old_node);
	return node;
}

/**************************************************************************/

static inline void __hamt_insert_leaf(struct hamt_root *root,
                                      struct hamt_state *s,
                                      uint128_t *item_hash, void *item,
                                      uint128_t *leaf_hash, void *leaf)
{
        int leaf_slice = slice_get(*leaf_hash, s->level);
        int item_slice = slice_get(*item_hash, s->level);
        if (leaf_slice != item_slice) {
		*s->ptr[s->level] = ntos(__hamt_new_node2(root,
                                                          item, item_slice,
                                                          leaf, leaf_slice));
        } else {
                struct hamt_node *node =  __hamt_new_node1(root,
                                                           item_slice,
							   item_to_slot(NULL));
                *s->ptr[s->level] = ntos(node);
                s->ptr[s->level + 1] = &node->slots[0];
                s->level += 1;
                __hamt_insert_leaf(root, s, item_hash, item, leaf_hash, leaf);
        }
}

void *hamt_insert(struct hamt_root *root, void *item)
{
	uint128_t *item_hash = root->hash(root->hash_ud, item);
	assert(((unsigned long)item & ITEM_MASK) == 0);
	assert(item);

	if (unlikely(ston(root->slot) == NULL)) {
		root->slot = item_to_slot(item);
		return item;
	}

	struct hamt_state s;
        s.level = 0;
        s.ptr[0] = &root->slot;
        void *found_item = __hamt_search(root, item_hash, &s);

        if (unlikely(found_item != NULL)) {
                return found_item;
        }

        if (!is_leaf(s.ptr[s.level])) {
                __hamt_add_slot(root, s.ptr[s.level], item, item_hash, s.level);
        } else {
                void *leaf = to_leaf(s.ptr[s.level]);
                uint128_t *leaf_hash = root->hash(root->hash_ud, leaf);

                __hamt_insert_leaf(root, &s, item_hash, item, leaf_hash, leaf);
        }
	return item;
}

/**************************************************************************/

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
	       sizeof(struct hamt_slot)*slot);
	memcpy(&node->slots[slot], &old_node->slots[slot+1],
	       sizeof(struct hamt_slot)*(old_size-slot-1));

	__hamt_free_node(root, old_node);
	return node;
}

static struct hamt_slot __hamt_get_other_slot(struct hamt_node *node,
					       struct hamt_slot *a)
{
	if (node->slots[0].off == a->off) {
		return node->slots[1];
	} else {
		return node->slots[0];
	}
}

void *hamt_delete(struct hamt_root *root, uint128_t *hash)
{
	if (unlikely(ston(root->slot) == NULL)) {
		return NULL;
	}

	struct hamt_state s;
        s.level = 0;
        s.ptr[0] = &root->slot;

	void *found_item = __hamt_search(root, hash, &s);
	if (unlikely(found_item == NULL)) {
		return NULL;
	}
	if (unlikely(s.level == 0)) {
		root->slot = (struct hamt_slot){0};
		goto done;
	}
	struct hamt_slot *found_slot = s.ptr[s.level];
	s.level--;

	struct hamt_node *node = ston(*s.ptr[s.level]);

	int slice = slice_get(*hash, s.level);
	if (set_count(node->mask) != 2) {
		*s.ptr[s.level] = ntos(__hamt_del_node(root, node, slice));
		goto done;
	} else { // set_count == 2
		struct hamt_slot other_slot = \
			__hamt_get_other_slot(node, found_slot);
		if(!is_leaf(&other_slot)) {
			uint64_t other_slice = set_first(set_del(node->mask, slice));
			__hamt_free_node(root, node);
			*s.ptr[s.level] = ntos(__hamt_new_node1(root, other_slice, other_slot));
			goto done;
		} else {
			while (1) {
				__hamt_free_node(root, node);
				if (unlikely(s.level == 0)) {
					root->slot = other_slot;
					goto done;
				}

				s.level--;
				node = ston(*s.ptr[s.level]);
				if (set_count(node->mask) != 1) {
					break;
				}
			}
			slice = slice_get(*hash, s.level);
			int slot = set_slot_number(node->mask, slice);
			node->slots[slot] = other_slot;
			goto done;
		}
	}
done:
	return found_item;
}

/**************************************************************************/

static void *__hamt_down(struct hamt_state *s)
{
	if (!is_leaf(s->ptr[s->level])) {
                struct hamt_node *node = ston(*s->ptr[s->level]);
		int slice = set_first(node->mask);
		s->hash = slice_set(s->hash, slice, s->level);

		int slot = set_slot_number(node->mask, slice);
		s->ptr[s->level + 1] = &node->slots[slot];
		s->level += 1;
		return __hamt_down(s);
	} else {
                return to_leaf(s->ptr[s->level]);
	}
}

void *hamt_first(struct hamt_root *root, struct hamt_state *s)
{
	if (unlikely(ston(root->slot) == NULL)) {
		return NULL;
	}

	s->level = 0;
	s->ptr[0] = &root->slot;
	return __hamt_down(s);
}
