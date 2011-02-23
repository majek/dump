/*
gcc memalloc.c memalloc_test.c -DVALGRIND -I.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memalloc.h"

int main()
{
	struct mem_context *ctx = mem_context_new();
	int i, j;
	for (i=1; i< 400; i++) {
		void *ptrs[1025];
		for (j=0; j < 1025; j++) {
			ptrs[j] = mem_alloc(ctx, i);
			memset(ptrs[j], 0xFF, i);
		}
		for (j=0; j < 1025; j++) {
			mem_free(ctx, ptrs[j], i);
		}
	}
	mem_context_free(ctx);
}
