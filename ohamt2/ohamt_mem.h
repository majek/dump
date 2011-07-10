struct mem;

struct mem *mem_new();
void mem_free(struct mem *mem);

struct ohamt_slot slot_alloc(struct mem *mem, unsigned width);
void slot_free(struct mem *mem, struct ohamt_slot slot);

char *slot_to_str(struct ohamt_slot slot);

int slot_is_leaf(struct ohamt_slot slot);
uint64_t slot_to_leaf(struct ohamt_slot slot);
struct ohamt_node *slot_to_node(struct mem *mem, struct ohamt_slot slot);

struct ohamt_slot leaf_to_slot(uint64_t leaf);
