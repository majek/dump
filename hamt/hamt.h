#include <sys/types.h>

#define popcount(v) __builtin_popcountll(v)


typedef u_int64_t u64;
typedef u_int32_t u32;

struct hamt_node;

struct hamt_root {
	int mem_histogram[65];
	struct hamt_node *node;
};

#define HAMT_ROOT (struct hamt_root) {{0}, NULL}
#define HAMT_ITEM(i) (struct hamt_item) {i}


struct hamt_node {
	u64 mask;
	struct hamt_node *slots[0];
};

struct hamt_item {
	u64 hash;
};

struct hamt_item *hamt_search(struct hamt_root *root, u64 hash);
struct hamt_item *hamt_insert(struct hamt_root *root, struct hamt_item *item);

unsigned int bits_for_level(u64 hash, int level);
unsigned int slot_number(u64 mask, unsigned int bitoffset);


#ifndef container_of
#define container_of(ptr, type, member) ({      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
	(type *)( (char *)__mptr - offsetof(type,member) );})
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif //container_of

#define	hamt_entry(ptr, type, member) container_of(ptr, type, member)




