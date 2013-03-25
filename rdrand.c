#include <stdio.h>


/* http://lkml.indiana.edu/hypermail/linux/kernel/1110.3/01116.html */

#ifdef __x86_64__
# define RDRAND_LONG ".byte 0x48,0x0f,0xc7,0xf0"
#else
# define RDRAND_INT  ".byte 0x0f,0xc7,0xf0"
# define RDRAND_LONG	RDRAND_INT
#endif

#ifdef __x86_64__
#  define PRIxLONGSZ "16"
#else
#  define PRIxLONGSZ "8"
#endif

#define RDRAND_RETRY_LOOPS	10

#define RESEED_LOOP ((512*128)/sizeof(unsigned long))


static inline long rdrand_long(unsigned long *v) {
	int ok;
	asm volatile("1: " RDRAND_LONG "\n\t"
		     "jc 2f\n\t"
		     "decl %0\n\t"
		     "jnz 1b\n\t"
		     "2:"
		     : "=r" (ok), "=a" (*v)
		     : "0" (RDRAND_RETRY_LOOPS));
	return ok;
}


static inline int rdrand_seed() {
	unsigned long tmp;
	unsigned count, i;
	for (count = i = 0; i < RESEED_LOOP; i++) {
		int ok = rdrand_long(&tmp);
		if (ok)
			count++;
	}
	return count == RESEED_LOOP;
}



int main() {
	printf("[.] seed initialized = %s\n", rdrand_seed() ? "OK" : "FAIL");
	int i;
	for (i = 0; i < 16; i++) {
		unsigned long tmp;
		rdrand_long(&tmp);
		printf("[ ] 0x%0" PRIxLONGSZ "lx\n", tmp);
	}
	return 0;
}
