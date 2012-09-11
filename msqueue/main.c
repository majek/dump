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

#ifdef __MACH__
# include <mach/mach_time.h>
#endif

#include "queue.h"
#include "stddev.h"

#ifdef __MACH__
unsigned long long gettime()
{
	return mach_absolute_time();
}
#else
unsigned long long gettime()
{
	struct timespec t;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
	return (long long)t.tv_sec * 1000000000LL + t.tv_nsec;
}
#endif

static void *malloc_aligned(unsigned int size)
{
	void *ptr;
	int r = posix_memalign(&ptr, 256, size);
	if (r != 0) {
		perror("malloc error");
		abort();
	}
	memset(ptr, 0, size);
	return ptr;
}

struct thread_data {
	pthread_t thread_id;
	struct queue_root *queue;
	double locks_ps;
	unsigned long long rounds;
};

static unsigned long long threshold = 10000;

static void *thread_entry_point(void *_thread_data)
{
	unsigned long long counter = 0;
	unsigned long long t0, t1;
	struct thread_data *td = (struct thread_data *)_thread_data;
	struct queue_root *queue = td->queue;

	struct queue_head *item = \
		malloc_aligned(sizeof(struct queue_head));
	INIT_QUEUE_HEAD(item);

	t0 = gettime();
	while (1) {
		queue_put(item, queue);
		item = queue_get(queue);
		if (!item) {
			abort();
		}
		counter ++;
		if (counter != threshold) {
			continue;
		} else {
			t1 = gettime();
			unsigned long long delta = t1 - t0;
			double locks_ps = (double)counter / ((double)delta / 1000000000LL);
			td->locks_ps = locks_ps;
			td->rounds += 1;
			t0 = t1;
			counter = 0;

			#if 0
			printf("thread=%16lx round done in %.3fms, locks per sec=%.3f\n",
			       (long)pthread_self(),
			       (double)delta / 1000000,
			       locks_ps);
			#endif
		}
	}

	return NULL;
}


int main(int argc, char **argv)
{
	int i;
	int threads = 1;
	int prefill = 0;

	if (argc > 1) {
		threads = atoi(argv[1]);
	}
	if (argc > 2) {
		prefill = atoi(argv[2]);
	}
	if (argc > 3) {
		fprintf(stderr, "Usage: %s [threads] [prefill]\n", argv[0]);
		abort();
	}
	fprintf(stderr, "Running with threads=%i prefill=%i\n", threads, prefill);
	threshold /= threads;

	struct queue_root *queue = ALLOC_QUEUE_ROOT();

	for (i=0; i < prefill; i++) {
		struct queue_head *item = \
			malloc_aligned(sizeof(struct queue_head));
		INIT_QUEUE_HEAD(item);
		queue_put(item, queue);
	}

	struct thread_data *thread_data = \
		malloc_aligned(sizeof(struct thread_data) * threads);

	for (i=0; i < threads; i++) {
		thread_data[i].queue = queue;
		int r = pthread_create(&thread_data[i].thread_id,
				       NULL,
				       &thread_entry_point,
				       &thread_data[i]);
		if (r != 0) {
			perror("pthread_create()");
			abort();
		}
	}

	printf("total loops per second, average loops per thread, deviation, rounds average, deviation\n");
	int reports = 0;
	while (1) {
		sleep(1);
		// Not doing any locking - we're only reading values.
		// Single round shorter than 50ms ?
		if (threshold / thread_data[0].locks_ps < 0.05) {
			fprintf(stderr, "threshold %lli -> %lli\n",
			       threshold,
			       threshold * 2);
			threshold *= 2;
			continue;
		}

		struct stddev sd, rounds;
		memset(&sd, 0, sizeof(sd));
		memset(&rounds, 0, sizeof(rounds));
		for (i=0; i < threads; i++) {
			stddev_add(&sd, thread_data[i].locks_ps);
			stddev_add(&rounds, thread_data[i].rounds);
		}
		double avg, dev, rounds_avg, rounds_dev;
		stddev_get(&sd, NULL, &avg, &dev);
		stddev_get(&rounds, NULL, &rounds_avg, &rounds_dev);
		printf("%.3f, %.3f, %.3f, %.3f, %.3f\n", sd.sum, avg, dev, rounds_avg, rounds_dev);

		if (reports++ > 20) {
			break;
		}
	}

	return 0;
}
