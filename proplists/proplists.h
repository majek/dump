#ifndef _PROPLISTS_H
#define _PROPLISTS_H

/* You need:
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
 */

struct proplists_item {
        struct list_head in_list;
        uint64_t key_hash;
        char *value;
        char key[];
};

struct proplists_root {
        struct list_head list_of_items;
};


//#define PROPLISTS_ROOT_INIT(name) (struct proplists_root) { {&(name).root,  &(name).root}}
static inline void INIT_PROPLISTS_ROOT(struct proplists_root *root) {
        INIT_LIST_HEAD(&root->list_of_items);
}

static inline uint64_t  __proplists_hash(char *key, int *key_sz) {
        /* djb hash with 64bit output,  http://www.cse.yorku.ca/~oz/hash.html */
        uint64_t hash = 5381;
        char *s = key;

        while(*s) {
                hash = ((hash << 5) + hash) + *s;
                s++;
        }
        if (key_sz) {
                *key_sz = s - key;
        }
        return hash;
}


static inline char *proplists_find(struct proplists_root *root, char *key) {
        uint64_t key_hash = __proplists_hash(key, NULL);
        struct list_head *head;
        list_for_each(head, &root->list_of_items) {
                struct proplists_item *item = \
                        container_of(head, struct proplists_item, in_list);
                if (item->key_hash == key_hash && strcmp(key, item->key) == 0) {
                        return item->value;
                }
                if (item->key_hash > key_hash) {
                        break;
                }
        }
        return NULL;
}

static inline int proplists_add(struct proplists_root *root,
                                char *key, char *value) {
        int key_sz;
        uint64_t key_hash = __proplists_hash(key, &key_sz);
        struct list_head *head;
        list_for_each(head, &root->list_of_items) {
                struct proplists_item *item = \
                        container_of(head, struct proplists_item, in_list);
                if (item->key_hash == key_hash && strcmp(key, item->key) == 0) {
                        return 0;
                }
                if (item->key_hash > key_hash) {
                        goto insert;
                }
        }
        head = &root->list_of_items;

insert:;
        int value_sz = strlen(value);
        struct proplists_item *new_item = (struct proplists_item*)      \
                malloc(sizeof(struct proplists_item)
                       + key_sz + 1 + value_sz + 1);
        INIT_LIST_HEAD(&new_item->in_list);

        new_item->key_hash = key_hash;
        memcpy(new_item->key, key, key_sz + 1);

        new_item->value = new_item->key + key_sz + 1;
        memcpy(new_item->value, value, value_sz + 1);

        list_add_tail(&new_item->in_list, head);
        return 1;
}

static inline int proplists_del(struct proplists_root *root, char *key) {
        int key_sz;
        uint64_t key_hash = __proplists_hash(key, &key_sz);
        struct list_head *head;
        list_for_each(head, &root->list_of_items) {
                struct proplists_item *item = \
                        container_of(head, struct proplists_item, in_list);
                if (item->key_hash == key_hash && strcmp(key, item->key) == 0) {
                        list_del(&item->in_list);
                        free(item);
                        return 1;
                }
                if (item->key_hash > key_hash) {
                        break;
                }
        }
        return 0;
}




/* based on list_for_each */
#define proplists_for_each(pos, root, key, value)                       \
        for (pos = (root)->list_of_items.next; prefetch(pos->next),     \
                     (key) = container_of(pos, struct proplists_item, in_list)->key, \
                     (value) = container_of(pos, struct proplists_item, in_list)->value, \
                     pos != &(root)->list_of_items;                    \
             pos = pos->next)



#endif // _PROPLISTS_H
