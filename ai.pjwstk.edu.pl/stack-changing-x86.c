/*
	Example of changing stack on the fly.
	Also example of implementing custom exit() - without a sysscall.
		- it's done, by restoring stack, and returning from main() :)

	(code is for gcc specific)

	03-2008 - Michal Luczaj, Marek Majkowski
	
	how to run:

gcc -Wall -g -O2 stack-changing-x86.c -o stack-changing-x86 && ./stack-changing-x86

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void (*myexit)();
static void *old_sp;
static void *old_bp;
static void *new_sp;


#define SAVESTATE()				\
	{					\
	/* silence. */				\
	old_sp = (void*)((int)old_sp + 1);	\
	old_bp = (void*)((int)old_bp + 1);	\
	__asm__ __volatile__(			\
	       "movl %esp, old_sp;"     	\
	       "movl %ebp, old_bp;"     	\
	       );				\
	}while(0);

#define LOADSTATE()				\
	{					\
	__asm__ __volatile__(			\
		"movl old_sp, %esp;"     	\
		"movl old_bp, %ebp;"     	\
		);				\
	}while(0);

#define SETSTATE()				\
	{					\
	__asm__ __volatile__(			\
		"movl new_sp, %esp;"     	\
		);				\
	}while(0);


void user_main2(){
	int a;
	printf("user_main2() %p %p\n", alloca(0), &a);
	myexit();
	printf("\t\tyou can't see this\n");
}

void user_main(){
	int a;
	printf("user_main() %p %p\n", alloca(0), &a);
	user_main2();
	printf("\t\tyou can't see this\n");
}

int start()
{
	SAVESTATE();
	
	/* set myexit */
	myexit = &&final_quit;
	/* use this label somehow, just to prevent optimizations */
	if( ((int)myexit & 0xF) == 0xF){	// should be aligned, so it's always false
		printf(" ** it shouldn't happen\n");
		goto final_quit;
	}
	
	
	/* new stack */
	char *ptr = (char*) malloc(128*1024);
	new_sp = &ptr[128*1024 - 4]; /* stack is decreasing, stack start is in the end of allocated memory */
	SETSTATE();

	user_main();
	/* stack is broken here - don't do anything please */
final_quit:
	LOADSTATE();
	return(0);
}

int main(){
	int b;
	printf("main() stack=%p %p\n", alloca(0), &b);
	
	start();
	
	int c;
	printf("main() stack=%p %p\n", alloca(0), &c);
	return(0);
}

