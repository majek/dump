#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "list.h"
#include "trace.h"


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
		exit(-1);
	}

	struct trace trace;
	memset(&trace, 0, sizeof(struct trace));

	trace_init(&trace);

	int pid = trace_execvp(&trace, &argv[1]);
	char buf[1024];
	argv_join(buf, sizeof(buf), &argv[1], " ");
	fprintf(stderr, "[+] pid=%i, running: %s\n", pid, buf);
	
	while (trace.process_count > 0) {
		trace_read(&trace);
	};

	exit(-1);
}


struct proc {
	int pid;
};

void *process_start(int pid, void *tracedata) {
	struct proc *proc = calloc(1, sizeof(struct proc));
	proc->pid = pid;
	fprintf(stderr, "#%i start\n", proc->pid);
	return proc;
}
void process_stop(void *procdata, int type, int status) {
	struct proc *proc = procdata;
	if (type == TYPE_EXIT) {
		fprintf(stderr, "#%i exit status=%i\n", proc->pid, status);
	} else if (type == TYPE_SIGNAL) {
		fprintf(stderr, "#%i exit signal=%i\n", proc->pid, status);
	}
	free(proc);
}
void process_syscall(void *procdata, int syscall_exit, struct parameters *params) {
	struct proc *proc = procdata;
	fprintf(stderr, "#%i syscall %s %li\n", proc->pid, syscall_exit ? "-" : "_", params->syscall);
}
