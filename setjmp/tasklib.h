

typedef void(*callback)();

struct context {
	jmp_buf tasks[1024];

	int last_task_no;
	int curr_task_no;
};

struct context *new_context();
void yield(struct context *context, int task_no, void *ud);
int spawn(struct context *context, callback fun);
