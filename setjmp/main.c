/*

  gcc -Wall -g main.c tasklib.c -D_FORTIFY_SOURCE=0 -O3 && gdb ./a.out -ex r

 */
#include <stdio.h>
#include <setjmp.h>
#include "tasklib.h"


int max_task = 0;
struct context *mc;

void task_entry() {
	/* Every task, when it starts - first return to scheduler */
	if (mc->curr_task_no != 0) {
		printf("#%02i yielding to 0\n", mc->curr_task_no);
		yield(mc, 0, NULL);
	}
	int i;
	for(i=0; i < 2; i++){
		int tn = mc->curr_task_no + 1;
		if (tn > max_task) tn = 0;
		printf("#%02i yielding to %i\n", mc->curr_task_no, tn);
		yield(mc, tn, NULL);
	}
}


int main() {
	mc = new_context();
	/* We're in scheduler -> task 0  */

	int i;
	for (i=1; i < 16; i++) {
		printf("#%02i spawning #%i\n", mc->curr_task_no, mc->last_task_no);
		max_task = spawn(mc, task_entry);
	}
       
	task_entry();
	
	/* Allow task to die */
	for (i=1; i < 16; i++) {
		yield(mc, i, NULL);
	}

	/* We're the last one left */
	return 0;
}


