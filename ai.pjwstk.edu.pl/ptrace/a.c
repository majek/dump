#include <stdio.h>
int main(){
	unsigned long db_regs;
	asm volatile ("mov %%dr0, %0" : "=r"(db_regs));
	printf("%08lx\n", db_regs);
	return(0);
}

