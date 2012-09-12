#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "queue.h"

#define QUEUE_POISON1 ((void*)0xCAFEBAB5)

struct queue_root {
	struct queue_head *head;
	unsigned int head_lock;

	char r[256];
	struct queue_head *tail;
	unsigned int tail_lock;

	struct queue_head divider;
};

struct queue_root *ALLOC_QUEUE_ROOT()
{
	struct queue_root *root = \
		malloc(sizeof(struct queue_root));
	root->head_lock = 0;
	root->tail_lock = 0;
	__sync_synchronize();

	root->divider.next = NULL;
	root->head = &root->divider;
	root->tail = &root->divider;
	return root;
}

static inline void _lock(unsigned int *lock) {
	while (1) {
		int i;
		for (i=0; i < 10000; i++) {
			if (__sync_bool_compare_and_swap(lock, 0, 1)) {
				return;
			}
		}
		sched_yield();
	}
}

static inline void _unlock(unsigned int *lock) {
        __sync_bool_compare_and_swap(lock, 1, 0);
}

void INIT_QUEUE_HEAD(struct queue_head *head)
{
	head->next = QUEUE_POISON1;
}

void queue_put(struct queue_head *new,
	       struct queue_root *root)
{
	new->next = NULL;

	_lock(&root->tail_lock);
	root->tail->next = new;
	root->tail = new;
	_unlock(&root->tail_lock);
}

struct queue_head *queue_get(struct queue_root *root)
{
	struct queue_head *head, *next;

	while (1) {
		_lock(&root->head_lock);
		head = root->head;
		next = head->next;
		if (next == NULL) {
			_unlock(&root->head_lock);
			return NULL;
		}
		root->head = next;
		_unlock(&root->head_lock);
		
		if (head == &root->divider) {
			queue_put(head, root);
			continue;
		}

		head->next = QUEUE_POISON1;
		return head;
	}
}
