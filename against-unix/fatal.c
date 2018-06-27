#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
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


int main(int argc, char **argv)
{
	int forks = 0;
	int fd_max = 1;
	if (argc > 1) {
		forks = atoi(argv[1]);
		if (forks < 0 || forks > 10000) {
			forks = 0;
		}
	}
	if (argc > 2) {
		fd_max = atoi(argv[2]);
		if (fd_max < 0 || fd_max > 10000) {
			fd_max = 1;
		}
	}
	printf("forks = %d, dupes per pid = %d\n", forks, fd_max);
	int *list_of_sd = calloc(1, sizeof(int) * fd_max);
	int *list_of_pids = calloc(1, sizeof(int) * fd_max);

	struct sockaddr_in sin4;
	memset(&sin4, 0, sizeof(sin4));
	sin4.sin_family = AF_INET;
	sin4.sin_port = 0;
	inet_pton(AF_INET, "127.0.0.1", &(sin4.sin_addr));

	struct sockaddr *sockaddr = (struct sockaddr*)&sin4;
	int sockaddr_len = sizeof(sin4);

	int i;
	for (i = 0; i < fd_max; i++) {
		if (i > 0) {
			list_of_sd[i] = dup(list_of_sd[0]);
			continue;
		}
		int sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		int one = 1;
		setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
			   sizeof(one));

		sin4.sin_port = htons(i + 1024+1);
		bind(sd, sockaddr, sockaddr_len);

		int flags = fcntl(sd, F_GETFL, 0);
		fcntl(sd, F_SETFL, flags | O_NONBLOCK);

		list_of_sd[i] = sd;
	}

	for(i=0; i < forks; i++) {
		int r = fork();
		if (r == 0) {
			break;
		}
		list_of_pids[i] = r;
	}

	if (i == forks) {
		printf("started\n");
		while (1) {
			sleep(100);
		}
		return 0;
	}

	while (1) {
		fd_set rfds;
		FD_ZERO(&rfds);
		for (i = 0; i < fd_max; i++) {
			int sd = list_of_sd[i];
			FD_SET(sd, &rfds);
		}
		int r = select(list_of_sd[fd_max-1]+1, &rfds, NULL, NULL, NULL);
		for (i = 0; i < fd_max; i++) {
			int sd = list_of_sd[i];
			if (FD_ISSET(sd, &rfds)) {
				char buf[1024];
				struct sockaddr_in sin;
				int sal = sizeof(sin);
				int l = recvfrom(sd, buf, sizeof(buf), 0,
						 (struct sockaddr*)&sin, &sal);
				if (l > 0) {
					sendto(sd, buf, l, 0, (struct sockaddr*)&sin, sal);
				}
//				break;
			}
		}
	}

	return 0;
}
