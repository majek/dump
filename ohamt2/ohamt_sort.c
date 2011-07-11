#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#include "ohamt.h"


struct item {
	int counter;
        uint128_t hash;
	char *key;
};

struct item *items = NULL;
static int items_cnt = 0;
static int items_sz = 0;


static uint128_t hash(void *ud, uint64_t hitem)
{
	ud = ud;
        return items[hitem >> 1].hash;
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

uint64_t new_item(char *key) {
	if (items_cnt == items_sz) {
		items_sz *= 2;
		items = realloc(items, sizeof(struct item)*items_sz);
		assert(items);
	}
	struct item *item = &items[items_cnt++];

        item->counter = 1;
        item->key = strdup(key);

        unsigned char digest[20];

	MD5_CTX md5;
	MD5_Init(&md5);
	MD5_Update(&md5, key, strlen(key));
	MD5_Final(digest, &md5);

        memcpy(&item->hash, &digest, sizeof(item->hash));
	return items_cnt-1;
}

void del_item(uint64_t item_no) {
	assert(item_no == (uint64_t)items_cnt-1);
	free(items[item_no].key);
	items_cnt--;
}

int main()
{
	struct ohamt_root root;
	INIT_OHAMT_ROOT(&root, hash, NULL);

	items_cnt = 1; 		/* 0 is invalid */
	items_sz = 1024;
	items = malloc(sizeof(struct item)*items_sz);

	while (1) {
		char buf[1024];
		char *r = fgets(buf, sizeof(buf), stdin);
		if (r == NULL) {
			break;
		}
		strip(buf);

		uint64_t item_no = new_item(buf);
		uint64_t hinserted = ohamt_insert(&root, item_no << 1);
		if ((hinserted >> 1) != item_no) {
			items[hinserted >> 1].counter++;
			del_item(item_no);
		}
	}


	/* int i, sum=0; */
	/* for (i=0; i < sizeof(mem.histogram)/sizeof(mem.histogram[0]); i++) { */
	/* 	int used = (int)sizeof(void*)*i* (mem.histogram[i]); */
	/* 	fprintf(stderr, "[%i] -> %i (%i bytes)\n", */
	/* 	       i, */
	/* 	       mem.histogram[i], */
	/* 	       used); */
	/* 	sum += used; */
	/* } */
	/* fprintf(stderr, "%i/%i = %.3f bytes/item\n", sum, items, sum/(float)items); */

	printf("sleep\n");
	sleep(1);
	while (1) {
		struct ohamt_state state;
		uint64_t hitem_no = ohamt_first(&root, &state);
		if (hitem_no == 0) {
			break;
		}
		struct item *i = &items[hitem_no >> 1];
		printf("\t%i %s\n", i->counter, i->key);

		ohamt_delete(&root, hash(NULL, hitem_no));
	}

	while (items_cnt > 1) {
		del_item(items_cnt - 1);
	}
	FREE_OHAMT_ROOT(&root);

	free(items);

	return 0;
}
