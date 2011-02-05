
#include <stdio.h>

#include <stdint.h>

#include "hamt.h"
#include "hamt.c"

#define start_test(name)			\
	do {					\
	int ok=0, fail=0;			\
	printf("\t%-18s:", name)		\

#define test(cmd)						\
	do {							\
		int _res = (cmd);				\
		if(_res) {					\
			ok++;					\
		} else {					\
			if(!fail) printf("\n");			\
			printf("\t\tFAILED: %s  (eq 0x%x)\t  (%s:%i)\n",	\
			       #cmd, _res,  __FILE__, __LINE__);	\
			fail++;					\
		}						\
	} while(0)

#define test_eq(cmd1, cmd2)						\
	do {								\
		uint128_t _res1 = (cmd1);				\
		uint128_t _res2 = (cmd2);				\
		if(_res1 == _res2) {					\
			ok++;						\
		} else {						\
			if(!fail) printf("\n");			\
			printf("\t\tFAILED: %s (eq 0x%lx) != %s (eq 0x%lx)\t  (%s:%i)\n",	\
			       #cmd1,(uint64_t)_res1, #cmd2, (uint64_t)_res2,  __FILE__, __LINE__); \
			fail++;					\
		}						\
	} while(0)

#define stop_test()				\
	if(!fail) {				\
		printf("%3i ok!\n", ok);	\
	};					\
	} while (0)


static void *custom_malloc(size_t sz)
{
	return malloc(sz);
}

static void custom_free(void *ptr, size_t sz)
{
	free(ptr);
}

struct item {
        uint128_t key;
};

static uint128_t *user_hash(void *ud, void *ptr)
{
        struct item *itm = (struct item*)ptr;
	return &itm->key;
}

uint128_t *kalloc(uint64_t *k_start)
{
	uint64_t *k = k_start;
	int i = 0 ;
	while (*k) { i++, k++;};
	uint128_t *a = (uint128_t*)malloc(sizeof(uint128_t) * i);
	k = k_start;
	i =0;
	while (*k) {
		a[i] = *k++;
		i++;
	}
	return a;
}



int main() {
	setvbuf(stdout, NULL, _IONBF, 0);


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

	/* start_test("slice_get"); */
	/* { */
	/* 	test(slice_get((uint128_t)0x7f, 0) == 0x3f); */
	/* 	test(slice_get((uint128_t)0x7f, 1) == 0x1); */
	/* 	test(slice_get((uint128_t)0x7f, 2) == 0); */
	/* 	test(slice_get((uint128_t)0x7f, 23) == 0); */
	/* 	test_eq(slice_get((uint128_t)0x7000000000000000ULL << 64, 21), 1); */
	/* 	test_eq(slice_get((uint128_t)0x7000000000000000ULL << 64, 20), 0x6); */
	/* 	test_eq(slice_get((uint128_t)0xFFFF000000000000ULL << 64, 21), 0x0f); */
	/* 	test_eq(slice_get((uint128_t)0xFFFF000000000000ULL << 64, 20), 0x3f); */
	/* 	test_eq(slice_get((uint128_t)0xFFFF000000000000ULL << 64, 19), 0x3f); */
	/* 	test_eq(slice_get((uint128_t)0xFFFF000000000000ULL << 64, 18), 0x0); */
	/* 	test_eq(slice_get((uint128_t)0xFFFF810000000000ULL << 64, 18), 0x20); */
	/* 	test_eq(slice_get((uint128_t)0xFFFF810000000000ULL << 64, 17), 0x10); */
	/* } */
	/* stop_test(); */

	/* start_test("slice_set"); */
	/* { */
	/* 	//test(slice_set(0, 1, 11) == 0x0); */
	/* 	test(slice_set(0, 1, 10) == 0x1); */
	/* 	test(slice_set(0, 1, 9) == 0x40); */
	/* 	test(slice_set(0, 0x3f, 0) == 0xF000000000000000ULL); */
	/* 	test(slice_set(0, 0x3f, 8) == 0x3f << (6*2)); */
	/* 	test(slice_set(0, 0x3f, 7) == 0x3f << (6*3)); */
	/* 	test(slice_set(0, 0x3f, 1) == 0x3fULL << (6*9)); */
	/* 	test(slice_set(0, 0x3f, 0) == 0x3fULL << (6*10)); */
	/* 	test(slice_set(0, 0x39, 0) == 0x9ULL << (6*10)); */
	/* 	test(slice_set(0, 0x3E, 0) == 0xE000000000000000ULL); */
	/* 	test(slice_set(1, 1, 10) == 0x1); */
	/* 	test(slice_set(1, 1, 9) == 0x41); */
	/* 	test(slice_set(0x8000000000000000ULL, 1, 9) == 0x8000000000000040ULL); */
	/* } */
	/* stop_test(); */



	struct hamt_root root;
	struct hamt_state s;

	start_test("hamt: few items");
	{
                root = HAMT_ROOT(custom_malloc, custom_free, user_hash, NULL);
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
		test(hamt_delete(&root, &k->key) == NULL);
		test(hamt_delete(&root, &i->key) == i);
		test(hamt_delete(&root, &i->key) == NULL);
		test(hamt_search(&root, &i->key) == NULL);
		test(hamt_delete(&root, &j->key) == j);
		test(hamt_delete(&root, &j->key) == NULL);
		test(hamt_search(&root, &j->key) == NULL);
		free(i);
		free(j);
		free(k);
	}
	stop_test();

	start_test("hamt: single item");
	{
                root = HAMT_ROOT(custom_malloc, custom_free, user_hash, NULL);
		uint64_t a[] = {-1,1,2,3,4,0};
		uint128_t *key = kalloc(&a[0]);
		test(hamt_search(&root, &key[1]) == NULL);
		test(hamt_insert(&root, &key[1]) == &key[1]);
		test(hamt_search(&root, &key[1]) == &key[1]);
		test(hamt_delete(&root, &key[2]) == NULL);
		test(hamt_delete(&root, &key[1]) == &key[1]);
		test(hamt_delete(&root, &key[1]) == NULL);
		test(hamt_search(&root, &key[1]) == NULL);
		test(hamt_first(&root, &s) == NULL);
		free(key);
	}
	stop_test();

	start_test("hamt: low bits");
	{
                root = HAMT_ROOT(custom_malloc, custom_free, user_hash, NULL);
		uint64_t a[] = {-1,1,2,0x100,0x1001,0};
		uint128_t *key = kalloc(&a[0]);
		uint128_t *key1 = &key[1];
		uint128_t *key2 = &key[2];
		test(hamt_insert(&root, key1) == key1);
		test(hamt_insert(&root, key2) == key2);
		test(hamt_search(&root, key1) == key1);
		test(hamt_search(&root, key2) == key2);
		test(hamt_search(&root, &key[3]) == NULL);
		test(hamt_search(&root, &key[4]) == NULL);
		test(hamt_first(&root, &s) == key1);
		/* test(hamt_next(&root, &s) == key2); */
		/* test(hamt_next(&root, &s) == NULL); */
		test(hamt_delete(&root, key1) == key1);
		test(hamt_delete(&root, key2) == key2);
		test(hamt_delete(&root, key1) == NULL);
		test(hamt_delete(&root, key2) == NULL);
		test(hamt_first(&root, &s) == NULL);
		free(key);
	}
	stop_test();

	start_test("hamt: high bits");
	{
                root = HAMT_ROOT(custom_malloc, custom_free, user_hash, NULL);
		uint64_t a[] = {-1,0x8000000000000000ULL,0x9000000000000000ULL,0x100,0x1001,0};
		uint128_t *key = kalloc(&a[0]);
		uint128_t *key1 = &key[1];
		uint128_t *key2 = &key[2];
		test(hamt_insert(&root, key1) == key1);
		test(hamt_insert(&root, key2) == key2);
		test(hamt_search(&root, key1) == key1);
		test(hamt_search(&root, key2) == key2);
		test(hamt_search(&root, &key[3]) == NULL);
		test(hamt_search(&root, &key[4]) == NULL);
		test(hamt_first(&root, &s) == key1);
		/* test(hamt_next(&root, &s) == key2); */
		/* test(hamt_next(&root, &s) == NULL); */
		test(hamt_delete(&root, key1) == key1);
		test(hamt_delete(&root, key2) == key2);
		test(hamt_delete(&root, key1) == NULL);
		test(hamt_delete(&root, key2) == NULL);
		test(hamt_first(&root, &s) == NULL);
		free(key);
	}
	stop_test();

        return 0;
}
