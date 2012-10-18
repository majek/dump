#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "tasklib.h"


#define DEBUG(_par...) do {						\
		fprintf(stderr, "%14s() %s:%u ",			\
			__FUNCTION__, __FILE__, __LINE__);		\
		fprintf(stderr, " " _par);				\
		fprintf(stderr, "\n");					\
	} while (0)


static void grow_stack(struct context *context, int task_no) {
	char _frame[4096];
	/* We need to allocate few K of stack for every task.
	   Prevent compiler from optiomizing this out. */
	memset(_frame, 0, 4096);

	if (task_no >= 1024) {	
		/* Return to task 0 */
		longjmp(context->tasks[0], 1);
	}

	int r = setjmp(context->tasks[task_no]);
	if (r != 0) {
		context->curr_task_no = task_no;
		if (r == 1) {
			/* for task 0 just return to parent */
			return;
		}

		fprintf(stderr, " [.] #%i starting\n", task_no);
		callback fun = (callback)r;
		fun();
		fprintf(stderr, " [.] #%i exited\n", task_no);

		/* Return to task 0 after someone exited. */
		longjmp(context->tasks[0], 1);
	} else {
		// DEBUG("task #%i, stack=%p\n", task_no, __builtin_frame_address(0));
		grow_stack(context, task_no + 1);
		/* GCC does tail recursion nowadays. Prevent that. */
		__asm__ __volatile__ ("" ::: "memory");
	}
}

struct context *new_context() {
	struct context *context;
	context = malloc(sizeof(struct context));
	memset(context, 0, sizeof(struct context));
	context->last_task_no = 1;
	context->curr_task_no = 0;

	grow_stack(context, 0);
	return context;
}


void yield(struct context *context, int task_no, void *ud) {
	// DEBUG("context=%p", context);
	int curr_task_no = context->curr_task_no;
	int r = setjmp(context->tasks[curr_task_no]);
	if (!r) {
		longjmp(context->tasks[task_no], (int) ud);
	}
	context->curr_task_no = curr_task_no;
	return;
}

int spawn(struct context *context, callback fun) {
	int task_no = context->last_task_no++;
	yield(context, task_no, fun);
	return task_no;
}
