#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define DELAY_MS 42

#define TIMESPEC_NSEC_SUBTRACT(a,b) ((((long long)(a).tv_sec - (long long)(b).tv_sec) * 1000*1000*1000) + ((long long)(a).tv_nsec - (long long)(b).tv_nsec))
static void sighandler(int signo, siginfo_t *si, void *ucontext) {
	static struct timespec t0;
	if (signo == SIGVTALRM) {
		struct timespec t1;
		clock_gettime(CLOCK_MONOTONIC, &t1);
		long long delta = TIMESPEC_NSEC_SUBTRACT(t1, t0);
		if(t0.tv_sec)
			printf("%llins %ins -> %.3fms\n", delta, (DELAY_MS*1000*1000), ((float)delta/1000000.0 - (float)(DELAY_MS)));
		t0 = t1;
	}
	return;
}


int main(int argc, char *argv[]) {
	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = (void*)sighandler;
	sa.sa_flags = SA_ONSTACK | SA_SIGINFO;
	
	if(sigaction(SIGVTALRM, &sa, NULL) < 0)
		return -1;

	struct itimerval timer;
	/* Configure the timer to expire after 1000 msec... */
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = DELAY_MS*1000;
	/* ... and every 1000 msec after that. */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = DELAY_MS*1000;
	/* Start a virtual timer. It counts down whenever this process is executing. */
	setitimer(ITIMER_VIRTUAL, &timer, NULL);
	while(1) {
		// abuse cpu
		int i, j;
		for(i=0; i!=0; i++)
			j = j | i;
	}
}
