#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#include "common.h"

#define MTU_SIZE (2048 - 64 * 2)

struct state
{
	int fd;
	int polling;
	union
	{
		struct sockaddr_in sin4;
		struct sockaddr_in6 sin6;
	} src_addr;
	struct msghdr msg;
	char buffer[MTU_SIZE];
	struct iovec iovec;
	char pktinfo[4096];

} __attribute__((aligned(64)));

struct state *state_init(struct state *s)
{
	struct msghdr *msg = &s->msg;

	msg->msg_name = &s->src_addr;
	msg->msg_namelen = sizeof(s->src_addr);
	msg->msg_iov = &s->iovec;
	msg->msg_iovlen = 1;
	msg->msg_control = &s->pktinfo[0];
	msg->msg_controllen = sizeof(s->pktinfo);

	s->iovec.iov_base = &s->buffer[0];
	s->iovec.iov_len = MTU_SIZE;
	return s;
}

static void thread_loop(void *userdata)
{
	struct state *state = userdata;

	while (1) {
		int flags = state->polling ? MSG_DONTWAIT : 0;
		int r = recvmsg(state->fd, &state->msg, flags);
		if (r <= 0 && (errno == EAGAIN || errno == EWOULDBLOCK ||
			       errno == EINTR)) {
			continue;
		}
		if (r == 0 && state->polling) {
			continue;
		}

		if (r <= 0) {
			PFATAL("recvmmsg()");
		}

		struct msghdr *msg = &state->msg;
		msg->msg_iov->iov_len = r;

		int s = sendmsg(state->fd, &state->msg, 0);
		if (s != r) {
			PFATAL("sendmmsg()");
		}
		msg->msg_iov->iov_len = MTU_SIZE;
		msg->msg_controllen = sizeof(state->pktinfo);
	}
}

int main(int argc, const char *argv[])
{
	const char *listen_addr_str = "0.0.0.0:4321";
	int thread_num = 1;
	int reuseport = 0;
	int polling = 0;
	int busy_poll = 0;

	static struct option long_options[] = {
		{"busy-poll", required_argument, 0, 'b'},
		{"threads", required_argument, 0, 't'},
		{"polling", no_argument, 0, 'p'},
		{"reuseport", no_argument, 0, 'r'},
		{"listen", required_argument, 0, 'l'},
		{NULL, 0, 0, 0}};
	const char *optstring = optstring_from_long_options(long_options);

	optind = 1;
	while (1) {
		int option_index = 0;
		int arg = getopt_long(argc, (char **)argv, optstring,
				      long_options, &option_index);
		if (arg == -1) {
			break;
		}

		switch (arg) {
		case 'b':
			busy_poll = atoi(optarg);
			break;

		case 't':
			thread_num = atoi(optarg);
			break;

		case 'p':
			polling = 1;
			break;

		case 'r':
			reuseport = 1;
			break;

		case 'l':
			listen_addr_str = optarg;
			break;

		default:
			FATAL("Unknown option %c: %s", arg, argv[optind]);
		}
	}

	if (optind != argc) {
		FATAL("Usage: %s [--buf-size=<>] [--threads=<>] [--polling] "
		      "[--reuseport] [--listen=]",
		      argv[0]);
	}

	struct net_addr listen_addr;
	parse_addr(&listen_addr, listen_addr_str);

	int main_fd = -1;
	if (reuseport == 0) {
		fprintf(stderr,
			"[*] Starting udpreceiver on %s, "
			"polling=%i, busy_poll=%i, threads=%i, reuseport=%i\n",
			addr_to_str(&listen_addr), polling, busy_poll,
			thread_num, reuseport);

		main_fd = net_bind_udp(&listen_addr, 0, busy_poll);
	}

	struct state *array_of_states =
		calloc(thread_num, sizeof(struct state));

	int t;
	for (t = 0; t < thread_num; t++) {
		struct state *state = &array_of_states[t];
		state_init(state);
		state->polling = polling;
		if (reuseport == 0) {
			state->fd = main_fd;
		} else {
			fprintf(stderr, "[*] Starting udpreceiver on %s\n",
				addr_to_str(&listen_addr));

			int fd = net_bind_udp(&listen_addr, 1, busy_poll);
			state->fd = fd;
		}
		thread_spawn(thread_loop, state);
	}

	while (1) {
		struct timeval timeout = NSEC_TIMEVAL(MSEC_NSEC(1000UL));
		while (1) {
			int r = select(0, NULL, NULL, NULL, &timeout);
			if (r != 0) {
				continue;
			}
			if (TIMEVAL_NSEC(&timeout) == 0) {
				break;
			}
		}
	}

	return 0;
}
