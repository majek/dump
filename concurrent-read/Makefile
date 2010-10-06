ifeq (debian, $(shell hostname))
ITEMS=100
else
ITEMS=3000
endif

ARGS=\
	"normal		none	none		sort	pread"	\
	"normal		sort	fadvise		none	pread"	\
	"normal		sort	readahead	none	pread"


B=\
	"normal		sort	fadvise	rand	gthread512"	\
	"normal		sort	fadvise	sort	gthread512"
	

all: read_bench random.bin
	@for ARGS in $(ARGS); do					\
		sync;							\
		sudo bash -c "echo 3 > /proc/sys/vm/drop_caches";	\
		sync;							\
		sudo dd if=/dev/sda of=/dev/null bs=1M count=1 2> /dev/null;	\
		./read_bench $(ITEMS) $$ARGS;				\
	done


random.bin:
	dd if=/dev/urandom of=random.bin bs=1M count=16



read_bench: read_bench.c
	gcc -g -O2 -Wall read_bench.c -o read_bench -lm -lrt -I/usr/lib64/glib-2.0/include -I/usr/lib/glib-2.0/include -I/usr/include/glib-2.0 -lgthread-2.0
 