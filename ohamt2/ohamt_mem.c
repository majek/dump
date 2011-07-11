#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "config.h"
#include "queue.h"
#include "list.h"
#include "ohamt.h"
#include "ohamt_mem.h"
#include "ohamt_bitmap.h"




/* struct PACKED ohamt_slot { */
/* 	char data[5]; */
/* }; */

/* struct ohamt_node { */
/* 	uint64_t mask; */
/* 	struct ohamt_slot slots[]; */
/* }; */




static inline struct mem_page *page_alloc(struct mem *mem, unsigned width)
{
	unsigned chunk_size = CHUNK_SIZE(width);
	int sz = sizeof(struct mem_page) + chunk_size * CHUNKS_ON_PAGE + 3;
	char *ptr;
	if (posix_memalign((void*)&ptr, mem->cache_line_size, sz) != 0) {
		abort();
	}
	mem->pages_allocated ++;
	mem->allocated += sz;

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
	int sz = sizeof(struct mem_page) + CHUNK_SIZE(width) * CHUNKS_ON_PAGE + 3;

	mem->pages_allocated --;
	mem->allocated -= sz;
	bitmap_set(mem->pslots_mask, page->page_slot);
	mem->pslots[page->page_slot] = NULL;
	assert(page->free_chunks == CHUNKS_ON_PAGE);
	free(page);
}


static inline struct ohamt_slot chunk_to_slot(struct mem_page *page,
					      struct mem_chunk *chunk, unsigned width)
{
	char *ptr = (char*)page + sizeof(struct mem_page);
	/* assert((char*)chunk >= ptr); */
	/* assert(((char*)chunk - ptr) % CHUNK_SIZE(width) == 0); */
	unsigned index = ((char*)chunk - ptr) / CHUNK_SIZE(width);
	/* assert(index < CHUNKS_ON_PAGE); */

	union item u = {.node = {.is_node = 1,
				 .index = index,
				 .page_slot = page->page_slot,
				 .swidth = width-1}};
	return u.slot;
}


struct ohamt_slot slot_alloc(struct mem *mem, unsigned width)
{
	mem->used += CHUNK_SIZE(width);
	struct list_head *free_pages = &mem->list_of_free_pages[width-1];
	struct mem_page *page;
	if (!list_empty(free_pages)) {
		page = list_first_entry(free_pages, struct mem_page, in_list);
	} else {
		page = page_alloc(mem, width);
		list_add(&page->in_list, free_pages);
	}
	/* assert(page->width == width); */

	struct queue_head *qhead = queue_get(&page->queue_of_free_chunks);
	struct mem_chunk *chunk = \
		container_of(qhead, struct mem_chunk, in_queue);
	page->free_chunks --;

	if (unlikely(page->free_chunks == 0)) {
		list_del(&page->in_list);
		list_add(&page->in_list, &mem->list_of_busy_pages);
	}

	return chunk_to_slot(page, chunk, width);
}

void slot_free(struct mem *mem, struct ohamt_slot slot)
{
	union item u = {.slot = slot};
	mem->used -= CHUNK_SIZE(U_WIDTH(u));

	struct list_head *free_pages = &mem->list_of_free_pages[U_WIDTH(u) - 1];
	struct mem_page *page = mem->pslots[u.node.page_slot];

	struct mem_chunk *chunk = slot_to_chunk(mem, slot);

	queue_put_head(&chunk->in_queue,
		       &page->queue_of_free_chunks);
	page->free_chunks ++;

	if (unlikely(page->free_chunks == 1)) {
		list_del(&page->in_list);
		list_add(&page->in_list, free_pages);
	} else
	if (unlikely(page->free_chunks == CHUNKS_ON_PAGE)) {
		if (unlikely(list_is_singular(free_pages))) {
			/* Always keep one page hanging to avoid thrashing. */
		} else {
			list_del(&page->in_list);
			page_free(mem, page, U_WIDTH(u));
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
	struct mem *mem = mmap(NULL, sizeof(struct mem),
			   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
			   -1, 0);
	assert(mem != MAP_FAILED);

	mem->pages_allocated = 0;
	mem->cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	if (mem->cache_line_size == -1) {
		mem->cache_line_size = 64;
	}
	mem->allocated = 0;
	mem->used = 0;
	INIT_LIST_HEAD(&mem->list_of_busy_pages);
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
	assert(mem->allocated == 0);
	assert(mem->used == 0);
	munmap(mem, sizeof(struct mem));
}

void mem_allocated(struct mem *mem,
		   uint64_t *allocated_ptr, uint64_t *wasted_ptr)
{
	if (allocated_ptr) {
		*allocated_ptr = mem->allocated;
	}
	if (wasted_ptr) {
		*wasted_ptr = mem->allocated - mem->used;
	}
}
