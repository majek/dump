#include <stdio.h>
#include <string.h>

#include "ebtree/ebsttree.h"


struct item {
	int counter;
	struct ebmb_node node;
	char key[];
};

int main()
{
	printf("sizeof(ebmb_node)=%i\n", (int)sizeof(struct ebmb_node));
	struct eb_root root = EBMB_ROOT;
	while (1) {
		char buf[256];
		char *r = fgets(buf, sizeof(buf), stdin);
		if (r == NULL) {
			break;
		}
		int len = strlen(r) + 1;
		while((len-2 > 0) && (buf[len-2] == '\n')) {
			buf[len-2] = '\0';
			len--;
		}
		struct ebmb_node *node = ebst_lookup_len(&root, buf, len-1);
		if (node) {
			struct item *item = ebmb_entry(node, struct item, node);
			item->counter ++;
		} else {
			struct item *item =				\
				(struct item *)malloc(sizeof(struct item) + len);
			item->counter = 1;
			memcpy(item->key, buf, len);
			ebmb_insert(&root, &item->node, len);
		}
	}

	struct ebmb_node *node = ebmb_first(&root);
	while (node) {
		struct item *item = ebmb_entry(node, struct item, node);
		printf("%8i %s\n", item->counter, item->key);
		node = ebmb_next(node);
	}

	node = ebmb_first(&root);
	while (node) {
		struct ebmb_node *next_node = ebmb_next(node);
		ebmb_delete(node);
		struct item *item = ebmb_entry(node, struct item, node);
		free(item);
		node = next_node;
	}


	return 0;
}
