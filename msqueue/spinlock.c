/*
to fail:
gcc -Os -lrt spinlock.c -pthread -DFAIL && ./a.out

to succeed:
gcc -Os -lrt spinlock.c -pthread  && ./a.out
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

static void _lock(unsigned int *lock) {
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

static void _unlock(unsigned int *lock) {
#ifdef FAIL
#else
//	__sync_synchronize();
	__asm__ __volatile__ ("sfence" ::: "memory");
#endif
	*lock = 0;
}

struct queue_head {
	struct queue_head *next;
};

struct queue_root {
	struct queue_head *head;
	unsigned int head_lock;

	struct queue_head *tail;
	unsigned int tail_lock;

	struct queue_head divider;
};

static struct queue_root *queue_root_alloc()
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

		head->next = NULL;
		return head;
	}
}


struct queue_root *queue = NULL;


static void *thread_entry_point(void *_thread_data)
{
	struct queue_head *item = \
		malloc(sizeof(struct queue_head));

	while (1) {
		queue_put(item, queue);
		item = queue_get(queue);
		if (!item) {
			abort();
		}		
	}
}


int main(int argc, char **argv)
{
	int threads = 4, i;

	pthread_t *thread_ids =				\
		malloc(sizeof(pthread_t) * threads);

	queue = queue_root_alloc();

	for (i=0; i < threads; i++) {
		int r = pthread_create(&thread_ids[i],
				       NULL,
				       &thread_entry_point,
				       NULL);
		if (r != 0) {
			perror("pthread_create()");
			abort();
		}
	}

	while (1) {
		sleep(100);
	}
	return 0;
}
