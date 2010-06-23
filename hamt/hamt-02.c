#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hamt.h"

struct item {
	struct hamt_item hamt;
	u32 size;
	u64 file_offset;
};

int main()
{
	printf("sizeof()=%i\n", sizeof(struct item));
}
