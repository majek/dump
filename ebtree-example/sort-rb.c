#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rbtree/rbtree.h"


struct item {
	int counter;
	struct rb_node node;
	char key[];
};

struct item *tree_lookup(struct rb_root *root, char *key) {
	int result = 0;
	struct rb_node *node = root->rb_node;

	while (node) {
		struct item *item = rb_entry(node, struct item, node);
		result = strcmp(key, item->key);

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return item;
	}
	return NULL;
}

static int tree_insert(struct rb_root *root, struct item *data) {
        struct rb_node **new = &root->rb_node, *parent = NULL;

        /* Figure out where to put new node */
        while (*new) {
                struct item *item = rb_entry(*new, struct item, node);
                int result = strcmp(data->key, item->key);

                parent = *new;
                if (result < 0)
                        new = &((*new)->rb_left);
                else if (result > 0)
                        new = &((*new)->rb_right);
                else
                        return 0;
        }

        /* Add new node and rebalance tree. */
        rb_link_node(&data->node, parent, new);
        rb_insert_color(&data->node, root);

        return 1;
}



int main()
{
	printf("sizeof(rb_node)=%i\n", (int)sizeof(struct rb_node));
	struct rb_root root = RB_ROOT;
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
		struct item *item = tree_lookup(&root, buf);
		if (item) {
			item->counter ++;
		} else {
			struct item *item =				\
				(struct item *)malloc(sizeof(struct item) + len);
			item->counter = 1;
			memcpy(item->key, buf, len);
			tree_insert(&root, item);
		}
	}

	struct rb_node *node = rb_first(&root);
	while (node) {
		struct item *item = rb_entry(node, struct item, node);
		//printf("%8i %s\n", item->counter, item->key);
		printf("%s\n", item->key);
		node = rb_next(node);
	}

	node = rb_first(&root);
	while (node) {
		struct rb_node *next_node = rb_next(node);
		rb_erase(node, &root);
		struct item *item = rb_entry(node, struct item, node);
		free(item);
		node = next_node;
	}

	return 0;
}

