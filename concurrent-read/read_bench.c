//  sudo apt-get install libglib2.0-dev

#define _XOPEN_SOURCE 600
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <glob.h>

#include <glib.h>


#define MAX_FDS 32
#define BUF_SZ 65536

#define TIMESPEC_SUBTRACT(a,b) ((long long)((a).tv_sec - (b).tv_sec) * 1000000000LL + (a).tv_nsec - (b).tv_nsec)


struct rnd {
	int fd;
	int offset;
	int length;
};


struct server {
	int is_shutdown;
	GAsyncQueue* joblist_queue;
	GAsyncQueue* out_queue;
};

struct server server;


gpointer network_gthread_aio_read_thread(gpointer _srv) {
	static unsigned int buf[4096];
	struct server *srv = (struct server *)_srv;

	g_async_queue_ref(srv->joblist_queue);

	while (!srv->is_shutdown) {
		GTimeVal ts;
        	struct rnd *r = NULL;

		/* wait one second as the poll() */
		g_get_current_time(&ts);
		g_time_val_add(&ts, 500 * 1000);
		if ((r = g_async_queue_timed_pop(srv->joblist_queue, &ts))) {
			assert(r);
			int t = pread(r->fd, buf, r->length, r->offset);
			assert(t >= 0);
			g_async_queue_push(srv->out_queue, r);
		}
	}
	g_async_queue_unref(srv->joblist_queue);

	return NULL;
}

	
#define LI_THREAD_STACK_SIZE (64 * 1024)
GThread **spawn(int threads_no) {
	int i;
	server.joblist_queue = g_async_queue_new();
	server.out_queue = g_async_queue_new();

	GThread **threads = (GThread**) calloc(threads_no, sizeof(GThread *));
	for(i = 0; i < threads_no; i++) {
		GError *gerr = NULL;
		threads[i] = g_thread_create_full(network_gthread_aio_read_thread, &server, LI_THREAD_STACK_SIZE, 1, TRUE, G_THREAD_PRIORITY_NORMAL, &gerr);
		if (gerr) {
			printf("g_thread_create failed: %s\n", gerr->message);
			assert(0);
		}
	}
	return(threads);
}

void join(GThread **threads, int threads_no) {
	int i;
	server.is_shutdown = 1;
	for(i = 0; i < threads_no; i++) {
		g_thread_join(threads[i]);
	}
	g_async_queue_unref(server.joblist_queue);
	g_async_queue_unref(server.out_queue);
	free(threads);
}



unsigned int rand32() {
	static int fd;
	static unsigned int buf[4096];
	static int buf_pos;
	
	if(0 == fd) {
		fd = open("random.bin", O_RDONLY);
		goto read;
	}

	if(buf_pos >= sizeof(buf)/sizeof(buf[0])) {
		read:;
		int r = read(fd, buf, sizeof(buf));
		if(r != sizeof(buf))
			assert(0);
		buf_pos = 0;
	}
	return buf[buf_pos++];
}

double rand_double() {
	return (double)rand32() / (double)0xFFFFFFFFU;
}


double polar_norm_dist() {
	static double prev;
	if(0.0 != prev) {
		double x2 = prev;
		prev = 0.0;
		return(x2);
	}
start:;
	double v1 = 2*rand_double() - 1;
	double v2 = 2*rand_double() - 1;
	double w = v1*v1 + v2*v2;
	if(w > 1)
		goto start;
	double y = sqrt(-2*log(w) / w);
	double x1 = v1*y;
	double x2 = v2*y;
	prev = x2;
	return(x1);
}




typedef void (*engine_t)(struct rnd *rnd, int rnd_sz);
typedef void (*advise_t)(struct rnd *rnd, int rnd_sz);
typedef void (*preadvise_t)(int *fds, int fds_sz);


static int rnd_cmp(const void *a, const void *b) {
	struct rnd *ii_a = (struct rnd *)a;
	struct rnd *ii_b = (struct rnd *)b;
	if(ii_a->fd == ii_b->fd){
		if(ii_a->offset == ii_b->offset)
			return(0);
		if(ii_a->offset < ii_b->offset)
			return(-1);
		return(1);
	}
	if(ii_a->fd < ii_b->fd)
		return(-1);
	return(1);
}

void do_fadvise_normal(int *fds, int fds_sz) {
	int i;
	for(i=0; i<fds_sz; i++)
		posix_fadvise(fds[i], 0, 0, POSIX_FADV_NORMAL);
}
void do_fadvise_random(int *fds, int fds_sz) {
	int i;
	for(i=0; i<fds_sz; i++)
		posix_fadvise(fds[i], 0, 0, POSIX_FADV_RANDOM);
}
void do_fadvise_sequential(int *fds, int fds_sz) {
	int i;
	for(i=0; i<fds_sz; i++)
		posix_fadvise(fds[i], 0, 0, POSIX_FADV_SEQUENTIAL);
}
void do_fadvise_noreuse(int *fds, int fds_sz) {
	int i;
	do_fadvise_normal(fds, fds_sz);
	for(i=0; i<fds_sz; i++)
		posix_fadvise(fds[i], 0, 0, POSIX_FADV_NOREUSE);
}

void do_fadvise(struct rnd *rnd, int rnd_sz) {
	int i;
	for(i=0; i<rnd_sz; i++) {
		struct rnd *r = &rnd[i];
		posix_fadvise(r->fd, r->offset, r->length, POSIX_FADV_WILLNEED);
	}
}

void do_readahead(struct rnd *rnd, int rnd_sz) {
	int i;
	for(i=0; i<rnd_sz; i++) {
		struct rnd *r = &rnd[i];
		readahead(r->fd, r->offset, r->length);
	}
}

void do_read(struct rnd *rnd, int rnd_sz) {
	char buf[BUF_SZ];
	int i;
	for(i=0; i<rnd_sz; i++) {
		struct rnd *r = &rnd[i];
		int t = pread(r->fd, buf, r->length, r->offset);
		assert(t >= 0);
	}
}

struct rnd *do_sort(struct rnd *rnd_o, int rnd_sz) {
	struct rnd * rnd = (struct rnd*)malloc(sizeof(struct rnd) * rnd_sz);
	memcpy(rnd, rnd_o, rnd_sz*sizeof(struct rnd));
	
	qsort(rnd, rnd_sz, sizeof(struct rnd), rnd_cmp);
	return(rnd);
}


void do_gthread(struct rnd *rnd, int rnd_sz) {
	int i;
	for(i=0; i<rnd_sz; i++) {
		g_async_queue_push(server.joblist_queue, &rnd[i]);
	}
	for(i=0; i<rnd_sz; i++) {
		struct rnd *r = g_async_queue_pop(server.out_queue);
		assert(r);
	}
}



int main(int argc, char **argv) {
	int i;
	assert(argc == 7);
	
	int rnd_sz = atoi(argv[1]);
	char *preadvise_name = argv[2];
	int advise_sorted = strcmp(argv[3], "sort") == 0;
	char *advise_name = argv[4];
	int engine_sorted = strcmp(argv[5], "sort") == 0;
	char *engine_name = argv[6];
	
	int fds[MAX_FDS];
	int fds_sz = 0;
	
	
	glob_t globbuf;
	globbuf.gl_offs = 1;
	char glob_str[] = "data*bin";
	char **off;
		
	/* Load data files */
	glob(glob_str, 0, NULL, &globbuf);
	for(off=globbuf.gl_pathv, fds_sz=0; off && *off; off++, fds_sz++) {
		int fd = open(*off, O_RDONLY);
		assert(fd >= 0);
		fds[fds_sz] = fd;
	}
	globfree(&globbuf);

	printf(" [*] (%i data files) ", fds_sz);
	
	struct rnd *rnd = malloc(sizeof(struct rnd) * rnd_sz);
	for(i=0; i<rnd_sz; i++) {
		int nr = 500.0*polar_norm_dist() + 64.0;
		nr = abs(nr);
		if(nr > BUF_SZ)
			nr = BUF_SZ;
		if(nr == 0)
			nr = 1;
		
		rnd[i] = (struct rnd) {
			.fd = fds[rand32() % fds_sz],
			.offset = rand32() % (512*1024*1024),
			.length = nr
		};
	}

	preadvise_t preadvise;
	if(0 == strcmp(preadvise_name, "normal")) {
		preadvise = do_fadvise_normal;
	}else 
	if(0 == strcmp(preadvise_name, "random")) {
		preadvise = do_fadvise_random;
	}else 
	if(0 == strcmp(preadvise_name, "sequential")) {
		preadvise = do_fadvise_sequential;
	}else 
	if(0 == strcmp(preadvise_name, "noreuse")) {
		preadvise = do_fadvise_noreuse;
	}else {
		assert(0);
	}

	engine_t engine;
	int gthreads_no = 0;
	GThread **threads = NULL;
	if(0 == strcmp(engine_name, "pread")) {
		engine = do_read;
	}else
	if(0 == strcmp(engine_name, "gthread32")) {
		engine = do_gthread;
		gthreads_no = 32;
	}else
	if(0 == strcmp(engine_name, "gthread64")) {
		engine = do_gthread;
		gthreads_no = 64;
	}else
	if(0 == strcmp(engine_name, "gthread128")) {
		engine = do_gthread;
		gthreads_no = 128;
	}else
	if(0 == strcmp(engine_name, "gthread512")) {
		engine = do_gthread;
		gthreads_no = 512;
	}else{
		assert(0);
	}
	
	
	advise_t advise;
	if(0 == strcmp(advise_name, "fadvise")) {
		advise = do_fadvise;
	}else 
	if(0 == strcmp(advise_name, "readahead")) {
		advise = do_readahead;
	}else 
	if(0 == strcmp(advise_name, "none")) {
		advise = NULL;
	}else {
		assert(0);
	}
	
	if(gthreads_no) {
		g_thread_init(NULL);
		threads = spawn(gthreads_no);
	}
	
	struct timespec a, b;
	clock_gettime(CLOCK_MONOTONIC, &a);
	printf("%10s  %11s/%6s %14s/%6s : ",
		preadvise_name,
		advise_name,
		advise_sorted ? "sorted" : "random",
		engine_name,
		engine_sorted ? "sorted" : "random" );
	fflush(stdout);
	for(i=0; i<10; i++) {
		struct rnd *items = &rnd[(rnd_sz/10)*i];
		int items_sz = rnd_sz/10;
		
		preadvise(fds, fds_sz);
		
		if(advise) {
			struct rnd *l_items = items;;
			if(advise_sorted)
				l_items = do_sort(items, items_sz);
			
			advise(l_items, items_sz);
			
			if(advise_sorted)
				free(l_items);
		}
		
		struct rnd *l_items = items;;
		
		if(engine_sorted)
			l_items = do_sort(items, items_sz);
		
		engine(l_items, items_sz);
		
		if(engine_sorted)
			free(l_items);
		
		printf(".");
		fflush(stdout);
	}
	clock_gettime(CLOCK_MONOTONIC, &b);
	double s = (TIMESPEC_SUBTRACT(b, a)/1000000)/1000.0;
	printf(" %8.3fs\n", s);
	if(gthreads_no)
		join(threads, gthreads_no);
	return(0);
}

