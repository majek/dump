#include <stdio.h>


#ifdef __x86_64__
#  define RDRAND_LONG ".byte 0x48,0x0f,0xc7,0xf0"
#else
#  define RDRAND_INT  ".byte 0x0f,0xc7,0xf0"
#  define RDRAND_LONG	RDRAND_INT
#endif

#define RDRAND_RETRY_LOOPS	10

long rdrand_long(unsigned long *v) {
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


int main() {
	int i;
	for (i=0; 1; i++) {
		unsigned long v = 0;
		int ok = rdrand_long(&v);
		if (ok != 10)
			printf("v=%lx ok=%i\n", v, ok);
	}
	return 0;
}
