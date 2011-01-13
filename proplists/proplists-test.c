
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "proplists.h"


int main() {

        struct proplists_root root;
        INIT_PROPLISTS_ROOT(&root);

        assert(proplists_find(&root, "a", NULL) == NULL);
        assert(proplists_find(&root, "b", NULL) == NULL);

        assert(proplists_add(&root, "c", "value_c", 0) == 1);
        assert(proplists_add(&root, "a", "value_a", 0) == 1);
        assert(proplists_add(&root, "b", "value_b", 0) == 1);
        assert(proplists_find(&root, "a", NULL) != NULL);
        assert(proplists_find(&root, "b", NULL) != NULL);
        assert(proplists_find(&root, "c", NULL) != NULL);
        assert(strcmp(proplists_find(&root, "b", NULL), "value_b") == 0);
        assert(strcmp(proplists_find(&root, "a", NULL), "value_a") == 0);
        assert(strcmp(proplists_find(&root, "c", NULL), "value_c") == 0);
        assert(proplists_find(&root, "wrong", NULL) == NULL);

        assert(proplists_add(&root, "a", "value_a2", 0) == 0);
        assert(strcmp(proplists_find(&root, "a", NULL), "value_a") == 0);

        struct list_head *pos;
        char *key, *value;
        int value_sz;
        proplists_for_each(pos, &root, key, value, value_sz) {
                printf("%s -> %s [%i]\n", key, value, value_sz);
        }


        assert(proplists_find(&root, "c", NULL) != NULL);
        assert(proplists_del(&root, "c") == 1);
        assert(proplists_find(&root, "c", NULL) == NULL);

        assert(proplists_find(&root, "a", NULL) != NULL);
        assert(proplists_del(&root, "a") == 1);
        assert(proplists_find(&root, "a", NULL) == NULL);

        assert(proplists_find(&root, "b", NULL) != NULL);
        assert(proplists_del(&root, "b") == 1);
        assert(proplists_find(&root, "b", NULL) == NULL);


        return 0;
}
