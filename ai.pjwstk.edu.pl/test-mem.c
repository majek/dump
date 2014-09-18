/* 
gcc -Wall -O2 test-mem.c  -o test-mem && ./test-mem
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>

/* stolen from nmap/utils.h
   Timeval subtraction in microseconds */
#define TIMEVAL_SUBTRACT(a,b) (((a).tv_sec - (b).tv_sec) * 1000000 + (a).tv_usec - (b).tv_usec)


long int memory_megabytes=512;
#define MEMORY_POOL_SZ (memory_megabytes*1024*256)

volatile int loop_goes=1;

void sig_handler(int sig){
	loop_goes = 0;
	return;
}

int do_benchmark(int loop_number, int benchmark_time){
	unsigned int i;
	struct timeval time0;
	struct timeval time1;
	
	unsigned long bigloop_counter = 0;
	loop_goes = 1;
	int *bigmem;
	bigmem = (int*)malloc(MEMORY_POOL_SZ*sizeof(int));
	// Load the memory!
	for(i=0; i<MEMORY_POOL_SZ; i++){
		bigmem[i] = 31;
	}

	if(benchmark_time){
		// Schedule signal
		alarm(benchmark_time);
		signal(SIGALRM, sig_handler);		
	}

	gettimeofday(&time0, NULL);

	while(loop_goes){
		// This loop is rather quick
		bigloop_counter++;

		for(i=0; i<MEMORY_POOL_SZ; i++){
			bigmem[i] = 32;
		}
		
		// if loop_number is specified, just exit if counter returns
		if(loop_number){
			loop_number--;
			if(!loop_number)
				break;
		}
	}
	gettimeofday(&time1, NULL);
	unsigned long long microsec = TIMEVAL_SUBTRACT(time1, time0);
	double mln_int_per_sec = (((MEMORY_POOL_SZ/1000000.0)*bigloop_counter)/(microsec/1000000.0) );
	printf("In %3.3fms done %lu tests. It means %7.3f mln (read integers)/second. = %7.3f Gbits/sec = %7.3f GBytes/sec\n",
			microsec/1000.0, bigloop_counter, mln_int_per_sec, (mln_int_per_sec*4.0*8.0)/1000.0, (mln_int_per_sec*4.0)/1024.0 );	
	free(bigmem);
	return(bigloop_counter);
}

int main(int argc, char **argv)
{
	int benchmark_time = 1;
	int processes_to_fork = 0;
	if(argc > 1){
		processes_to_fork = atoi(argv[1]);
		if(argc > 2){
			benchmark_time = atoi(argv[2]);
			if(argc > 3){
				memory_megabytes = atoi(argv[3]);
			}
		}
	}else{
		printf("%s <processes> <benchmark_time_in_seconds> <mem_MB_to_allocate>\n", argv[0]);
		exit(0);
	}
	
	printf("For single process:\n");
	int loop_number = do_benchmark(0, benchmark_time);
	printf("\nFor %i processes:\n", processes_to_fork);
	int i;
	
	for(i=0; i<processes_to_fork; i++){
		int pid = fork();
		switch(pid){
			case 0: // child
				do_benchmark(loop_number, 0);
				exit(1234); // child always exits.
				break;
			default: // father
			break;
		}
	}
	i = 0;
	while(i!= processes_to_fork){
		int exitcode = 0;
		wait(&exitcode);
		i++;
	}
	
	
	return(0);
}
