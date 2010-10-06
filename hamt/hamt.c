/* Hash Array Mapped Trie */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hamt.h"

struct hamt_node *__hamt_new_node(struct hamt_root *root,
				  u64 mask, int len);
struct hamt_node *__hamt_new_node2(struct hamt_root *root,
				   struct hamt_item *item1,
				   struct hamt_item *item2,
				   int level);
struct hamt_node *__hamt_add_slot(struct hamt_root *root,
				  struct hamt_node **node_ptr,
				  struct hamt_item *item,
				  int level);


int is_leaf(struct hamt_node *node)
{
	return !!(((unsigned long)node) & 0x1);
}

struct hamt_item *get_leaf(struct hamt_node *node)
{
	void *ptr = (void*)(((unsigned long)node) & ~0x1);
	return (struct hamt_item *)ptr;
}

struct hamt_node *to_node(struct hamt_item *item)
{
	void *ptr = (void*)(((unsigned long)item) | 0x1);
	return (struct hamt_node *)ptr;
}





unsigned int bits_for_level(u64 hash, int level)
{
	assert(level <= 10);
	return  (hash >> ((10-level)*6LL)) & 0x3FLL;
}

unsigned int slot_number(u64 mask, unsigned int bitoffset)
{
	assert(bitoffset <= 0x3f);
	assert(mask & (1LL << bitoffset));

	u64 cleared = mask & ~((1LL << bitoffset) | ((1LL << bitoffset)-1));
	return popcount(cleared);
}

int slot_exists(u64 mask, unsigned int bitoffset)
{
	assert(bitoffset <= 0x3f);
	return (mask & (1LL << (u64)bitoffset)) != 0;
}


void __hamt_search(struct hamt_root *root,
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

struct hamt_item *hamt_search(struct hamt_root *root, u64 hash)
{
	int level = 0;
	struct hamt_node **node_ptr;
	__hamt_search(root, hash, &node_ptr, &level);
	if (is_leaf(*node_ptr)) {
		return get_leaf(*node_ptr);
	}
	return NULL;
}

struct hamt_item *hamt_insert(struct hamt_root *root, struct hamt_item *item)
{
	struct hamt_node **node_ptr;
	int level;
	__hamt_search(root, item->hash, &node_ptr, &level);
	if (!*node_ptr) {
		int bit_slice = bits_for_level(item->hash, 0);
		*node_ptr =  __hamt_new_node(root, 1LL << bit_slice, 1);
		(*node_ptr)->slots[0] = to_node(item);
	} else
	if (is_leaf(*node_ptr)) {
		struct hamt_item *leaf = get_leaf(*node_ptr);
		if (leaf->hash == item->hash) {
			return leaf;
		}
		while (bits_for_level(leaf->hash, level) ==
		       bits_for_level(item->hash, level)) {
			int bit_slice = bits_for_level(item->hash, level) ;
			*node_ptr =  __hamt_new_node(root, 1LL << bit_slice, 1);
			node_ptr = &(*node_ptr)->slots[0];
			level++;
		}
		*node_ptr = __hamt_new_node2(root, item, leaf, level);
	} else { // not leaf -> node.
		__hamt_add_slot(root, node_ptr, item, level);
	}
	return item;
}


struct hamt_node *__hamt_new_node(struct hamt_root *root, u64 mask, int len)
{
	assert(len <= 64);
	assert(popcount(mask) == len);
	int size = sizeof(struct hamt_node) + len * sizeof(void*);
	root->mem_histogram[size/sizeof(void*)]++;
	struct hamt_node *node = (struct hamt_node *)malloc(size);
	node->mask = mask;
	return node;
}

void __hamt_free_node(struct hamt_root *root, struct hamt_node *node)
{
	int len = popcount(node->mask);
	int size = sizeof(struct hamt_node) + len * sizeof(void*);
	root->mem_histogram[size/sizeof(void*)]--;
	free(node);
}


struct hamt_node *__hamt_new_node2(struct hamt_root *root,
				   struct hamt_item *item1,
				   struct hamt_item *item2,
				   int level)
{
	u64 bit_slice1 = bits_for_level(item1->hash, level);
	u64 bit_slice2 = bits_for_level(item2->hash, level);

	struct hamt_node *node = __hamt_new_node(root, (1LL << bit_slice1) | (1LL << bit_slice2), 2);
	node->slots[ slot_number(node->mask, bit_slice1) ] = to_node(item1);
	node->slots[ slot_number(node->mask, bit_slice2) ] = to_node(item2);
	/* printf("3e -> %016lx %016lx %016lx\n", */
	/*        node->mask, */
	/*        node->slots[0], */
	/*        node->slots[1]); */
	return node;
}

struct hamt_node *__hamt_add_slot(struct hamt_root *root,
				  struct hamt_node **node_ptr,
				  struct hamt_item *item,
				  int level)
{
	u64 bit_slice = bits_for_level(item->hash, level);

	struct hamt_node *old_node = *node_ptr;
	int old_size = popcount(old_node->mask);
	int new_size = old_size + 1;
	struct hamt_node *node = __hamt_new_node(root,
						 old_node->mask | (1LL << (u64)bit_slice),
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


/* struct hamt_item hamt_iterate(struct hamt_root *root) */
/* { */
/* 	int i; */
/* 	struct hamt_node *node = &root->node; */
/* } */

