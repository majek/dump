#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char **argv)
{
	int forks = 0;
	int fd_max = 1;
	int port = 1024+1;

	if (argc > 1) {
		forks = atoi(argv[1]);
		if (forks < 0 || forks > 1000000) {
			forks = 0;
		}
	}

	if (argc > 2) {
		fd_max = atoi(argv[2]);
		if (fd_max < 0 || fd_max > 10000) {
			fd_max = 1;
		}
	}

	int dup_pipe = 0;
	if (argc > 3) {
		dup_pipe = 1;
	}

	fprintf(stderr, "forks = %d, dupes per fork = %d, total = %d, dup_pipe=%d\n",
		forks, fd_max, forks*fd_max, dup_pipe);
	int *list_of_sd = calloc(1, sizeof(int) * fd_max);
	int *list_of_pids = calloc(1, sizeof(int) * fd_max);

	struct sockaddr_in sin4;
	memset(&sin4, 0, sizeof(sin4));
	sin4.sin_family = AF_INET;
	sin4.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &(sin4.sin_addr));

	struct sockaddr *sockaddr = (struct sockaddr*)&sin4;
	int sockaddr_len = sizeof(sin4);

	if (1) {
		// first socket
		int sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		int one = 1;
		setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
			   sizeof(one));

		bind(sd, sockaddr, sockaddr_len);
		listen(sd, 128);

		int flags = fcntl(sd, F_GETFL, 0);
		fcntl(sd, F_SETFL, flags | O_NONBLOCK);

		list_of_sd[0] = sd;
	}

	int i;
	for(i=0; i < forks; i++) {
		int r = fork();
		if (r == 0) {
			break;
		}
		list_of_pids[i] = r;
	}
	int proc_num = i;

	if (i == forks) {
		// main thread
		fprintf(stderr, "[+] started sd=%d\n", list_of_sd[0]);
		while (1) {
			sleep(100);
		}
		fprintf(stderr, "[+] done\n");
		return 0;
	}

	int pipefd[2];
	int r = pipe(pipefd);
	if (r < 0) {
		perror("pipe");
	}

	for (i = 1; i < fd_max; i++) {
		if (dup_pipe){
			list_of_sd[i] = dup(pipefd[0]);
		} else {
			list_of_sd[i] = dup(list_of_sd[0]);
		}
	}

	while (1) {
		fd_set rfds;
		FD_ZERO(&rfds);
		for (i = 0; i < fd_max; i++) {
			int sd = list_of_sd[i];
			FD_SET(sd, &rfds);
		}

		select(list_of_sd[fd_max-1]+1, &rfds, NULL, NULL, NULL);
		for (i = 0; i < fd_max; i++) {
			int sd = list_of_sd[i];
			if (FD_ISSET(sd, &rfds)) {
				int l = accept(sd, NULL, 0);
				if (l >= 0) {
					printf("received. fd=%d, procnum=%d\n", sd, proc_num);
					close(l);
				}
			}
		}
	}

	return 0;
}
