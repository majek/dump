#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

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
	struct sockaddr_in sin4;
	memset(&sin4, 0, sizeof(sin4));
	sin4.sin_family = AF_INET;
	sin4.sin_port = 0;
//	sin4.sin_addr = INADDR_ANY;
	struct sockaddr *sockaddr = (struct sockaddr*)&sin4;
	int sockaddr_len = sizeof(sin4);


	int sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int one = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
		   sizeof(one));

	sin4.sin_port = htons(1 + 1024);
	connect(sd, sockaddr, sockaddr_len);


	uint64_t ta = realtime_now();
	uint64_t best_td = (uint64_t)-1LL;
	int i;
	for (i = 0; i < 16; i++) {
		uint64_t t0 = realtime_now();
		char buf[1024];
		send(sd, buf, sizeof(buf), 0);
		recv(sd, buf, sizeof(buf), 0);
		uint64_t td = realtime_now()- t0;
		if (best_td > td)
			best_td = td;
	}
	uint64_t tdd = realtime_now()- ta;
	printf("min=%.3fms tot=%.3fms\n", best_td/1000000., tdd / 1000000.);

	return 0;
}
