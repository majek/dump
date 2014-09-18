// gcc -O3 -Wall -msse -mmmx -msse -msse2 test-mem-fill.c -march=pentium4 -mtune=pentium4 && ./a.out 123
/* by Marek Majkowski */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>

#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>

/* Timeval subtract in milliseconds */
#define TIMEVAL_MSEC_SUBTRACT(a,b) ((((a).tv_sec - (b).tv_sec) * 1000) + ((a).tv_usec - (b).tv_usec) / 1000)
#define TIMEVAL_SUBTRACT(a,b) (((a).tv_sec - (b).tv_sec) * 1000000 + (a).tv_usec - (b).tv_usec)


void fill_standard(int bit_field, int *buf, int nodes_length){
	int uid;
	for(uid=0; uid < nodes_length; uid++)
		*buf++ = bit_field;
}

void check(int bit_field, int *buf, int nodes_length){
	int uid;
	for(uid=0; uid < nodes_length; uid++)
		if(buf[uid] != bit_field){
			fprintf(stderr, "ZLE !\n");
			break;
		}
}


void fill_mmx(int bit_field, int *buf, int nodes_length){
	__m64 m =  _mm_set1_pi32(bit_field); /*m = <bf><bf>*/
	__m64 *ptr = (__m64*) buf;
	int uid;
	for(uid=0; uid < nodes_length/2; uid++)
		*ptr++ = m;
}

void fill_sse(int bit_field, int *buf, int nodes_length){
	__m128i *ptr = (__m128i*) buf;
	__m128i s = _mm_set1_epi32(bit_field);
	int uid;
	for(uid=0; uid < nodes_length/4; uid++){
		*ptr++ = s;
	}
}

void fill_sse2(int bit_field, int *buf, int nodes_length){
	__m128i *ptr = (__m128i*) buf;
	__m128i s = _mm_set1_epi32(bit_field);
	int uid;
	for(uid=0; uid < nodes_length/4; uid++){
		_mm_stream_si128(ptr++, s);
	}
}

int main(int argc, char **argv){
	struct timeval time_start;
	struct timeval time_stop;
	unsigned long nanoseconds;

	int  bit_field = atoi(argv[1]);
	int nodes_length = 16*1024*1024;


	int *buf = (int*)malloc((nodes_length+256)*sizeof(int));
	buf = (int*)((unsigned int)buf & ~0x3Fu);


	gettimeofday(&time_start, NULL);
		fill_standard(bit_field, buf, nodes_length);
	gettimeofday(&time_stop, NULL);
	nanoseconds = TIMEVAL_SUBTRACT(time_stop, time_start);
		check(bit_field, buf, nodes_length);
	printf("Standard %5lu.%03lums\n", nanoseconds/1000, nanoseconds%1000 );
		fill_standard(bit_field+2, buf, nodes_length);


	gettimeofday(&time_start, NULL);
		fill_mmx(bit_field, buf, nodes_length);
	gettimeofday(&time_stop, NULL);
	nanoseconds = TIMEVAL_SUBTRACT(time_stop, time_start);
		check(bit_field, buf, nodes_length);
	printf("Fill mmx %5lu.%03lums\n", nanoseconds/1000, nanoseconds%1000 );
		fill_standard(bit_field+2, buf, nodes_length);

	gettimeofday(&time_start, NULL);
		fill_sse(bit_field, buf, nodes_length);
	gettimeofday(&time_stop, NULL);
	nanoseconds = TIMEVAL_SUBTRACT(time_stop, time_start);
		check(bit_field, buf, nodes_length);
	printf("Fill sse %5lu.%03lums\n", nanoseconds/1000, nanoseconds%1000 );
		fill_standard(bit_field+2, buf, nodes_length);

	gettimeofday(&time_start, NULL);
		fill_sse2(bit_field, buf, nodes_length);
	gettimeofday(&time_stop, NULL);
	nanoseconds = TIMEVAL_SUBTRACT(time_stop, time_start);
		check(bit_field, buf, nodes_length);
	printf("Fill sse2 %5lu.%03lums\n", nanoseconds/1000, nanoseconds%1000 );
		fill_standard(bit_field+2, buf, nodes_length);

	return(0);
}
