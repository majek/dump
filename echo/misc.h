int tcpsock_fd(tcpsock a);
size_t tcprecvpkt(tcpsock fd, char *buf, size_t buf_len, int64_t deadline,
		  int opportunistic);
size_t tcpsendpkt(tcpsock fd, char *buf, size_t buf_len, int64_t deadline,
		  int opportunistic);
ipaddr parse_addr(char *str);
const char *optstring_from_long_options(const struct option *opt);
uint64_t now_ns();
int taskset(int taskset_cpu);
size_t sizefromstring(const char *arg);
int set_cbpf(int fd, int reuseport_cpus);
