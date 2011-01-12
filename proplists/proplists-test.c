
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

        assert(proplists_find(&root, "a") == NULL);
        assert(proplists_find(&root, "b") == NULL);

        assert(proplists_add(&root, "c", "value_c") == 1);
        assert(proplists_add(&root, "a", "value_a") == 1);
        assert(proplists_add(&root, "b", "value_b") == 1);
        assert(proplists_find(&root, "a") != NULL);
        assert(proplists_find(&root, "b") != NULL);
        assert(proplists_find(&root, "c") != NULL);
        assert(strcmp(proplists_find(&root, "b"), "value_b") == 0);
        assert(strcmp(proplists_find(&root, "a"), "value_a") == 0);
        assert(strcmp(proplists_find(&root, "c"), "value_c") == 0);
        assert(proplists_find(&root, "wrong") == NULL);

        assert(proplists_add(&root, "a", "value_a2") == 0);
        assert(strcmp(proplists_find(&root, "a"), "value_a") == 0);

        struct list_head *pos;
        char *key, *value;
        proplists_for_each(pos, &root, key, value) {
                printf("%s -> %s\n", key, value);
        }


        assert(proplists_find(&root, "c") != NULL);
        assert(proplists_del(&root, "c") == 1);
        assert(proplists_find(&root, "c") == NULL);

        assert(proplists_find(&root, "a") != NULL);
        assert(proplists_del(&root, "a") == 1);
        assert(proplists_find(&root, "a") == NULL);

        assert(proplists_find(&root, "b") != NULL);
        assert(proplists_del(&root, "b") == 1);
        assert(proplists_find(&root, "b") == NULL);


        return 0;
}
