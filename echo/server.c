#include <errno.h>
#include <libmill/ip.h>
#include <libmill/libmill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#include "misc.h"

#define LISTEN_SIZE 1024
#define DEFAULT_PORT 1025
#define DEFAULT_BUF_SIZE (128 * 1024)


/* The question is: After a successful write (or read), to a socket,
 * should we retry sending again? Or go back to event loop?
 *
 * Doing the former (prio_to_current = 1) causes pretty bad imbalance
 * between sockets in long run. */
static int prio_to_current = 0;

static int one_direction = 0;

coroutine void do_handle_client(tcpsock cd, size_t buf_size)
{
	char *buf = malloc(buf_size);
	size_t rbytes = 0, wbytes = 0;
	while (1) {
		int opportunistic = 0;
		if (prio_to_current) {
			opportunistic = rbytes == buf_size && wbytes == buf_size;
		}

		rbytes = tcprecvpkt(cd, buf, buf_size, -1, opportunistic);
		if (errno != 0) {
			goto out;
		}

		if (one_direction) {
			continue;
		}

		size_t p = 0;
		while (p < rbytes) {
			wbytes = tcpsendpkt(cd, &buf[p], rbytes - p, -1, 1);
			if (errno != 0) {
				goto out;
			}
			p += wbytes;
		}
	}
out:
	tcpclose(cd);
	free(buf);
}

static void accept_loop(tcpsock ls, int cpu, size_t buf_size)
{
	if (cpu > -1) {
		// Ignore taskset errors
		taskset(cpu);
	}

	while (1) {
		tcpsock as = tcpaccept(ls, -1);
		// printf("pid=%d accept()\n", getpid());
		go(do_handle_client(as, buf_size));
	}
}

static void usage()
{
	fprintf(stderr,
		"Usage:\n"
		"\n"
		"    server [options]\n"
		"\n"
		"Simple TCP Echo server.\n"
		"\n"
		"Options:\n"
		"\n"
		"  --listen             Listen address. Default: 127.0.0.1:%d\n"
		"  --pin-cpu            Pin process to given CPU core.\n"
		"  --forks              Spawn given number of processes\n"
		"  --buf-size           Size of the receive / send buffer.\n"
		"                       (Userspace. Kernel should do autotune.)\n"
		"  --reuseport          Use multilpe accept queues, and set SO_REUSEPORT.\n"
		"  --one-direction      Only receive data. Don't do send.\n"
		"  --help               Print this message\n"
		"\n"
		"Example:\n"
		"\n"
		"    server -l 127.0.0.1:5555 -f 4\n"
		"\n",
		DEFAULT_PORT);
	exit(-1);
}

int main(int argc, char *argv[])
{
	signal(SIGPIPE, SIG_IGN);

	static struct option long_options[] = {
		{"listen", required_argument, 0, 'l'},
		{"pin-cpu", required_argument, 0, 'p'},
		{"forks", required_argument, 0, 'f'},
		{"buf-size", required_argument, 0, 'b'},
		{"reuseport", no_argument, 0, 'r'},
		{"one-direction", no_argument, 0, 'o'},
		{"help", no_argument, 0, 'h'},
		{NULL, 0, 0, 0}};

	const char *optstring = optstring_from_long_options(long_options);
	ipaddr listen_addr = iplocal("127.0.0.1", DEFAULT_PORT, 0);
	int taskset_cpu = -1;
	int forks = 1;
	size_t buf_size = DEFAULT_BUF_SIZE;
	int reuseport = 0;

	optind = 1;
	while (1) {
		int arg =
			getopt_long(argc, argv, optstring, long_options, NULL);
		if (arg == -1) {
			break;
		}

		switch (arg) {
		case 'l':
			listen_addr = parse_addr(optarg);
			break;

		case 'p':
			taskset_cpu = atoi(optarg);
			break;

		case 'f':
			forks = atoi(optarg);
			break;

		case 'b':
			buf_size = sizefromstring(optarg);
			break;

		case 'r':
			reuseport += 1;
			break;

		case 'o':
			one_direction ++;
			break;


		case 'h':
			usage();
			break;

		case '?':
			exit(-1);
			break;

		case 0:
		default:
			fprintf(stderr, "Unknown option %c: %s", arg,
				argv[optind]);
			exit(-1);
		}
	}

	char ipstr[IPADDR_MAXSTRLEN];
	ipaddrstr(listen_addr, ipstr);
	fprintf(stderr, "[+] Listening on %s:%d, %d forks\n", ipstr,
		mill_ipport(listen_addr), forks);


	int i;
	tcpsock *ls = calloc(forks, sizeof(tcpsock));
	if (reuseport == 0) {
		tcpsock l = tcplisten(listen_addr, LISTEN_SIZE, reuseport);
		if (!l) {
			perror("Can't open listening socket");
			exit(-2);
		}
		for (i = 0; i < forks; i++) {
			ls[i] = l;
		}
	} else {
		/* Must pre-allocate bound sockets first, and in
		 * order. The point is that the kernel REUSEPORT data
		 * structure order is arbitrary, and when we do
		 * RECV_CPU % NUMBER_FORKS, we want to hit the right
		 * worker, pinned on the right CPU. Not some random one. */
		for (i = 0; i < forks; i++) {
			tcpsock l = tcplisten(listen_addr, LISTEN_SIZE, reuseport);
			if (!l) {
				perror("Can't open listening socket");
				exit(-2);
			}
			ls[i] = l;
		}
		if (reuseport > 1) {
			set_cbpf(tcpsock_fd(ls[0]), forks);
		}
	}


	for (i = 0; i < forks; i++) {
		pid_t pid = mfork();
		if (pid < 0) {
			perror("Can't create new process");
			return 1;
		}

		int cpu = taskset_cpu;
		if (cpu > -1) {
			cpu += i;
		}

		if (pid == 0) {
			if (reuseport){
				int j;
				for (j = 0; j< forks; j++) {
					if (j != i) {
						tcpclose(ls[j]);
					}
				}
			}
			accept_loop(ls[i], cpu, buf_size);
		}
	}

	for (i = 0; i < forks; i++) {
		int status;
		wait(&status);
	}

	return 0;
}
