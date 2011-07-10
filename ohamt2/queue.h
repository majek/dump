#ifndef QUEUE_H
#define QUEUE_H
/*
 * Simple, memory-efficient implementation of a queue. Not suitable for multiple
 * threads. Use only from one thread.
 */

#ifndef container_of
#  define container_of(ptr, type, member) ({				\
			const typeof( ((type *)0)->member ) *__mptr = (ptr); \
			(type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#ifndef offsetof
#  define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif


#define QUEUE_POISON1 (void*)0xDEADBEE3

struct queue_head {
	struct queue_head *next;
};

struct queue_root {
	struct queue_head *first;
	struct queue_head *last;
};

static inline void INIT_QUEUE_ROOT(struct queue_root *root)
{
	root->first = NULL;
	root->last = NULL;
}

static inline int queue_empty(struct queue_root *root)
{
	if (root->first != NULL) {
		return 0;
	} else {
		return 1;
	}
}

static inline void INIT_QUEUE_HEAD(struct queue_head *item)
{
	item->next = QUEUE_POISON1;
}


/* Returns 1 if the queue was empty 0 otherwise. */
static inline int queue_put(struct queue_head *new,
			    struct queue_root *root)
{
	new->next = NULL;
	if (root->last) {
		root->last->next = new;
		root->last = new;
		return 0;
	} else {
		root->first = new;
		root->last = new;
		return 1;
	}
}

static inline int queue_put_head(struct queue_head *new,
				 struct queue_root *root)
{
	new->next = root->first;
	if (root->first) {
		root->first = new;
		return 0;
	} else {
		root->first = new;
		root->last = new;
		return 1;
	}
}

static inline int queue_is_enqueued(struct queue_head *item)
{
	if (item->next == QUEUE_POISON1) {
		return 0;
	}
	return 1;
}

static inline struct queue_head *queue_get(struct queue_root *root)
{
	if (unlikely(!root->first)) {
		return NULL;
	}
	struct queue_head *item = root->first;
	root->first = item->next;
	if (unlikely(!root->first)) {
		root->last = NULL;
	}
	prefetch(root->first);

	item->next = QUEUE_POISON1;
	return item;
}

static inline void queue_splice(struct queue_root *src,
				struct queue_root *dst)
{
	if (unlikely(src->first == NULL)) {
		return;
	}

	if (dst->last) {
		dst->last->next = src->first;
		dst->last = src->last;
	} else {
		dst->first = src->first;
		dst->last = src->last;
	}

	INIT_QUEUE_ROOT(src);
}

static inline void queue_splice_prepend(struct queue_root *src,
					struct queue_root *dst)
{
	if (unlikely(src->first == NULL)) {
		return;
	}
	struct queue_head *first = dst->first;

	dst->first = src->first;
	src->last->next = first;

	INIT_QUEUE_ROOT(src);
}

static inline void queue_slow_del(struct queue_head *old,
				  struct queue_root *root)
{
	struct queue_root newroot;
	INIT_QUEUE_ROOT(&newroot);
	while (1) {
		struct queue_head *item = queue_get(root);
		if (item != old) {
			queue_put(item,
				  &newroot);
		} else {
			break;
		}
	}
	// prefix is in newroot
	//   item is popped
	// suffix is in root
	queue_splice_prepend(&newroot, root);
}

#endif // QUEUE_H
