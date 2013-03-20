#include <stdio.h>
#include <stdlib.h>

__attribute__((noinline)) static void fun() {
	/* Use alloca, as (asm volatile "m") doesn't like pointers to stack */
	char *buf;
	buf = alloca(0x100);
	/* Make sure the compiler doens't optimize away the buf */
	asm volatile("" :: "m" (buf));

}

int main() {
	printf("[+] start\n");
	fun();
	printf("[+] end\n");
	return 0;
}
