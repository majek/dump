#include <stdio.h>
#include <stdlib.h>

#include "trace.h"
#include "syscalls.h"


struct proc {
	int pid;
};

static int process_continue(struct trace_process *process, int type, void *arg, void *userdata) {
	struct proc *proc = userdata;

	switch (type) {

	case TRACE_SYSCALL_ENTER: {
		struct trace_sysarg *sysarg = arg;
		char *name = "?";
		if (sysarg->number >= 0 && (unsigned)sysarg->number < sizeof(syscall_table) / sizeof(char*)) {
			if (syscall_table[sysarg->number]) 
				name = syscall_table[sysarg->number];
		}
		fprintf(stderr, "[.] #%i syscall %-22s\t... ", proc->pid, name);
		break; }
		
	case TRACE_SYSCALL_EXIT: {
		struct trace_sysarg *sysarg = arg;
		fprintf(stderr, " ret=%li\n", sysarg->ret);
		break; }

	case TRACE_SIGNAL:{
		int *signal_ptr = (int*)arg;
		fprintf(stderr, "[.] #%i got signal %i\n", proc->pid, *signal_ptr);
		break; }

	case TRACE_EXIT: {
		struct trace_exitarg *exitarg = arg;
		if (exitarg->type == TRACE_EXIT_NORMAL) {
			fprintf(stderr, "[.] #%i exit with status %i\n", proc->pid, exitarg->value);
		} else {
			fprintf(stderr, "[.] #%i exit with signal %i\n", proc->pid, exitarg->value);
		}
		free(proc);
		break; }

	}

	return 0;
}

static int process_new(struct trace_process *process, int type, void *arg, void *userdata) {
	int pid = (long)arg;
	
	struct proc *proc = calloc(1, sizeof(struct proc));
	proc->pid = pid;
	fprintf(stderr, "[.] #%i start\n", proc->pid);

	return trace_continue(process, process_continue, proc);
}


static int argv_join(char *dst, int dst_sz, char **argv, const char *delimiter) {
	if (argv[0]) {
		int r = snprintf(dst, dst_sz, "%s%s", delimiter, argv[0]);
		if (r < 0 || r >= dst_sz) {
			return dst_sz - 1;
		} else {
			return r + argv_join(dst + r, dst_sz - r, &argv[1], delimiter);
		}
	} else {
		if (dst_sz > 0)
			dst[0] = '\0';
		return 0;
	}
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s command [arguments ...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	struct trace *trace = trace_new(process_new, NULL);

	int pid = trace_execvp(trace, &argv[1]);
	char buf[1024];
	argv_join(buf, sizeof(buf), &argv[1], " ");
	fprintf(stderr, "[+] pid=%i, running: %s\n", pid, buf);

	while (trace_process_count(trace) > 0) {
		trace_read(trace);
	}

	trace_free(trace);

	return 0;
}
