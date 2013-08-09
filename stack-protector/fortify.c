#include <stdio.h>
#include <string.h>

void fun(char *s, int l) {
	char buf[0x100];
	strncpy(buf, s, l);
	asm volatile("" :: "m" (buf[0]));
}

int main(int argc, char **argv) {
	printf("[+] start\n");
	fun(argv[0], 0x120);
	printf("[+] end\n");
	return 0;
}
