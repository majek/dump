struct mem;

struct mem *mem_new();
void mem_free(struct mem *mem);
void mem_allocated(struct mem *mem,
		   uint64_t *allocated_ptr, uint64_t *wasted_ptr);

struct ohamt_slot slot_alloc(struct mem *mem, unsigned width);
void slot_free(struct mem *mem, struct ohamt_slot slot);


union PACKED item {
	struct ohamt_slot slot;
	struct PACKED node {
		unsigned is_node:1;
		unsigned index:10;
		unsigned page_slot:23;
		unsigned swidth:6;
	} node;
	uint64_t leaf;
};

static inline int slot_is_leaf(struct ohamt_slot slot)
{
	union item u = {.slot = slot};
	return !u.node.is_node;
}

static inline uint64_t slot_to_leaf(struct ohamt_slot slot)
{
	union item u = {.slot = slot};
	assert(u.node.is_node == 0);
	return u.leaf;
}

static inline struct ohamt_slot leaf_to_slot(uint64_t leaf)
{
	union item u = {.leaf = leaf};
	assert(!u.node.is_node);
	return u.slot;
}


#define CHUNKS_ON_PAGE 1024
#define PAGE_SLOTS_MAX (1 << 23)
#define CHUNK_SIZE(width) (8 + 5 * (width))
#define U_WIDTH(u) ((int)(u).node.swidth + 1)

struct mem {
	int pages_allocated;
	int cache_line_size;

	uint64_t allocated;
	uint64_t used;

	struct list_head list_of_busy_pages;
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


static inline struct mem_chunk *slot_to_chunk(struct mem *mem, struct ohamt_slot slot)
{
	union item u = {.slot = slot};
	struct mem_page *page = mem->pslots[u.node.page_slot];
	return (struct mem_chunk *)((char*)page + sizeof(struct mem_page) +
				    CHUNK_SIZE(U_WIDTH(u)) * u.node.index);
}

static inline struct ohamt_node *slot_to_node(struct mem *mem, struct ohamt_slot slot)
{
	struct mem_chunk *chunk = slot_to_chunk(mem, slot);
	return (struct ohamt_node *)chunk;
}
