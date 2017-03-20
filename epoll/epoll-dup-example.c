#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

int main() {
	int pfd[2];
	pipe(pfd);

	int rfd = pfd[0];
	int wfd = pfd[1];

	/* Write make the pipe readable. */
	char buf[1] = {0};
	write(wfd, buf, sizeof(buf));

	/*  */
	int epfd = epoll_create(1);
	struct epoll_event ev = {.events = EPOLLIN, .data.fd = rfd};
	epoll_ctl(epfd, EPOLL_CTL_ADD, rfd, &ev);


	int rfd2 = dup(rfd);
	printf("close(fd=%d)\n", rfd);
	close(rfd);

	int r = epoll_wait(epfd, &ev, 1, 1);
	if (r == 0) {
		printf("epoll_wait() finished with no events... okay...\n");
	} else {
		printf("epoll_wait() got Event! on fd %d! wait...\n", ev.data.fd);
	}

	r = epoll_ctl(epfd, EPOLL_CTL_DEL, rfd, NULL);
	if (r != 0) {
		perror("epoll_ctl(EPOLL_CTL_DEL, rfd)");
	}
	r = epoll_ctl(epfd, EPOLL_CTL_DEL, rfd2, NULL);
	if (r != 0) {
		perror("epoll_ctl(EPOLL_CTL_DEL, rfd2)");
	}

	// Bonus points!!!!!
	rfd = dup(rfd2);
	r = epoll_ctl(epfd, EPOLL_CTL_DEL, rfd, NULL);
	if (r == 0) {
		printf("epoll_ctl(EPOLL_CTL_DEL, rfd)... okay? how come?\n");
	}
	return 0;
}
