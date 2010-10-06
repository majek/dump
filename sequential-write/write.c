/*

rm bigfile.bin; gcc -Wall write.c -owrite -lrt && time ./write 128 f; ls -lah bigfile.bin;

 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef CLOCK_MONOTONIC
void get_clock(struct timespec *ts) {
	clock_gettime(CLOCK_MONOTONIC, ts);
}
#else
void get_clock(struct timespec *ts) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000L;
}
#endif

#include <sys/mman.h>

#define TIMESPEC_SUBTRACT(a,b) \
	((long long)((a).tv_sec - (b).tv_sec) * 1000000000LL + (a).tv_nsec - (b).tv_nsec)
#define MIN(a,b)   ((a) <= (b) ? (a) : (b))


struct timespec t0, t1, tt0, tt1;
void lap_start() {
	get_clock(&t0);
}

void lap_stop(char *fun, int stdout) {
	get_clock(&t1);
	long long td;
 	td = TIMESPEC_SUBTRACT(t1, t0);
	fprintf(stderr, "%s \t%6llims\n", fun, td/1000000);
	if (stdout) {
		printf("%s \t%.3f\n", fun, td/1000000/1000.0);
	}
	get_clock(&t0);
}

void t_lap_start() {
	get_clock(&tt0);
}
void t_lap_stop(char *fun, long size) {
	get_clock(&tt1);
	long long td;
 	td = TIMESPEC_SUBTRACT(tt1, tt0);
	float mbs = ((size/(1024.0*1024.0)) / (td/1000000.0/1000.0));
	fprintf(stderr, "%s \t%.3fMB/s\n", fun, mbs);
	printf("%s \t%.3f\n", fun, mbs);
}


void gogo_fsync(int fd, long size);
void gogo_msync(int fd, long size);

int main(int argc, char *argv[]) {
	int r;
	long size = 1L*1024L*1024L*1024L;
	if (argc > 1) {
		size = atoi(argv[1])*1024L*1024L;
	} else {
		fprintf(stderr, "%s [size in MB] [m|f|t|a] [file name]", argv[0]);
		abort();
	}

	int do_fsync = 1;
	int do_ftruncate = 0;
	int do_fallocate = 0;

	if (argc > 2) {
		char *opt;
		for (opt=argv[2]; *opt; opt++) {
			switch(*opt) {
			case 'm':
				do_fsync = 0;
				break;
			case 'f':
				do_fsync = 1;
				break;
			case 't':
				do_ftruncate = 1;
				break;
			case 'a':
				do_fallocate = 1;
				break;
			default:;
				fprintf(stderr, "Unknown option \"%c\".\n", *opt);
				abort();
			}
		}
	}
	if(do_fsync) {
	} else {
		if (do_ftruncate == 0 && do_fallocate == 0)
			do_ftruncate = 1;
	}

	char *filename = "bigfile.bin";
	if (argc > 3) {
		filename = argv[3];
	}

	fprintf(stderr, "file: \"%s\"\n", filename);
	fprintf(stderr, "size: %liMB\n", size/1024L/1024L);
	fprintf(stderr, "options: %s%s%s\n",
		do_fsync? "Fsync " : "Msync ",
		do_ftruncate? "fTruncate " : "",
		do_fallocate? "fAllocate " : ""
		);

	lap_start();

	int fd = open(filename, O_CREAT|O_TRUNC|O_RDWR, 0700);
	if (fd < 0) {
		perror("open()"); abort();
	}
	lap_stop("open", 0);

	if (do_ftruncate) {
		r = ftruncate(fd, size);
		if (r != 0) {
			perror("ftruncate()");
		}
		lap_stop("ftruncate", 0);
	}

	#if 0
	if (do_fallocate) {
		r = posix_fallocate(fd, 0, size);
		if (r != 0) {
			perror("posix_fallocate()");
		}
		lap_stop("posix_fallocate", 0);
	}
	#endif

	if (do_fsync) {
		gogo_fsync(fd, size);
	} else {
		gogo_msync(fd, size);
	}

	r = fsync(fd);
	if (r != 0) {
		perror("fsync()");
	}
	lap_stop("fsync", 0);

	r = close(fd);
	if (r != 0) {
		perror("close()");
	}
	lap_stop("close", 0);

	return 0;
}

void gogo_msync(int fd, long size) {
	int r;
	void *ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		perror("mmap()"); abort();
	}
	lap_stop("mmap", 0);

	t_lap_start();
	memset(ptr, 65, size);
	lap_stop("memset", 1);

	r = msync(ptr, size, MS_SYNC);
	if (r != 0) {
		perror("msync");
	}
	lap_stop("msync", 1);
	t_lap_stop("total", size);

	r = munmap(ptr, size);
	if (r != 0) {
		perror("munmap()");
	}
	lap_stop("munmap", 0);
}

void gogo_fsync(int fd, long size) {
	int r;
	long ptr_size = 32*1024*1024;
	void *ptr = malloc(ptr_size);
	memset(ptr, 65, ptr_size);
	lap_stop("memset(32)", 0);

	t_lap_start();
	long to_write = size;

	while(to_write > 0) {
		r = write(fd, ptr, MIN(to_write, ptr_size));
		if (r < 0) {
			perror("write()");
		}
		to_write -= r;
	}
	lap_stop("write", 1);

	r = fdatasync(fd);
	if (r != 0) {
		perror("fdatasync()");
	}
	lap_stop("fdatasync", 1);
	t_lap_stop("total", size);
}
