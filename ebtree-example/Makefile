
EBTREE_OBJS=\
	ebtree/ebtree.c	\
	ebtree/eb32tree.c \
	ebtree/ebmbtree.c \
	ebtree/ebsttree.c \
	ebtree/ebimtree.c

sort-eb: *.c ebtree/* Makefile
	gcc -m32 -Wall -g -O2 sort-eb.c $(EBTREE_OBJS) -o sort-eb

sort-rb: *.c rbtree/* Makefile
	gcc -m32 -Wall -g -O2 sort-rb.c rbtree/rbtree.c -o sort-rb

