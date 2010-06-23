#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "hashes.h"

typedef	u_int32_t (*hash_fun)(unsigned char *buf, int len);

struct hashes {
	char *name;
	hash_fun fun;
};

struct hashes hashes[] = {
	{"uri", &haproxy_uri_hash},
	{"sax", &sax_hash},
	{"pjw", &hashpjw},
	{"djbx33", &hash_djbx33},
	{"bernstein", &bernstein},
	{"superfast", &SuperFastHash},
	{"superfast2", &SuperFastHash2},
	{"fnv_32a", &fnv_32a_str},
	{"word", &hashword},
	{"kr", &kr_hash},
	{"fnv", &fnv_hash},
	{"oat", &oat_hash},
	{"wt", &wt_hash},
	{"sha1", &sha1},
	{NULL, NULL}
};

int strip(char *buf)
{
	int len = strlen(buf) + 1;
	while((len-2 > 0) && (buf[len-2] == '\n')) {
		buf[len-2] = '\0';
		len--;
	}
	return len;
}


int main(int argc, char **argv)
{
	assert(argc==2);
	char *name = argv[1];
	int i;
	hash_fun fun = NULL;
	for (i=0; hashes[i].name; i++) {
		if(!strcmp(name, hashes[i].name)) {
			fun = hashes[i].fun;
			break;
		}
	}
	if (!fun) {
		assert(0);
	}

	while (1) {
		char buf[256];
		char *r = fgets(buf, sizeof(buf), stdin);
		if (r == NULL) {
			break;
		}
		int len = strip(buf);
		printf("%08x\n", fun((unsigned char*)buf, len));
	}
	return 0;
}
