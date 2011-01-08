
#include <stdio.h>

#include <stdint.h>

#include "hamt.h"
#include "hamt-rec.c"

#define start_test(name)			\
	do {					\
	int ok=0, fail=0;			\
	printf("\t%-18s:", name)		\

#define test(cmd)						\
	do {							\
		if(cmd) {					\
			ok++;					\
		} else {					\
			if(!fail) printf("\n");			\
			printf("\t\tFAILED: %s\t  (%s:%i)\n",	\
			       #cmd, __FILE__, __LINE__);	\
			fail++;					\
		}						\
	} while(0)

#define stop_test()				\
	if(!fail) {				\
		printf("%3i ok!\n", ok);	\
	};					\
	} while (0)


static void *custom_malloc(int sz)
{
	return malloc(sz);
}

static void custom_free(int sz, void *ptr)
{
	free(ptr);
}

struct item {
        uint128_t key;
};

static uint128_t *user_hash(void *ptr)
{
        struct item *itm = (struct item*)ptr;
	return &itm->key;
}


int main() {
	setvbuf(stdout, NULL, _IONBF, 0);

	struct hamt_root root;

	start_test("hamt: few items");
	{
                root = HAMT_ROOT(custom_malloc, custom_free, user_hash);
                struct item *i = (struct item*)malloc(sizeof(struct item));
                *i = (struct item){127};
                struct item *j = (struct item*)malloc(sizeof(struct item));
                *j = (struct item){1};
                struct item *k = (struct item*)malloc(sizeof(struct item));
                *k = (struct item){9};
		test(hamt_search(&root, &i->key) == NULL);
		test(hamt_insert(&root, i) == i);
		test(hamt_search(&root, &i->key) == i);

		test(hamt_search(&root, &j->key) == NULL);
		test(hamt_insert(&root, j) == j);
		test(hamt_search(&root, &i->key) == i);
		test(hamt_search(&root, &j->key) == j);
		/* test(hamt_delete(&root, 0xDEAD) == NULL); */
		/* test(hamt_delete(&root, key[0]) == &key[0]); */
		/* test(hamt_delete(&root, key[0]) == NULL); */
		/* test(hamt_search(&root, key[0]) == NULL); */
	}
	stop_test();

        return 0;
}
