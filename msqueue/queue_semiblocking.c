#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "queue.h"

#define QUEUE_POISON1 ((void*)0xCAFEBAB5)

struct queue_root {
	struct queue_head *in_queue;
	struct queue_head *out_queue;
//	pthread_spinlock_t lock;
	pthread_mutex_t lock;
};


#ifndef _cas
# define _cas(ptr, oldval, newval) \
         __sync_bool_compare_and_swap(ptr, oldval, newval)
#endif

struct queue_root *ALLOC_QUEUE_ROOT()
{
	struct queue_root *root =			\
		malloc(sizeof(struct queue_root));

//	pthread_spin_init(&root->lock, PTHREAD_PROCESS_PRIVATE);
	pthread_mutex_init(&root->lock, NULL);

	root->in_queue = NULL;
	root->out_queue = NULL;
	return root;
}

void INIT_QUEUE_HEAD(struct queue_head *head)
{
	head->next = QUEUE_POISON1;
}

void queue_put(struct queue_head *new,
	       struct queue_root *root)
{
	while (1) {
		struct queue_head *in_queue = root->in_queue;
		new->next = in_queue;
		if (_cas(&root->in_queue, in_queue, new)) {
			break;
		}
	}
}

struct queue_head *queue_get(struct queue_root *root)
{
//	pthread_spin_lock(&root->lock);
	pthread_mutex_lock(&root->lock);

	if (!root->out_queue) {
		while (1) {
			struct queue_head *head = root->in_queue;
			if (!head) {
				break;
			}
			if (_cas(&root->in_queue, head, NULL)) {
				// Reverse the order
				while (head) {
					struct queue_head *next = head->next;
					head->next = root->out_queue;
					root->out_queue = head;
					head = next;
				}
				break;
			}
		}
	}

	struct queue_head *head = root->out_queue;
	if (head) {
		root->out_queue = head->next;
	}
//	pthread_spin_unlock(&root->lock);
	pthread_mutex_unlock(&root->lock);
	return head;
}
