#include <stdio.h>
#include <stdlib.h>
	char buf;

__attribute__((noinline)) void fun2(int b) {
        asm volatile("" :: "m" (buf));
}

__attribute__((noinline)) void fun(int a) {
        asm volatile("" :: "m" (buf));
	fun2(2);
        asm volatile("" :: "m" (buf));
}


int main() {
	printf("[+] start\n");
	fun(1);
	printf("[+] end\n");
	return 0;
}
