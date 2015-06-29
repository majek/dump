#define _GNU_SOURCE // for SYS_gettid

#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <limits.h>
#include <getopt.h>

#include "common.h"
#include "stddev.h"

struct state
{
	struct net_addr *target_addr;
	int src_port;
	int polling;
	int busy_poll;

	pthread_spinlock_t lock;
	struct stddev stddev;
	struct stddev stddev_packet;
};

long gettid() { return syscall(SYS_gettid); }

#define PKT_SIZE 32

void thread_loop(void *userdata)
{
	struct state *state = userdata;

	pthread_spin_init(&state->lock, PTHREAD_PROCESS_PRIVATE);

	char send_buf[PKT_SIZE], recv_buf[PKT_SIZE];
	struct msghdr *msg = calloc(2, sizeof(struct msghdr));
	struct iovec *iovec = calloc(2, sizeof(struct iovec));
	int i;
	for (i = 0; i < 2; i++) {
		msg[i].msg_iov = &iovec[i];
		msg[i].msg_iovlen = 1;
		iovec[i].iov_len = sizeof(send_buf);
	}

	iovec[0].iov_base = send_buf;
	iovec[1].iov_base = recv_buf;

	char pktinfo[4096];
	msg[1].msg_control = pktinfo;
	msg[1].msg_controllen = sizeof(pktinfo);

	uint64_t packet_no = 0;
	while (1) {
		int fd = net_connect_udp(state->target_addr, state->src_port,
					 state->busy_poll);

		struct timeval tv = {1, 0}; // 1 second
		int r = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv,
				   sizeof(tv));
		if (r < 0) {
			PFATAL("setsockopt(SO_RCVTIMEO)");
		}

		for (;; packet_no++) {
			memset(send_buf, 0, sizeof(send_buf));
			snprintf(send_buf, sizeof(send_buf), "%i-%li-%lu",
				 getpid(), gettid(), packet_no);

			uint64_t t0 = realtime_now(), t1 = 0, tp = 0;
			int r = sendmsg(fd, &msg[0], 0);
			if (r <= 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK ||
				    errno == ECONNREFUSED) {
					break;
				}
				if (errno == EINTR) {
					continue;
				}
				PFATAL("sendmmsg()");
			}

			msg[1].msg_controllen = sizeof(pktinfo);
			while (1) {
				int flags = state->polling ? MSG_DONTWAIT : 0;
				r = recvmsg(fd, &msg[1], flags);
				t1 = realtime_now();
				if (t1 - t0 >= 1000000000) {
					/* No msg for 1s */
					errno = ECONNREFUSED;
					break;
				}

				if (r <= 0 &&
				    (errno == EINTR || errno == EAGAIN ||
				     errno == EWOULDBLOCK)) {
					continue;
				}

				if (r == 0 && state->polling) {
					continue;
				}
				break;
			}

			if (r <= 0 && errno == ECONNREFUSED) {
				sleep(1);
				break;
			}

			if (r <= 0) {
				PFATAL("recvmmsg()");
			}

			if (memcmp(send_buf, recv_buf, sizeof(recv_buf)) != 0) {
				fprintf(stderr, "[!] bad message\n");
				sleep(1);
				break;
			}

			struct cmsghdr *cmsg;
			for (cmsg = CMSG_FIRSTHDR(&msg[1]); cmsg;
			     cmsg = CMSG_NXTHDR(&msg[1], cmsg)) {
				switch (cmsg->cmsg_level) {
				case SOL_SOCKET:
					switch (cmsg->cmsg_type) {
					case SO_TIMESTAMPNS: {
						struct timespec *ts =
							(struct timespec *)
							CMSG_DATA(cmsg);
						tp = TIMESPEC_NSEC(ts);
						break;
					}
					}
					break;
				}
			}

			pthread_spin_lock(&state->lock);
			stddev_add(&state->stddev, t1 - t0);
			if (tp != 0) {
				stddev_add(&state->stddev_packet, t1 - tp);
			}
			pthread_spin_unlock(&state->lock);
		}
		close(fd);
	}
}

int main(int argc, const char *argv[])
{
	int base_src_port = 65500;
	int polling = 0;
	int busy_poll = 0;
	int packet_timestamp = 0;

	static struct option long_options[] = {
		{"src-port", required_argument, 0, 's'},
		{"polling", no_argument, 0, 'p'},
		{"busy-poll", required_argument, 0, 'b'},
		{"timestamp", no_argument, 0, 't'},
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
		case 's':
			base_src_port = atoi(optarg);
			break;

		case 'p':
			polling = 1;
			break;

		case 'b':
			busy_poll = atoi(optarg);
			break;

		case 't':
			packet_timestamp = 1;
			break;

		default:
			FATAL("Unknown option %c: %s", arg, argv[optind]);
		}
	}

	int thread_num = argc - optind;
	if (thread_num < 1) {
		FATAL("Usage: %s [--polling] [--busy-poll=<>] [--src-port=<>] "
		      "[--timestamp] [target:port...]",
		      argv[0]);
	}
	struct net_addr *target_addrs =
		calloc(thread_num, sizeof(struct net_addr));

	int t;
	for (t = 0; t < thread_num; t++) {
		const char *target_addr_str = argv[optind + t];
		parse_addr(&target_addrs[t], target_addr_str);

		fprintf(stderr, "[*] Sending to %s, polling=%i, src_port=%i\n",
			addr_to_str(&target_addrs[t]), polling,
			base_src_port + t);
	}

	struct state *array_of_states =
		calloc(thread_num, sizeof(struct state));

	for (t = 0; t < thread_num; t++) {
		struct state *state = &array_of_states[t];
		state->target_addr = &target_addrs[t];
		state->polling = polling;
		state->busy_poll = busy_poll;
		if (base_src_port > 0) {
			state->src_port = base_src_port + t;
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

		for (t = 0; t < thread_num; t++) {
			struct state *state = &array_of_states[t];

			uint64_t pps;
			double avg, stddev;
			double avg_packet, stddev_packet;

			pthread_spin_lock(&state->lock);
			stddev_get(&state->stddev, &pps, &avg, &stddev);
			int min = state->stddev.min;
			stddev_get(&state->stddev_packet, NULL, &avg_packet,
				   &stddev_packet);
			stddev_init(&state->stddev);
			stddev_init(&state->stddev_packet);
			pthread_spin_unlock(&state->lock);

			printf("pps=%6lu avg=%7.3fus dev=%7.3fus min=%6.3fus  ",
			       pps, avg / 1000., stddev / 1000.,
			       (double)min / 1000.);
			if (packet_timestamp) {
				printf("(packet=%5.3fus/%5.3f)  ",
				       avg_packet / 1000.,
				       stddev_packet / 1000.);
			}
		}
		printf("\n");
	}
	return 0;
}
