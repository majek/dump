#include <stdio.h>

#include "hamt.h"
#include "hamt.c"

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

static u64 direct_hash(void *ptr)
{
	u64 *v = (u64*)ptr;
	return *v;
}


int main()
{
	setvbuf(stdout, NULL, _IONBF, 0);

	printf("Running tests:\n");

	/* To avoid gcc warnings on big integer shifts. */
	int v0 = 0;
	int v63 = 63;
	int v64 = 64;
	int v65 = 65;

	start_test("set_count");
	{
		test(set_count(0) == 0);
		test(set_count(1) == 1);
		test(set_count(255) == 8);
		test(set_count(256) == 1);
		test(set_count(0xFFFFFFFFLL) == 32);
		test(set_count(0xFFFFFFFF) == 32);
		test(set_count(0xFFFFFFFFFFFFFFFFLL) == 64);
		test(set_count(0x8000000000000001LL) == 2);
		test(set_count(~0LL) == 64);
		test(set_count(-1LL) == 64);
	}
	stop_test();

	start_test("set_next");
	{
		test(set_next(0, 0) == -1);
		test(set_next(0, v63) == -1);
		test(set_next(0, v64) == -1);
		test(set_next(0, v65) == -1);
		test(set_next(0x80000000, 0) == 31);
		test(set_next(0x00000001, 0) == -1);
		test(set_next(0x00000001, 1) == -1);
		test(set_next(0x80000001, 30) == 31);
		test(set_next(0x8000000000000000ULL, 0) == 63);
		test(set_next(0x0000000100000000ULL, 1) == 32);
		test(set_next(0x0000000100000000ULL, 31) == 32);
		test(set_next(0x0000000100000000ULL, 32) == -1);
		test(set_next(0x8000000100000000ULL, 32) == 63);
		test(set_next(0x8000000100000000ULL, 62) == 63);
		test(set_next(0x8000000100000000ULL, v63) == -1);
	}
	stop_test();

	start_test("set_first");
	{
		test(set_first(1) == 0);
		test(set_first(8) == 3);
		test(set_first(0xF0) == 4);
		test(set_first(0x8000000000000000ULL) == 63);
		test(set_first(0) == -1);
	}
	stop_test();

	start_test("set_last");
	{
		test(set_last(1) == 0);
		test(set_last(2) == 1);
		test(set_last(0) == -1); // actually, it's unspecified
		test(set_last(0x8000000000000000ULL) == 63);
		test(set_last(0x7000000000000000ULL) == 62);
	}
	stop_test();

	start_test("set_count_bigger");
	{
		test(set_count_bigger(0x8000000000000000ULL, 0) == 1);
		test(set_count_bigger(0x8000000000000000ULL, v63) == 0);
		test(set_count_bigger(0x8000000000000001ULL, 62) == 1);
		test(set_count_bigger(~0ULL, 0) == 63);
		test(set_count_bigger(~0ULL, v63) == 0);
		test(set_count_bigger(255, 7) == 0);
		test(set_count_bigger(255, 1) == 6);
	}
	stop_test();

	start_test("set_contains");
	{
		test(set_contains(0, 0) == 0);
		test(set_contains(1, 0) == 1);
		test(set_contains(0x8000000000000000ULL, v64) == 0);
		test(set_contains(0x8000000000000000ULL, 63) == 1);
	}
	stop_test();

	start_test("set_add");
	{
		test(set_add(0, 0) == 1);
		test(set_add(0, 1) == 2);
		test(set_add(0xFFFFFFFFFFFFFFFFULL, 1) == 0xFFFFFFFFFFFFFFFFULL);
		test(set_add(0xFFFFFFFFFFFFFFF0ULL, 0) == 0xFFFFFFFFFFFFFFF1ULL);
		test(set_add(0x8000000000000000ULL, 0) == 0x8000000000000001ULL);
		test(set_add(0, 63) == 0x8000000000000000ULL);
		test(set_add(1, 63) == 0x8000000000000001ULL);
		//test(set_add(1, v64) == 1);
	}
	stop_test();

	start_test("set_del");
	{
		test(set_del(0, 0) == 0);
		test(set_del(2, 1) == 0);
		test(set_del(0xFFFFFFFFFFFFFFFFULL, 1) == 0xFFFFFFFFFFFFFFFDULL);
		test(set_del(0xFFFFFFFFFFFFFFF1ULL, 0) == 0xFFFFFFFFFFFFFFF0ULL);
		test(set_del(0x8000000000000001ULL, 0) == 0x8000000000000000ULL);
		test(set_del(0x8000000000000000ULL, 63) == 0);
		test(set_del(0x8000000000000001ULL, 63) == 1);
		//test(set_del(1, v64) == 1);
	}
	stop_test();

	start_test("slice_get");
	{
		test(slice_get(0x7f, 10) == 0x3f);
		test(slice_get(0x7f, 9) == 0x1);
		test(slice_get(0x7f, 8) == 0);
		test(slice_get(0x7f, v0) == 0);
		test(slice_get(0x7000000000000000ULL, 0) == 0x7);
		test(slice_get(0xFFFF000000000000ULL, 0) == 0x0f);
		test(slice_get(0xFFFF000000000000ULL, 1) == 0x3f);
		test(slice_get(0xFFFF000000000000ULL, 2) == 0x3f);
		test(slice_get(0xFFFF000000000000ULL, 3) == 0x0);
		test(slice_get(0xFFFF810000000000ULL, 3) == 0x20);
		test(slice_get(0xFFFF810000000000ULL, 4) == 0x10);
	}
	stop_test();

	start_test("slice_set");
	{
		//test(slice_set(0, 1, 11) == 0x0);
		test(slice_set(0, 1, 10) == 0x1);
		test(slice_set(0, 1, 9) == 0x40);
		test(slice_set(0, 0x3f, 0) == 0xF000000000000000ULL);
		test(slice_set(0, 0x3f, 8) == 0x3f << (6*2));
		test(slice_set(0, 0x3f, 7) == 0x3f << (6*3));
		test(slice_set(0, 0x3f, 1) == 0x3fULL << (6*9));
		test(slice_set(0, 0x3f, 0) == 0x3fULL << (6*10));
		test(slice_set(0, 0x39, 0) == 0x9ULL << (6*10));
		test(slice_set(0, 0x3E, 0) == 0xE000000000000000ULL);
		test(slice_set(1, 1, 10) == 0x1);
		test(slice_set(1, 1, 9) == 0x41);
		test(slice_set(0x8000000000000000ULL, 1, 9) == 0x8000000000000040ULL);
	}
	stop_test();

	struct hamt_root root;
	struct hamt_state s;
	start_test("hamt: single item");
	{
		root = HAMT_ROOT(custom_malloc, custom_free, direct_hash);
		u64 key[] = {0};
		test(hamt_search(&root, key[0]) == NULL);
		test(hamt_insert(&root, &key[0]) == &key[0]);
		test(hamt_search(&root, key[0]) == &key[0]);
		test(hamt_delete(&root, 0xDEAD) == NULL);
		test(hamt_delete(&root, key[0]) == &key[0]);
		test(hamt_delete(&root, key[0]) == NULL);
		test(hamt_search(&root, key[0]) == NULL);
		test(hamt_first(&root, &s) == NULL);
	}
	stop_test();

	start_test("hamt: few items");
	{
		root = HAMT_ROOT(custom_malloc, custom_free, direct_hash);
		u64 key[] = {0, 1, 2, 2};
		test(hamt_search(&root, key[0]) == NULL);
		test(hamt_insert(&root, &key[0]) == &key[0]);
		test(hamt_insert(&root, &key[1]) == &key[1]);
		test(hamt_insert(&root, &key[2]) == &key[2]);
		test(hamt_insert(&root, &key[3]) == &key[2]);
		test(hamt_search(&root, key[0]) == &key[0]);
		test(hamt_search(&root, key[1]) == &key[1]);
		test(hamt_search(&root, key[2]) == &key[2]);
		test(hamt_search(&root, key[3]) == &key[2]);
		test(hamt_search(&root, 0xDEAD) == NULL);
		test(hamt_first(&root, &s) == &key[0]);
		test(hamt_next(&root, &s) == &key[1]);
		test(hamt_next(&root, &s) == &key[2]);
		test(hamt_next(&root, &s) == NULL);
		test(hamt_delete(&root, key[0]) == &key[0]);
		test(hamt_delete(&root, key[1]) == &key[1]);
		test(hamt_delete(&root, key[2]) == &key[2]);
		test(hamt_delete(&root, key[0]) == NULL);
		test(hamt_first(&root, &s) == NULL);
	}
	stop_test();

	start_test("hamt: low bits");
	{
		root = HAMT_ROOT(custom_malloc, custom_free, direct_hash);
		u64 v1 = 0x1ULL;
		u64 v2 = 0x2ULL;
		u64 *key1 = &v1;
		u64 *key2 = &v2;
		test(hamt_insert(&root, key1) == key1);
		test(hamt_insert(&root, key2) == key2);
		test(hamt_search(&root, *key1) == key1);
		test(hamt_search(&root, *key2) == key2);
		test(hamt_search(&root, 0xDEAD) == NULL);
		test(hamt_search(&root, 0x100) == NULL);
		test(hamt_search(&root, 0x1001) == NULL);
		test(hamt_first(&root, &s) == key1);
		test(hamt_next(&root, &s) == key2);
		test(hamt_next(&root, &s) == NULL);
		test(hamt_delete(&root, *key1) == key1);
		test(hamt_delete(&root, *key2) == key2);
		test(hamt_delete(&root, *key1) == NULL);
		test(hamt_delete(&root, *key2) == NULL);
		test(hamt_first(&root, &s) == NULL);
	}
	stop_test();


	start_test("hamt: high bits");
	{
		root = HAMT_ROOT(custom_malloc, custom_free, direct_hash);
		u64 v1 = 0x8000000000000000ULL;
		u64 v2 = 0x9000000000000000ULL;
		u64 *key1 = &v1;
		u64 *key2 = &v2;
		test(hamt_insert(&root, key1) == key1);
		test(hamt_insert(&root, key2) == key2);
		test(hamt_search(&root, *key1) == key1);
		test(hamt_search(&root, *key2) == key2);
		test(hamt_search(&root, 0xDEAD) == NULL);
		test(hamt_search(&root, 0x100) == NULL);
		test(hamt_search(&root, 0x1001) == NULL);
		test(hamt_first(&root, &s) == key1);
		test(hamt_next(&root, &s) == key2);
		test(hamt_next(&root, &s) == NULL);
		test(hamt_delete(&root, *key1) == key1);
		test(hamt_delete(&root, *key2) == key2);
		test(hamt_delete(&root, *key1) == NULL);
		test(hamt_delete(&root, *key2) == NULL);
		test(hamt_first(&root, &s) == NULL);
	}
	stop_test();

	start_test("hamt: mixed high bits");
	{
		root = HAMT_ROOT(custom_malloc, custom_free, direct_hash);
		u64 key[] = {0xFFFFFFFFFFFFFFFFULL,
			     0xFFFFFFFFFFFFFFFEULL,
			     0,
			     1,
			     0xFFFFFFFF00000000ULL};
		test(hamt_insert(&root, &key[0]) == &key[0]);
		test(hamt_insert(&root, &key[1]) == &key[1]);
		test(hamt_insert(&root, &key[2]) == &key[2]);
		test(hamt_insert(&root, &key[3]) == &key[3]);
		test(hamt_insert(&root, &key[4]) == &key[4]);
		test(hamt_search(&root, key[0]) == &key[0]);
		test(hamt_search(&root, key[1]) == &key[1]);
		test(hamt_delete(&root, key[0]) == &key[0]);
		test(hamt_search(&root, key[0]) == NULL);
		test(hamt_insert(&root, &key[0]) == &key[0]);
		test(hamt_search(&root, key[0]) == &key[0]);
		test(hamt_search(&root, key[1]) == &key[1]);

		test(hamt_first(&root, &s) == &key[2]);
		test(hamt_next(&root, &s) == &key[3]);
		test(hamt_next(&root, &s) == &key[4]);
		test(hamt_next(&root, &s) == &key[1]);
		test(hamt_next(&root, &s) == &key[0]);
		test(hamt_next(&root, &s) == NULL);

		test(hamt_delete(&root, key[4]) == &key[4]);
		test(hamt_delete(&root, key[0]) == &key[0]);
		test(hamt_delete(&root, key[1]) == &key[1]);
		test(hamt_delete(&root, key[2]) == &key[2]);
		test(hamt_delete(&root, key[3]) == &key[3]);
		test(hamt_first(&root, &s) == NULL);
	}
	stop_test();


	return 0;
}
