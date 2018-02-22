#include <errno.h>
#include <libmill/ip.h>
#include <libmill/libmill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <getopt.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "misc.h"

#define READ_BUF_SIZE (128 * 1024)
#define DEFAULT_PORT 1025
#define DEFAULT_SIZE (4L * 1024 * 1024)
#define DEFAULT_NUMBER 100
#define DEFAULT_CONCURRENCY 1
#define DEFAULT_BUF_SIZE (128 * 1024)

static char *write_buffer = NULL;

/* The question is: After a successful write (or read), to a socket,
 * should we retry sending again? Or go back to event loop?
 *
 * Doing the former (prio_to_current = 1) causes pretty bad imbalance
 * between sockets in long run. */
static int prio_to_current = 0;

static int one_direction = 0;

coroutine void read_forever(tcpsock cd, size_t max_size, chan ch,
			    char *buf, size_t buf_size)
{
	size_t n = 0;
	size_t nbytes = 0;

	if (one_direction) {
		errno = 0;
		goto done;
	}

	while (n < max_size) {
		int opportunistic = 0;
		if (prio_to_current) {
			opportunistic = nbytes == buf_size;
		}
		nbytes = tcprecvpkt(cd, buf, buf_size, -1, opportunistic);
		if (errno != 0) {
			break;
		}
		n += nbytes;
	}

done:;
	int e = errno;
	shutdown(tcpsock_fd(cd), SHUT_RD);
	chs(ch, int, e);
}

coroutine void write_forever(tcpsock cd, size_t max_size, chan ch,
			     size_t buf_size)
{
	char * bb = "PROXY TCP4 1.2.3.4 1.2.3.4 11 11\r\n";
	tcpsendpkt(cd, bb, strlen(bb), -1, 1);

	size_t n = 0;
	size_t nbytes = 0;
	while (n < max_size) {
		int sz = buf_size < max_size - n ? buf_size : max_size - n;
		int opportunistic = 0;
		if (prio_to_current) {
			opportunistic = nbytes == buf_size;
		}
		nbytes = tcpsendpkt(cd, write_buffer, sz, -1,
				    opportunistic);
		if (errno != 0) {
			break;
		}
		n += nbytes;
	}
	int e = errno;

	shutdown(tcpsock_fd(cd), SHUT_WR);
	chs(ch, int, e);
}

coroutine static void do_connect(ipaddr target, int64_t max_size, sem_t *sem,
				 chan done, size_t buf_size)
{
	char *buf = malloc(buf_size);

	while (1) {
		if (sem != NULL) {
			if (sem_trywait(sem)) {
				// errno == EAGAIN?
				break;
			}
		}
		uint64_t t0 = now_ns();
		tcpsock cd = tcpconnect(target, -1);
		if (errno != 0) {
			fprintf(stderr, "[!] connection error: %s \n", strerror(errno));
			usleep(1000 * 250); // 250 ms
			continue;
		}

		chan ch = chmake(int, 0);
		go(read_forever(cd, max_size, ch, buf, buf_size));
		go(write_forever(cd, max_size, ch, buf_size));
		int a = chr(ch, int);
		int b = chr(ch, int);
		if (a == b) {
		};
		tcpclose(cd);
		uint64_t t1 = now_ns();
		printf("%li\n", (t1 - t0) / 1000000);
	}
	free(buf);
	chs(done, int, 1);
}

static void connect_loop(ipaddr target, size_t max_size, int concurrency,
			 int cpu, sem_t *sem, size_t buf_size)
{
	if (cpu > -1) {
		// Ignore taskset errors
		taskset(cpu);
	}

	chan done = chmake(int, 0);

	int i;
	for (i = 0; i < concurrency; i++) {
		go(do_connect(target, max_size, sem, done, buf_size));
	}

	for (i = 0; i < concurrency; i++) {
		int a = chr(done, int);
		a = a;
	}
}

static void usage()
{
	fprintf(stderr,
		"Usage:\n"
		"\n"
		"    client [options]\n"
		"\n"
		"Benchmarking client ding simple TCP Echo.\n"
		"\n"
		"Options:\n"
		"\n"
		"  --target             Targret. Default: 127.0.0.1:%d\n"
		"  --size               Number of bytes to exchange over "
		"single\n"
		"                       TCP connection. Default %liM.\n"
		"  --number             Stop after a number of connections.\n"
		"                       Default %i (-1 is unlimited).\n"
		"  --pin-cpu            Pin process to given CPU core.\n"
		"  --concurrency        Number of concurrent connections. "
		"Default: %i\n"
		"  --forks              Spawn given number of processes "
		"(concurrency\n"
		"                       will get divided\n"
		"  --buf-size           Size of the receive / send buffer.\n"
		"  --one-direction      Only send data. Don't wait on receive.\n"
		"  --help               Print this message\n"
		"\n"
		"Example:\n"
		"\n"
		"    client -t 127.0.0.1:5555 -b 1000000 -c 200 -f 4\n"
		"\n",
		DEFAULT_PORT, DEFAULT_SIZE / 1024 / 1024, DEFAULT_NUMBER,
		DEFAULT_CONCURRENCY);
	exit(-1);
}

int main(int argc, char *argv[])
{
	signal(SIGPIPE, SIG_IGN);

	static struct option long_options[] = {
		{"target", required_argument, 0, 't'},
		{"size", required_argument, 0, 's'},
		{"number", required_argument, 0, 'n'},
		{"pin-cpu", required_argument, 0, 'p'},
		{"concurrency", required_argument, 0, 'c'},
		{"forks", required_argument, 0, 'f'},
		{"buf-size", required_argument, 0, 'b'},
		{"one-direction", no_argument, 0, 'o'},
		{"help", no_argument, 0, 'h'},
		{NULL, 0, 0, 0}};

	const char *optstring = optstring_from_long_options(long_options);

	ipaddr target = iplocal("127.0.0.1", DEFAULT_PORT, 0);
	int taskset_cpu = -1;
	int concurrency = DEFAULT_CONCURRENCY;
	int number = DEFAULT_NUMBER;
	int forks = 1;
	size_t max_size = DEFAULT_SIZE;
	size_t buf_size = DEFAULT_BUF_SIZE;

	optind = 1;
	while (1) {
		int arg =
			getopt_long(argc, argv, optstring, long_options, NULL);
		if (arg == -1) {
			break;
		}

		switch (arg) {
		case 't':
			target = parse_addr(optarg);
			break;

		case 's':
			max_size = sizefromstring(optarg);
			break;

		case 'n':
			number = atoi(optarg);
			break;

		case 'p':
			taskset_cpu = atoi(optarg);
			break;

		case 'c':
			concurrency = atoi(optarg);
			break;

		case 'f':
			forks = atoi(optarg);
			break;

		case 'b':
			buf_size = sizefromstring(optarg);
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

	write_buffer = malloc(buf_size);
	memset(write_buffer, 'a', buf_size);

	char ipstr[IPADDR_MAXSTRLEN];
	ipaddrstr(target, ipstr);
	fprintf(stderr,
		"[+] Connecting to %s:%d from: %d connections, %d forks, %ld "
		"bytes per connection\n",
		ipstr, mill_ipport(target), concurrency, forks, max_size);

	sem_t *sem = NULL;
	if (number > -1) {
		sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE,
			   MAP_ANONYMOUS | MAP_SHARED, -1, 0);
		sem_init(sem, 1, number);
	}

	int conc_share = concurrency / forks;
	int i;
	for (i = 0; i < forks; i++) {
		pid_t pid = mfork();
		if (pid < 0) {
			perror("Can't create new process");
			return 1;
		}

		if (i == forks - 1) {
			// last loop
			conc_share += concurrency % forks;
		}

		int cpu = taskset_cpu;
		if (cpu > -1) {
			cpu += i;
		}

		if (pid > 0) {
			/* fprintf(stderr, "[ ] pid=%d concurrency=%d pincpu=%d\n", */
			/* 	pid, conc_share, cpu); */
		} else if (pid == 0) {
			connect_loop(target, max_size, conc_share, cpu, sem,
				     buf_size);
			exit(0);
		}
	}

	for (i = 0; i < forks; i++) {
		int status;
		wait(&status);
	}

	return 0;
}
