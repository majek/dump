#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define TIMEVAL_NSEC(ts)                                                       \
	((ts)->tv_sec * 1000000000ULL + (ts)->tv_usec * 1000ULL)

uint64_t realtime_now()
{
        struct timeval tv;
	gettimeofday(&tv, NULL);
        return TIMEVAL_NSEC(&tv);
}

int main()
{
	int port = 1024+1;

	struct sockaddr_in sin4;
	memset(&sin4, 0, sizeof(sin4));
	sin4.sin_family = AF_INET;
	sin4.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &(sin4.sin_addr));

	struct sockaddr *sockaddr = (struct sockaddr*)&sin4;
	int sockaddr_len = sizeof(sin4);


	//int sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	/* int one = 1; */
	/* setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, */
	/* 	   sizeof(one)); */


	uint64_t ta = realtime_now();
	uint64_t best_td = (uint64_t)-1LL;
	uint64_t sum = 0;
	uint64_t sum_sq = 0;
	int MAX = 16;
	int i;
	for (i = 0; i < MAX; i++) {
		int sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

		int flags = fcntl(sd, F_GETFL, 0);
		fcntl(sd, F_SETFL, flags | O_NONBLOCK);

		uint64_t t0 = realtime_now();
		int r = connect(sd, sockaddr, sockaddr_len);
		uint64_t t1 = realtime_now();
		if (r < 0 && errno != EINPROGRESS){
			perror("connect()");
		}
		close(sd);

		int k;
		for (k=0; k < 1000; k++) {
			sched_yield();
		}
		//usleep(1000*1000); // 100ms
		sleep(1);

		uint64_t td = t1 - t0;
		if (best_td > td)
			best_td = td;
		printf("took %.3fms\n", td / 1000000.);
		sum += td;
		sum_sq += td*td;
	}
	uint64_t tdd = realtime_now()- ta;
	printf("min=%.3fms tot=%.3fms\n", best_td/1000000., tdd / 1000000.);

	double avg, dev, var;
	avg = (double)sum / (double)MAX;
	var = ((double)sum_sq / (double)MAX) - (avg*avg);
	dev = sqrt(var);

	printf("min %.3f avg %.3f var %.3f\n", best_td / 1000000., avg / 1000000., dev/ 1000000.);
	return 0;
}
