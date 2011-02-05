#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#include "hamt.h"


struct item {
	int counter;
        uint128_t hash;
	char key[];
};

static uint128_t *hash(void *ud, void *ptr)
{
	struct item *item = (struct item*)ptr;
        return &item->hash;
}

static struct {
	int histogram[42];
} mem;

static void *custom_malloc(size_t sz)
{
	int ptr_sz = sizeof(void *);
	int ptrs = (sz+ptr_sz-1)/ptr_sz;
	mem.histogram[ptrs]++;
	return malloc(sz+8);
}

static void custom_free(void *ptr, size_t sz)
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

struct item *new_item(char *key, int len) {
        struct item *item =                                             \
                (struct item *)malloc(sizeof(struct item) + len + 1);
        item->counter = 1;
        memcpy(item->key, key, len);
        item->key[len] = '\0';

        unsigned char digest[20];

	MD5_CTX md5;
	MD5_Init(&md5);
	MD5_Update(&md5, item->key, len);
	MD5_Final(digest, &md5);

        memcpy(&item->hash, &digest, sizeof(item->hash));
	return item;
}

int main()
{
	int items = 0;
	struct hamt_root root = HAMT_ROOT(custom_malloc, custom_free, hash, NULL);

	while (1) {
		char buf[1024];
		char *r = fgets(buf, sizeof(buf), stdin);
		if (r == NULL) {
			break;
		}
		int len = strip(buf);

		struct item *item = new_item(buf, len);
		struct item *inserted = (struct item*)hamt_insert(&root, item);
		if (inserted != item) {
			inserted->counter++;
			free(item);
		} else {
                        items += 1;
                        //struct item *a = (struct item*)hamt_search(&root, &item->hash);
                        //assert(a == item);
                }
	}

	struct hamt_state state;
	void *ptr;
	/* for(ptr = hamt_first(&root, &state); */
	/*     ptr != NULL; */
	/*     ptr = hamt_next(&root, &state)) { */
	/* 	struct item *item = (struct item*)ptr; */
	/* 	printf("%s\n", item->key); */
	/* 	items ++; */
	/* } */

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

		hamt_delete(&root, hash(NULL, ptr));
		free(ptr);
	}

	return 0;
}
