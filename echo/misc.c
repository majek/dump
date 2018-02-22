#define _GNU_SOURCE

#include <errno.h>
#include <libmill/ip.h>
#include <libmill/libmill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sched.h>
#include <sys/socket.h>

#include <linux/filter.h>

#ifndef SO_ATTACH_REUSEPORT_CBPF
#define SO_ATTACH_REUSEPORT_CBPF	51
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

int tcpsock_fd(tcpsock a)
{
	int *b = (int *)a;
	return b[1];
}

size_t tcprecvpkt(tcpsock fd, char *buf, size_t buf_len, int64_t deadline,
		  int opportunistic)
{
	if (opportunistic) {
		/* Assume the socket is readable and doesn't need blocking. */
		int r = read(tcpsock_fd(fd), buf, buf_len);
		if (r > 0) {
			errno = 0;
			return r;
		} else if (r == 0) {
			errno = ECONNRESET;
			return 0;
		} else {
			if (errno == EAGAIN) {
				errno = 0;
				// fallthrough
			} else {
				// keep errno
				return 0;
			}
		}
	}

	int events = fdwait(tcpsock_fd(fd), FDW_IN, deadline);
	if ((events & (FDW_IN | FDW_ERR)) != 0) {
		errno = 0;
		int r = read(tcpsock_fd(fd), buf, buf_len);
		if (r > 0) {
			errno = 0;
			return r;
		} else if (r == 0) {
			errno = ECONNRESET;
			return 0;
		} else {
			if (errno == EAGAIN) {
				errno = 0;
			}
			return 0;
		}
	}

	errno = ETIMEDOUT;
	return 0;
}

size_t tcpsendpkt(tcpsock fd, char *buf, size_t buf_len, int64_t deadline,
		  int opportunistic)
{
	if (opportunistic) {
		/* Opportunistic write. Assume write buffer has some space */
		int r = write(tcpsock_fd(fd), buf, buf_len);
		if (r > 0) {
			errno = 0;
			return r;
		} else if (r == 0) {
			errno = ECONNRESET;
			return 0;
		} else { // if (r < 0) {
			if (errno != EAGAIN) {
				return -1;
			}
		}
	}

	int events = fdwait(tcpsock_fd(fd), FDW_OUT, deadline);
	if ((events & (FDW_OUT | FDW_ERR)) != 0) {
		int r = write(tcpsock_fd(fd), buf, buf_len);
		if (r < 0) {
			if (errno == EAGAIN) {
				errno = 0;
			}
			return 0;
		} else if (r == 0) {
			errno = ECONNRESET;
			return 0;
		} else {
			errno = 0;
			return r;
		}
	}

	errno = ETIMEDOUT;
	return 0;
}

ipaddr parse_addr(char *str)
{
	char *colon = strrchr(str, ':');
	if (colon == NULL) {
		fprintf(stderr, "Something's wrong with the port number\n");
		exit(-2);
	}
	int port = atoi(colon + 1);
	if (port < 0 || port > 65535) {
		fprintf(stderr, "Something's wrong with the port number\n");
		exit(-2);
	}
	*colon = '\x00';
	char *host = str;
	if (host[0] == '[' && host[strlen(host) - 1] == ']') {
		host[strlen(host) - 1] = '\x00';
		host = &host[1];
	}
	if (strlen(host) == 0) {
		host = NULL;
	}
	return iplocal(host, port, 0);
}

const char *optstring_from_long_options(const struct option *opt)
{
	static char optstring[256] = {0};
	char *osp = optstring;

	for (; opt->name != NULL; opt++) {
		if (opt->flag == 0 && opt->val > 0 && opt->val < 256) {
			*osp++ = opt->val;
			switch (opt->has_arg) {
			case optional_argument:
				*osp++ = ':';
				*osp++ = ':';
				break;
			case required_argument:
				*osp++ = ':';
				break;
			}
		}
	}
	*osp++ = '\0';

	if (osp - optstring >= (int)sizeof(optstring)) {
		abort();
	}

	return optstring;
}

#define TIMESPEC_NSEC(ts) ((ts)->tv_sec * 1000000000ULL + (ts)->tv_nsec)

uint64_t now_ns()
{
	struct timespec now_ts;
	clock_gettime(CLOCK_MONOTONIC, &now_ts);
	return TIMESPEC_NSEC(&now_ts);
}

int taskset(int taskset_cpu)
{
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(taskset_cpu, &set);
	return sched_setaffinity(0, sizeof(cpu_set_t), &set);
}

size_t sizefromstring(const char *arg)
{
	size_t v = atol(arg);
	switch (arg[strlen(arg) - 1] | 0x20) {
	case 'k':
		v *= 1024L;
		break;
	case 'm':
		v *= 1024 * 1024L;
		break;
	case 'g':
		v *= 1024 * 1024 * 1024L;
		break;
	case 't':
		v *= 1024 * 1024 * 1024 * 1024L;
		break;
	case '0' ... '9':
		break;
	default:
		fprintf(stderr, "Unknown suffix %s\n", arg);
		exit(-3);
	}
	return v;
}

int set_cbpf(int fd, int reuseport_cpus){
	if (reuseport_cpus < 2) {
		errno = 0;
		return -1;
	}
	struct sock_filter code[] = {
                { BPF_LD  | BPF_W | BPF_ABS, 0, 0, SKF_AD_OFF + SKF_AD_CPU },
                { BPF_ALU | BPF_MOD | BPF_K, 0, 0, reuseport_cpus },
	 	{ BPF_RET | BPF_A, 0, 0, 0 },
	};
	struct sock_fprog fprog = {
		.len = ARRAY_SIZE(code),
		.filter = code,
	};

        int r = setsockopt(fd, SOL_SOCKET, SO_ATTACH_REUSEPORT_CBPF,
                              (void*)&fprog, sizeof(fprog));
	return r;

}
