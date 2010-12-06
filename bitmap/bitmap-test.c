#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bitmap.h"


int main() {
	struct bitmap *bm = bitmap_new(1310);
	assert(bitmap_get(bm, 0) == 0);
	bitmap_set(bm, 0);
	assert(bitmap_get(bm, 0) == 1);
	bitmap_clear(bm, 0);
	assert(bitmap_get(bm, 0) == 0);

	assert(bitmap_get(bm, 63) == 0);
	bitmap_set(bm, 63);
	assert(bitmap_get(bm, 63) == 1);
	bitmap_clear(bm, 63);
	assert(bitmap_get(bm, 63) == 0);

	assert(bitmap_get(bm, 64) == 0);
	bitmap_set(bm, 64);
	assert(bitmap_get(bm, 64) == 1);
	bitmap_clear(bm, 64);
	assert(bitmap_get(bm, 64) == 0);

	bitmap_set(bm, 0);
	bitmap_set(bm, 1);
	bitmap_set(bm, 32);
	bitmap_set(bm, 33);
	bitmap_set(bm, 63);
	bitmap_set(bm, 64);
	bitmap_set(bm, 127);
	bitmap_set(bm, 128);
	bitmap_set(bm, 129);
	bitmap_set(bm, 1024);

	assert(bitmap_next(bm, -1) == 0);
	assert(bitmap_next(bm, 0) == 1);
	assert(bitmap_next(bm, 1) == 32);
	assert(bitmap_next(bm, 32) == 33);
	assert(bitmap_next(bm, 33) == 63);
	assert(bitmap_next(bm, 63) == 64);
	assert(bitmap_next(bm, 64) == 127);
	assert(bitmap_next(bm, 127) == 128);
	assert(bitmap_next(bm, 128) == 129);
	assert(bitmap_next(bm, 129) == 1024);
	assert(bitmap_next(bm, 1024) == -1);

	bitmap_free(bm);
	return 0;
}
