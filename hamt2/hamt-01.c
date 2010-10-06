#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#include "hamt.h"

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#define BUILD_BUG_ON_ZERO(e) (sizeof(char[1 - 2 * !!(e)]) - 1)
#define __must_be_array(a) \
        BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))


u64 sha1 (char *key, int len)
{
        SHA_CTX sha;
        unsigned char digest[20];

        SHA1_Init(&sha);
        SHA1_Update(&sha, key, len);
        SHA1_Final(digest, &sha);

        u64 *a = (u64*)(void*)digest;

        return a[0];
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

struct item {
	int counter;
	char key[];
};

static u64 hash(void *ptr)
{
	struct item *item = (struct item*)ptr;
	u64 a =  sha1(item->key, strlen(item->key));
	return a;
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
		memcpy(item->key, buf, len+1);

		struct item *inserted = \
			(struct item*)hamt_insert(&root, item);
		if (inserted != item) {
			free(item);
			inserted->counter++;
			if (strcmp(buf, inserted->key) != 0) {
				fprintf(stderr, "Conflict '%s'!='%s'\n", buf, inserted->key);
			}
		}
	}

	int items = 0;
	struct hamt_state state;
	void *ptr = hamt_first(&root, &state);
	while (ptr) {
		struct item *item = (struct item*)ptr;
		ptr = hamt_next(&root, &state);

		#if 0
		printf("%7i %s\n",
		       item->counter,
		       item->key);
		#else
		printf("%li\n", (long)item);
		#endif
		items++;
	}


	#if 1
	int i, sum=0;
	for (i=0; i < ARRAY_SIZE(mem.histogram); i++) {
		int used = (int)sizeof(void*)*i* (mem.histogram[i]);
		printf("[%i] -> %i (%i bytes)\n",
		       i,
		       mem.histogram[i],
		       used);
		sum += used;
	}

	printf("%i/%i = %.3f bytes/item\n", sum, items, sum/(float)items);
	#endif

	while (1) {
		void *ptr = hamt_first(&root, &state);
		if (!ptr) {
			break;
		}

		hamt_delete(&root, hash(ptr));
	}
	for (i=0; i < ARRAY_SIZE(mem.histogram); i++) {
		assert(mem.histogram[i] == 0);
	}

	return 0;
}
