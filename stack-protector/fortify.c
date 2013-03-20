#include <stdio.h>
#include <string.h>

/* __attribute__((noinline)) char *my_strcpy(char *dst, const char *src) { */
/* 	strcpy(dst, src); */
/* } */

__attribute__((noinline)) static void fun(char *s) {
	char buf[0x100];
	strcpy(buf, s);
	/* Make sure the compiler doens't optimize away the buf */
	asm volatile("" :: "m" (buf[0]));
}

int main(int argc, char **argv) {
	printf("[+] start\n");
	fun(argv[0]);
	printf("[+] end\n");
	return 0;
}
