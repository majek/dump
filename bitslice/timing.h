#include <sys/time.h>

static inline uint64_t gettime_ns() {
        struct timeval tv;
	gettimeofday(&tv, NULL);
        return (uint64_t)tv.tv_sec * 1000000000ULL + tv.tv_usec * 1000ULL;
}

#ifdef __i386__
#  define RDTSC_DIRTY "%eax", "%ebx", "%ecx", "%edx"
#elif __x86_64__
#  define RDTSC_DIRTY "%rax", "%rbx", "%rcx", "%rdx"
#else
# error unknown platform
#endif

#define RDTSC_START(cycles)						\
	do {								\
		register unsigned cycles_high, cycles_low;		\
		asm volatile("CPUID\n\t"				\
			     "RDTSC\n\t"				\
			     "mov %%edx, %0\n\t"			\
			     "mov %%eax, %1\n\t"			\
			     : "=r" (cycles_high), "=r" (cycles_low)	\
			     :: RDTSC_DIRTY);				\
		(cycles) = ((uint64_t)cycles_high << 32) | cycles_low;	\
	} while (0)

#define RDTSC_STOP(cycles)						\
	do {								\
		register unsigned cycles_high, cycles_low;		\
		asm volatile("RDTSCP\n\t"				\
			     "mov %%edx, %0\n\t"			\
			     "mov %%eax, %1\n\t"			\
			     "CPUID\n\t"				\
			     : "=r" (cycles_high), "=r" (cycles_low)	\
			     :: RDTSC_DIRTY);				\
		(cycles) = ((uint64_t)cycles_high << 32) | cycles_low;	\
	} while(0)

#define MIN(a,b)				\
	(a) < (b) ? (a) : (b)
