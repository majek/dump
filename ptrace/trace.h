struct parameters {
	unsigned long syscall;
	unsigned long args[6];
};

#define HPIDS_SIZE 51

struct trace {
	int sfd;
	int process_count;
	struct hlist_head hpids[HPIDS_SIZE];
	void *tracedata;
};


struct trace_process {
	struct hlist_node node;
	pid_t pid;
	int initialized;
	int within_syscall;
	struct trace *trace;
	void *procdata;
};

int trace_init(struct trace *trace);
int trace_execvp(struct trace *trace, char **argv);
void trace_read(struct trace *trace);


void *process_start(int pid, void *tracedata);
#define TYPE_EXIT 0
#define TYPE_SIGNAL 1
void process_stop(void *procdata, int type, int status);
void process_syscall(void *procdata, int syscall_exit, struct parameters *params);
