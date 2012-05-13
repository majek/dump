/* Implementation of modified Hash Array Mappe Tree.
 *   reference: http://hamt.sourceforge.net/
 */
#ifndef _QHAMT_H
#define _QHAMT_H


struct qhamt_node;

struct hamt_node {
	u64 mask;
	struct hamt_node *slots[];
};

struct hamt_state{
	int level;
	u64 hash;
	struct hamt_node **ptr[MAX_DEPTH];
};


struct qhamt_root {
	int q;
	struct qhamt_node *node;
	void *(*mem_alloc)(int size);
	void (*mem_free)(int size, void *ptr);
	u64 (*hash)(u64 item);
};

struct qhamt_mem {
	queue_root buffers[64];
};

struct qhamt_buffer {
	queue_head queue;
	int size;

};

#endif // _QHAMT_H
