/*
 *  gcc flood_mmap.c -l pcap -O3 -o flood_mmap
 */
#include <arpa/inet.h>
#include <ctype.h>
#include <getopt.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <pcap/pcap.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>


typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

struct ipv4_hdr
{

	u8 ver_hlen;   /* IP version (4), IP hdr len in dwords (4) */
	u8 tos_ecn;    /* ToS field (6), ECN flags (2)             */
	u16 tot_len;   /* Total packet length, in bytes            */
	u16 id;	/* IP ID                                    */
	u16 flags_off; /* Flags (3), fragment offset (13)          */
	u8 ttl;	/* Time to live                             */
	u8 proto;      /* Next protocol                            */
	u16 cksum;     /* Header checksum                          */
	u8 src[4];     /* Source IP                                */
	u8 dst[4];     /* Destination IP                           */

	/* Dword-aligned options may follow. */

} __attribute__((packed));

struct ipv6_hdr
{

	u32 ver_tos; /* Version (4), ToS (6), ECN (2), flow (20) */
	u16 pay_len; /* Total payload length, in bytes           */
	u8 proto;    /* Next protocol                            */
	u8 ttl;      /* Time to live                             */
	u8 src[16];  /* Source IP                                */
	u8 dst[16];  /* Destination IP                           */

	/* Dword-aligned options may follow if proto != PROTO_TCP and are
	   included in total_length; but we won't be seeing such traffic due
	   to BPF rules. */

} __attribute__((packed));

#define TIMESPEC_NSEC(ts) ((ts)->tv_sec * 1000000000ULL + (ts)->tv_nsec)

static uint16_t ip_checksum(const void *buf, int len)
{
	uint32_t cksum = 0;
	uint16_t *sp = (uint16_t *)buf;
	int n, sn;

	sn = (int)len / 2;
	n = (sn + 15) / 16;

	switch (sn % 16) {
	case 0:
		do {
			cksum += *sp++;
		case 15:
			cksum += *sp++;
		case 14:
			cksum += *sp++;
		case 13:
			cksum += *sp++;
		case 12:
			cksum += *sp++;
		case 11:
			cksum += *sp++;
		case 10:
			cksum += *sp++;
		case 9:
			cksum += *sp++;
		case 8:
			cksum += *sp++;
		case 7:
			cksum += *sp++;
		case 6:
			cksum += *sp++;
		case 5:
			cksum += *sp++;
		case 4:
			cksum += *sp++;
		case 3:
			cksum += *sp++;
		case 2:
			cksum += *sp++;
		case 1:
			cksum += *sp++;

		} while (--n > 0);
	}
	if (len & 1)
		cksum += htons(*(uint8_t *)sp << 8);

	cksum = (cksum >> 16) + (cksum & 0xffff);
	return ~cksum & 0xffff;
	return (~(cksum + (cksum >> 16)) & 0xffff);
}

/* uint16_t ip_checksumx(const uint8_t *buf, int len) { */
/* 	uint32_t cksum = 0; */
/* 	uint16_t *sp = (uint16_t *)buf; */

/* 	int i; */
/* 	for (i = 0; i < len/2; i+=1) { */
/* 		cksum += sp[i]; */
/* 	} */
/* 	if (len & 1) { */
/* 		cksum += *(buf + len - 1); */
/* 	} */
/* 	cksum = (cksum >> 16) + (cksum & 0xffff); */
/* 	cksum = (cksum >> 16) + (cksum & 0xffff); */
/* 	return ~cksum & 0xffff; */
/* } */

static int looks_like_ip(const u8 *data, int size)
{
	struct ipv4_hdr *ip4 = (void *)(data);
	struct ipv6_hdr *ip6 = (void *)(data);

	// Check if it looks like IP with no trailing data.
	if ((int)sizeof(struct ipv4_hdr) <= size &&
	    (ip4->ver_hlen & 0xF0) == 0x40 && (ip4->ver_hlen & 0x0F) >= 5 &&
	    (ip4->ver_hlen & 0x0F) * 4 <= size && ntohs(ip4->tot_len) == size &&
	    ip_checksum(ip4, (ip4->ver_hlen & 0x0F) * 4) == 0x0000 &&
	    ip4->ttl != 0) {
		return 4;
	}

	if ((int)sizeof(struct ipv6_hdr) <= size &&
	    (ip6->ver_tos & 0xF0) == 0x60 &&
	    ntohs(ip6->pay_len) + (int)sizeof(struct ipv6_hdr) == size &&
	    ip6->ttl != 0) {
		return 6;
	}
	return -1;
}

int find_ip_offset(const u8 *data, int size)
{
	int o;

	// look for ethernet ip types first
	for (o = 2; o < 42; o += 2) {
		if (data[o - 2] == '\x08' && data[o - 1] == '\x00' &&
		    looks_like_ip(data + o, size - o) == 4) {
			return o;
		}
		if ((char)data[o - 2] == '\x86' &&
		    (char)data[o - 1] == '\xdd' &&
		    looks_like_ip(data + o, size - o) == 6) {
			return o;
		}
	}

	// ignore ethernet, just look for IP
	for (o = 2; o < 40 + 2; o += 2) {
		if (looks_like_ip(data + o, size - o) != -1) {
			return o;
		}
	}

	return -1;
}

#define SHOUT(x...) fprintf(stderr, x)
#define ERRORF(x...) fprintf(stderr, x)

#define FATAL(x...)                                                            \
	do {                                                                   \
		ERRORF("[-] PROGRAM ABORT : " x);                              \
		ERRORF("\n         Location : %s(), %s:%u\n\n", __FUNCTION__,  \
		       __FILE__, __LINE__);                                    \
		exit(EXIT_FAILURE);                                            \
	} while (0)

#define PFATAL(x...)                                                           \
	do {                                                                   \
		ERRORF("[-] SYSTEM ERROR : " x);                               \
		ERRORF("\n        Location : %s(), %s:%u\n", __FUNCTION__,     \
		       __FILE__, __LINE__);                                    \
		perror("      OS message ");                                   \
		ERRORF("\n");                                                  \
		exit(EXIT_FAILURE);                                            \
	} while (0)


#ifndef PACKET_QDISC_BYPASS
#  define PACKET_QDISC_BYPASS 20
#endif


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


static void usage() {
        ERRORF(
"Usage:\n"
"\n"
"    flood_mmap [-s] [-i interface] [-s pps] [PCAP FILE [PCAP FILE ...]]\n"
"\n"
"Options:\n"
"\n"
"  --verbose,-v         Print more stuff. Repeat for debugging\n"
"  --help               Print this message\n"
"  -i                   Interface to send on\n"
"  -s                   Rewrite source mac\n"
"\n"
                );
        exit(EXIT_FAILURE);
}


int main(int argc, const char *argv[]) {
	int verbose = 0, rewrite_sourcemac = 0;
	const char *interface = NULL;

	static struct option long_options[] = {
		{"interface",  required_argument, 0, 'i' },
		{"help",       no_argument,       0, 'h' },
		{"verbose",    no_argument,       0, 'v' },
		{"sourcemac",  no_argument,       0, 's' },
		{NULL,         0,                 0,  0  }
	};
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
		case 'v':
			verbose += 1;
			break;

		case 'h':
			usage();
			break;

		case 'i':
			interface = optarg;
			break;

		case 's':
			rewrite_sourcemac = 1;
			break;

		case '?':
			exit(-1);

		default:
			FATAL("Unrecognized option %c: %s", arg, argv[optind]);
		}
	}


	if (interface == NULL) {
		FATAL("You need to specify outgoing interface with \"-i\" "
		      "option, for example \"-i eth0\".");
	}

	SHOUT("[ ] Interface %s", interface);


	int if_socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(if_socket == -1) {
		PFATAL("socket(PF_PACKET, SOCK_RAW)");
	}

	int r;

	struct ifreq s_ifr;
	memset(&s_ifr, 0, sizeof(s_ifr));
	strncpy(s_ifr.ifr_name, interface, sizeof(s_ifr.ifr_name));
	r = ioctl(if_socket, SIOCGIFINDEX, &s_ifr);
	if (r != 0) {
		PFATAL("ioctl(SIOCGIFINDEX, \"%s\")", interface);
	}

	struct sockaddr_ll my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sll_family = PF_PACKET;
	my_addr.sll_protocol = htons(ETH_P_ALL);
	my_addr.sll_ifindex =  s_ifr.ifr_ifindex;
	r = bind(if_socket, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_ll));
	if (r != 0) {
		PFATAL("bind(\"%s\", AF_PACKET)", interface);
	}

	int tmp = 1024*1024;a
	r = setsockopt(if_socket, SOL_SOCKET, SO_SNDBUF, &tmp, sizeof(tmp));
	if (r != 0) {
		PFATAL("setsockopt(SO_SNDBUF)");
	}

	/* Ignore send errors (wrong packets). */
	tmp = 1;
	r = setsockopt(if_socket, SOL_PACKET, PACKET_LOSS, &tmp, sizeof(tmp));
	if (r != 0) {
		SHOUT("[!] PACKET_LOSS sockopt not supported");
	}

	/* Skip qdisc */
	tmp = 1;
	r = setsockopt(if_socket, SOL_PACKET, PACKET_QDISC_BYPASS, &tmp, sizeof(tmp));
	if (r != 0) {
		SHOUT("[!] PACKET_QDISC_BYPASS sockopt not supported");
	}


	/*  */
	/* struct ifreq ifr; */
	/* memset(&s_ifr, 0, sizeof(s_ifr)); */
	/* strncpy(s_ifr.ifr_name, interface, sizeof(s_ifr.ifr_name)); */
	r = ioctl(if_socket, SIOCGIFHWADDR, &s_ifr);
	if (r != 0) {
		PFATAL("ioctl(SIOCGIFHWADDR)");
	}

	char source_mac[6];
	memcpy(source_mac, s_ifr.ifr_hwaddr.sa_data, 6);


	int frame_sz = 1024;
	unsigned int max_payload_sz = frame_sz - sizeof(struct tpacket_hdr);


	int packets_len = 0;
	int packets_cap = 1024;

	struct packet {
		u8 *data;
		int len;
	};
	struct packet *packets = realloc(NULL, sizeof(struct packet) * packets_cap);

	const char **argv_files = &argv[optind];
	if (*argv_files == NULL) {
		argv_files = (const char *[]){"-", NULL};
	}

	for (;*argv_files; argv_files++) {
		char errbuf[PCAP_ERRBUF_SIZE];
		if (strcmp(*argv_files, "-") == 0) {
			SHOUT("[*] Reading pcap from stdin");
		} else {
			SHOUT("[*] Reading pcap file \"%s\"", *argv_files);
		}
		pcap_t *pcap = pcap_open_offline(*argv_files, errbuf);
		if (pcap == NULL) {
			FATAL("pcap_open_offline(): %s", errbuf);
		}

		struct pcap_pkthdr *h;
		const u8 *data;

		int ip_offset = -1;
		int ip_offset_failures = 0;

		while (1) {
			unsigned int caplen;
			switch (pcap_next_ex(pcap, &h, &data)) {
			case 1:
				if (ip_offset < 0) {
					ip_offset = find_ip_offset(data, h->caplen);
					if (ip_offset < 0) {
						if (++ip_offset_failures > 5) {
							FATAL("Can't find a valid IP header ip %i packets. Giving up.",
							      ip_offset_failures);
						}
						break;
					}
					SHOUT("[ ] L3 offset is %i", ip_offset);
					if (ip_offset != 14) {
						FATAL("[!] Is this pcap ethernet?");
					}
				}

				caplen = h->caplen;
				if(caplen < h->len) {
					SHOUT("[ ] Packet cut (%i bytes unsaved), skipping", h->len - caplen);
					break;
				}

				if (caplen > max_payload_sz) {
					SHOUT("[ ] Packet too long (%i bytes), skipping", caplen);
					break;
				}

				if (caplen < 15) {
					SHOUT("[ ] Packet too short (%i bytes), skipping", caplen);
					break;
				}

				if (packets_len == packets_cap) {
					packets_cap *= 2;
					packets = realloc(packets, sizeof(struct packet) * packets_cap);
				}
				u8 *b = malloc(caplen);
				memcpy(b, data, caplen);
				packets[packets_len].data = b;
				packets[packets_len].len = caplen;
				packets_len += 1;

				break;

			case 0:
				// timeout
				break;

			case -1:
				FATAL("pcap_next_ex(): %s", pcap_geterr(pcap));
				break;

			case -2:
				goto next_file;
			}
		}
	next_file:;
	}

	SHOUT("[ ] Loaded %i packets", packets_len);


	struct tpacket_req req;
	memset(&req, 0, sizeof(req));
	req.tp_block_size = 4*1024*1024;
	req.tp_frame_size = frame_sz;
	int frames_per_block = req.tp_block_size / frame_sz;
	req.tp_block_nr = (packets_len+(frames_per_block-1)) / frames_per_block;
	req.tp_frame_nr = frames_per_block * req.tp_block_nr;

	r = setsockopt(if_socket, SOL_PACKET, PACKET_TX_RING, (void *) &req, sizeof(req));
	if (r != 0) {
		PFATAL("setsockopt(PACKET_TX_RING)");
	}

	/* calculate memory to mmap in the kernel */
	int size = req.tp_block_size * req.tp_block_nr;
	//data_offset = TPACKET_HDRLEN - sizeof(struct sockaddr_ll);
	int data_offset = TPACKET_ALIGN(sizeof(struct tpacket_hdr));

	/* mmap Tx ring buffers memory */
	void *ps_header_start = mmap(0, size, PROT_READ|PROT_WRITE,
				     MAP_SHARED | MAP_LOCKED | MAP_POPULATE, if_socket, 0);
	if (ps_header_start == MAP_FAILED) {
		PFATAL("mmap()");
	}
	memset(ps_header_start, 0, size);

	int i;
	char *frame;
	for (frame = ps_header_start, i=0; i < (int)req.tp_frame_nr; i++, frame += frame_sz) {
		struct tpacket_hdr *tp = (void*) frame;
		char *data = frame + data_offset;

		struct packet *p = &packets[i % packets_len];

		switch((volatile uint32_t)tp->tp_status) {
		case TP_STATUS_AVAILABLE:
			memcpy(data, p->data, p->len);
			if (rewrite_sourcemac) {
				memcpy(data+6, source_mac, 6);
			}
			tp->tp_len = p->len;
			break;
		default:
			FATAL("");
		}
	}


	struct timespec ts;
	long long t0, t1, td;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	t0 = TIMESPEC_NSEC(&ts);

	int count = 0;
	while (1) {
		__sync_synchronize();
		for (frame = ps_header_start, i=0; i < (int)req.tp_frame_nr; i++, frame += frame_sz) {
			struct tpacket_hdr *tp = (void*) frame;
			if ((volatile uint32_t)tp->tp_status == TP_STATUS_AVAILABLE) {
				tp->tp_status = TP_STATUS_SEND_REQUEST;// | TP_STATUS_CSUMNOTREADY;
				count += 1;
			}
		}
		__sync_synchronize();

		r = sendto(if_socket, NULL, 0, 0, // MSG_DONTWAIT,
			   (struct sockaddr *)NULL, sizeof(struct sockaddr_ll));

		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		t1 = TIMESPEC_NSEC(&ts);
		td = t1 - t0;
		if (td > 1000000000) {
			/* struct tpacket_stats stats; */
			/* unsigned int len = sizeof(stats); */
			/* r = getsockopt(if_socket, SOL_PACKET, PACKET_STATISTICS, &stats, &len); */
			/* if (r != 0) { */
			/* 	PFATAL("getsockopt(PACKET_STATISTICS)"); */
			/* } */

			//printf("pps: %f\n", (float)((int)req.tp_frame_nr * count) * 1000000000. / (float)td );
			//printf("tx: %u  drops: %u\n", stats.tp_packets, stats.tp_drops);
			printf("tx: %f.1\n", (float)count * 1000000000. / (float)td);
			t0 = t1;
			count = 0;
		}
	}

	return 0;
}
