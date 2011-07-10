#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "queue.h"
#include "list.h"
#include "ohamt_bitmap.h"


#define CHUNKS_ON_PAGE 1024
#define PAGE_SLOTS_MAX (1 << 23)
#define CHUNK_SIZE(width) (8 + 5 * (width))
#define U_WIDTH(u) ((int)(u).node.swidth + 1)

struct PACKED ohamt_slot {
	char data[5];
};

struct ohamt_node {
	uint64_t mask;
	struct ohamt_slot slots[];
};

union PACKED item {
	struct ohamt_slot slot;
	struct node {
		unsigned is_node:1;
		unsigned index:10;
		unsigned page_slot:23;
		unsigned swidth:6;
	} PACKED node;
	uint64_t leaf;
};

struct mem {
	int pages_allocated;
	int cache_line_size;

	struct list_head list_of_free_pages[64];
	uint64_t pslots_mask[PAGE_SLOTS_MAX/sizeof(uint64_t)];
	struct mem_page *pslots[PAGE_SLOTS_MAX];
};

struct mem_page {
	struct list_head in_list;
	struct queue_root queue_of_free_chunks;
	int free_chunks;
	int page_slot;
	unsigned width;
};

struct mem_chunk {
	struct queue_head in_queue;
};

static inline struct mem_page *page_alloc(struct mem *mem, unsigned width)
{
	mem->pages_allocated ++;
	unsigned chunk_size = CHUNK_SIZE(width);
	char *ptr;
	if (posix_memalign((void*)&ptr, mem->cache_line_size, sizeof(struct mem_page) +
			   chunk_size * CHUNKS_ON_PAGE) != 0) {
		abort();
	}

	struct mem_page *page = (struct mem_page *)ptr;
	INIT_LIST_HEAD(&page->in_list);
	INIT_QUEUE_ROOT(&page->queue_of_free_chunks);
	page->free_chunks = CHUNKS_ON_PAGE;
	page->page_slot = bitmap_find_set(mem->pslots_mask, sizeof(mem->pslots_mask));
	bitmap_clear(mem->pslots_mask, page->page_slot);
	mem->pslots[page->page_slot] = page;
	page->width = width;

	char *item = ptr + sizeof(struct mem_page);
	int i;
	for(i=0; i < CHUNKS_ON_PAGE; i++, item += chunk_size) {
		struct mem_chunk *chunk = (struct mem_chunk*)item;
		queue_put(&chunk->in_queue, &page->queue_of_free_chunks);
	}
	return page;
}

static inline void page_free(struct mem *mem, struct mem_page *page,
			     unsigned width)
{
	width = width;

	mem->pages_allocated --;
	bitmap_set(mem->pslots_mask, page->page_slot);
	mem->pslots[page->page_slot] = NULL;
	assert(page->free_chunks == CHUNKS_ON_PAGE);
	free(page);
}



static struct mem_chunk *slot_to_chunk(struct mem *mem, struct ohamt_slot slot)
{
	union item u = {.slot = slot};
	struct mem_page *page = mem->pslots[u.node.page_slot];
	return (struct mem_chunk *)((char*)page + sizeof(struct mem_page) +
				    CHUNK_SIZE(U_WIDTH(u)) * u.node.index);
}

static struct ohamt_slot chunk_to_slot(struct mem_page *page,
				       struct mem_chunk *chunk, unsigned width)
{
	char *ptr = (char*)page + sizeof(struct mem_page);
	assert((char*)chunk >= ptr);
	assert(((char*)chunk - ptr) % CHUNK_SIZE(width) == 0);
	unsigned index = ((char*)chunk - ptr) / CHUNK_SIZE(width);
	assert(index < CHUNKS_ON_PAGE);

	union item u = {.node = {.is_node = 1,
				 .index = index,
				 .page_slot = page->page_slot,
				 .swidth = width-1}};
	return u.slot;
}


int slot_is_leaf(struct ohamt_slot slot)
{
	union item u = {.slot = slot};
	return !u.node.is_node;
}

uint64_t slot_to_leaf(struct ohamt_slot slot)
{
	union item u = {.slot = slot};
	assert(u.node.is_node == 0);
	return u.leaf;
}

struct ohamt_node *slot_to_node(struct mem *mem, struct ohamt_slot slot)
{
	union item u = {.slot = slot};
	assert(u.node.is_node);
	struct mem_chunk *chunk = slot_to_chunk(mem, slot);
	return (struct ohamt_node *)chunk;
}

struct ohamt_slot leaf_to_slot(uint64_t leaf)
{
	union item u = {.leaf = leaf};
	assert(!u.node.is_node);
	return u.slot;
}


struct ohamt_slot slot_alloc(struct mem *mem, unsigned width)
{
	struct list_head *free_pages = &mem->list_of_free_pages[width-1];
	struct mem_page *page;
	if (!list_empty(free_pages)) {
		page = list_first_entry(free_pages, struct mem_page, in_list);
	} else {
		page = page_alloc(mem, width);
		list_add(&page->in_list, free_pages);
	}
	assert(page->width == width);

	struct queue_head *qhead = queue_get(&page->queue_of_free_chunks);
	struct mem_chunk *chunk = \
		container_of(qhead, struct mem_chunk, in_queue);
	page->free_chunks --;

	if (unlikely(page->free_chunks == 0)) {
		list_del(&page->in_list);
		INIT_LIST_HEAD(&page->in_list);
	}

	return chunk_to_slot(page, chunk, width);
}

void slot_free(struct mem *mem, struct ohamt_slot slot)
{
	union item u = {.slot = slot};

	struct list_head *free_pages = &mem->list_of_free_pages[U_WIDTH(u) - 1];
	struct mem_page *page = mem->pslots[u.node.page_slot];

	struct mem_chunk *chunk = slot_to_chunk(mem, slot);

	queue_put_head(&chunk->in_queue,
		       &page->queue_of_free_chunks);
	page->free_chunks ++;

	if (unlikely(page->free_chunks == 1 && list_empty(&page->in_list))) {
		list_add(&page->in_list,
			 free_pages);
	}
	if (unlikely(page->free_chunks == CHUNKS_ON_PAGE)) {
		if (!list_is_singular(free_pages)) {
				list_del(&page->in_list);
				page_free(mem, page, U_WIDTH(u));
		} else {
			/* Always keep one page hanging to avoid thrashing. */
		}
	}
}


static void _mem_do_thrashing(struct mem *mem)
{
	unsigned i;
	for(i=0; i < 64; i++) {
		unsigned width = i+1;
		struct list_head *free_pages = &mem->list_of_free_pages[width - 1];
		struct list_head *head, *tmp;
		list_for_each_safe(head, tmp, free_pages) {
			struct mem_page *page =				\
				container_of(head, struct mem_page, in_list);

			if (page->free_chunks == CHUNKS_ON_PAGE) {
				list_del(&page->in_list);
				page_free(mem, page, width);
			}
		}
	}
}

struct mem *mem_new()
{
	struct mem *mem = calloc(1, sizeof(struct mem));
	mem->pages_allocated = 0;
	mem->cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	if (mem->cache_line_size == -1) {
		mem->cache_line_size = 64;
	}
	int i;
	for(i=0; i < 64; i++) {
		INIT_LIST_HEAD(&mem->list_of_free_pages[i]);
	}
	memset(mem->pslots_mask, 0xFF, sizeof(mem->pslots_mask));
	return mem;
}

void mem_free(struct mem *mem)
{
	_mem_do_thrashing(mem);
	assert(mem->pages_allocated == 0);
	free(mem);
}
