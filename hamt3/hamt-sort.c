#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#include "hamt.h"


struct item {
	int counter;
	char key[];
};

static u64 hash(void *ptr)
{
	struct item *item = (struct item*)ptr;

        unsigned char digest[20];
        /* SHA_CTX sha; */
        /* SHA1_Init(&sha); */
        /* SHA1_Update(&sha, item->key, strlen(item->key)); */
        /* SHA1_Final(digest, &sha); */

	MD5_CTX md5;
	MD5_Init(&md5);
	MD5_Update(&md5, item->key, strlen(item->key));
	MD5_Final(digest, &md5);

        u64 *a = (u64*)(void*)digest;
	return *a;
}

static struct {
	int histogram[66];
} mem;

static void *custom_malloc(int sz)
{
	int ptr_sz = sizeof(void *);
	int ptrs = (sz+ptr_sz-1)/ptr_sz;
	mem.histogram[ptrs]++;
	return malloc(sz);
}

static void custom_free(int sz, void *ptr)
{
	int ptr_sz = sizeof(void *);
	int ptrs = (sz+ptr_sz-1)/ptr_sz;
	mem.histogram[ptrs]--;
	free(ptr);
}


static int strip(char *buf)
{
	int len = strlen(buf) + 1;
	while((len-2 > 0) && (buf[len-2] == '\n')) {
		buf[len-2] = '\0';
		len--;
	}
	return len;
}

int main()
{
	struct hamt_root root = HAMT_ROOT(custom_malloc, custom_free, hash);

	while (1) {
		char buf[1024];
		char *r = fgets(buf, sizeof(buf), stdin);
		if (r == NULL) {
			break;
		}
		int len = strip(buf);

		struct item *item =					\
			(struct item *)malloc(sizeof(struct item) + len + 1);
		item->counter = 1;
		memcpy(item->key, buf, len);
		item->key[len] = '\0';
		struct item *inserted = \
			(struct item*)hamt_insert(&root, item);
		if (inserted != item) {
			inserted->counter++;
			free(item);
		}
	}

	int items = 0;
	struct hamt_state state;
	void *ptr;
	for(ptr = hamt_first(&root, &state);
	    ptr != NULL;
	    ptr = hamt_next(&root, &state)) {
		struct item *item = (struct item*)ptr;
		// printf("%s\n", item->key);
		items ++;
	}

	int i, sum=0;
	for (i=0; i < sizeof(mem.histogram)/sizeof(mem.histogram[0]); i++) {
		int used = (int)sizeof(void*)*i* (mem.histogram[i]);
		fprintf(stderr, "[%i] -> %i (%i bytes)\n",
		       i,
		       mem.histogram[i],
		       used);
		sum += used;
	}
	fprintf(stderr, "%i/%i = %.3f bytes/item\n", sum, items, sum/(float)items);

	while (1) {
		void *ptr = hamt_first(&root, &state);
		if (ptr == NULL) {
			break;
		}

		hamt_delete(&root, hash(ptr));
		free(ptr);
	}

	return 0;
}
