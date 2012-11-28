#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/signalfd.h>

#include <sys/reg.h>

#include "list.h"
#include "trace.h"
#include "syscall.h"

/*
  References:
      http://strace.git.sourceforge.net/git/gitweb.cgi?p=strace/strace;a=blob;f=README-linux-ptrace;hb=HEAD
      http://hssl.cs.jhu.edu/~neal/woodchuck/src/commits/c8a6c6c6d28c87e2ef99e21cf76c4ea90a7e11ad/src/process-monitor-ptrace.c.raw.html
*/

#define PFATAL(msg ...)						\
	do { fprintf(stderr, msg); perror(""); exit(EXIT_FAILURE); } while (0)


int trace_init(struct trace *trace) {
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		PFATAL("sigprocmask(SIG_BLOCK, [SIGCHLD])");

	trace->sfd = signalfd(-1, &mask, 0);
	if (trace->sfd == -1)
		PFATAL("signalfd()");
	int i;
	for (i=0; i < HPIDS_SIZE; i++) {
		INIT_HLIST_HEAD(&trace->hpids[i]);
	}
	return trace->sfd;
}

static struct trace_process *trace_process_new(struct trace *trace, int pid) {
	struct trace_process *process = calloc(1, sizeof(struct trace_process));
	process->pid = pid;
	process->trace = trace;
	trace->process_count += 1;
	hlist_add_head(&process->node, &trace->hpids[pid % HPIDS_SIZE]);
	return process;
}

static void trace_process_del(struct trace_process *process) {
	hlist_del(&process->node);
	process->trace->process_count -= 1;
	free(process);
}

static void ptrace_prepare(int pid) {
	int r = ptrace(PTRACE_SETOPTIONS, pid, 0,
		       PTRACE_O_TRACESYSGOOD |
		       PTRACE_O_TRACEFORK |
		       PTRACE_O_TRACEVFORK |
		       PTRACE_O_TRACECLONE |
		       PTRACE_O_TRACEEXEC |
		       PTRACE_O_TRACEEXIT);
	if (r != 0)
		perror("ptrace(PTRACE_SETOPTIONS)");
}

int trace_execvp(struct trace *trace, char **argv) {
	int pid = fork();
	if (pid == -1)
		PFATAL("fork()");

	if (pid == 0) {
		// Child
		close(trace->sfd);

		// Restore default sigprocmask
		sigset_t mask;
		sigemptyset(&mask);
		sigaddset(&mask, SIGCHLD);
		sigprocmask(SIG_UNBLOCK, &mask, NULL);

		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		// Wait for parent to catch up.
		raise(SIGSTOP);
		
		execvp(argv[0], argv);
		PFATAL("execvp()");
	}
	struct trace_process *child_process = trace_process_new(trace, pid);
	child_process->procdata = process_start(pid, trace->tracedata);
	return pid;
}

static void trace_waitpid(struct trace *trace, struct signalfd_siginfo *fdsi) {
	int inject_signal = 0;
	int status = 0;
	int pid = fdsi->ssi_pid;
	int r = waitpid(pid, &status, WNOHANG);
	if (r != pid)
		PFATAL("waitpid()");

	struct trace_process *process;
	struct hlist_node *pos;
	hlist_for_each(pos, &trace->hpids[pid % HPIDS_SIZE]) {
		process = hlist_entry(pos, struct trace_process, node);
		if (process->pid == pid) {
			break;
		}
		process = NULL;
	}
	if (!process) {
		fprintf(stderr, "Received SIGCHLD from unknown process pid=%i stats=%i\n",
			pid, status);
		return;
	}

	if (!process->initialized) {
		/* First child SIGSTOPs itself after calling TRACEME,
		   descendants are STOPPED due to TRACEFORK. */
		if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGSTOP) {
			PFATAL("process in a wrong state %i %i %i\n", WIFSTOPPED(status), WSTOPSIG(status), SIGSTOP);
		}
		ptrace_prepare(pid);
		process->initialized = 1;
	} else {
		if (WIFEXITED(status)) {
			int exit_status = WEXITSTATUS(status);
			process_stop(process->procdata, TYPE_EXIT, exit_status);
			trace_process_del(process);
		} else
		if (WIFSIGNALED(status)) {
			int signal = WTERMSIG(status);
			process_stop(process->procdata, TYPE_SIGNAL, signal);
			trace_process_del(process);
		} else
		if (WIFSTOPPED(status)) {
			/* We can't use WSTOPSIG(status) as it cuts high bits. */
			int signal = (status >> 8) & 0xffff;
			switch (signal) {
			case SIGTRAP | 0x80: { // assuming PTRACE_O_SYSGOOD
				REGS_STRUCT regs;
				if (ptrace(PTRACE_GETREGS, pid, 0, &regs) < 0)
					PFATAL("ptrace(PTRACE_GETREGS)");
				int syscall_entry = SYSCALL_ENTRY;
				struct parameters params = {SYSCALL, {ARG1, ARG2, ARG3, ARG4, ARG5, ARG6}};
				if (syscall_entry != !process->within_syscall) {
					fprintf(stderr, "entry - exit desynchronizaion\n");
				} else {
					process_syscall(process->procdata, process->within_syscall, &params);
				}
				process->within_syscall = !process->within_syscall;
				} break;
			case SIGTRAP:
				/* Got a pure SIGTRAP - why? Let's assume it's
				   just a normal signal */
				inject_signal = signal;
				fprintf(stderr, "[.] got a signal SIGTRAP\n");
				break;
			case SIGTRAP | (PTRACE_EVENT_FORK  << 8):
			case SIGTRAP | (PTRACE_EVENT_VFORK << 8):
			case SIGTRAP | (PTRACE_EVENT_CLONE << 8): {
				unsigned long child_pid;
				ptrace(PTRACE_GETEVENTMSG, pid, NULL, &child_pid);
				fprintf(stderr, "[.] fork() or equivalent, child = %li\n", child_pid);
				struct trace_process *child_process = trace_process_new(trace, child_pid);
				child_process->procdata = process_start(child_pid, trace->tracedata);
				} break;
			case SIGTRAP | PTRACE_EVENT_EXEC << 8:
				// exec()
				break;
			case SIGTRAP | PTRACE_EVENT_EXIT << 8:
				// exit()
				break;
			default:
				inject_signal = signal;
				fprintf(stderr, "[.] got a signal %i\n", signal);
				break;
			}
		} else {
			fprintf(stderr, "[.] status %x not understood!\n", status);
		}
	}
	ptrace(PTRACE_SYSCALL, pid, 0, inject_signal);
}


/* Call this method when sfd is readable */
void trace_read(struct trace *trace) {
	struct signalfd_siginfo buf[4];

	int r = read(trace->sfd, &buf, sizeof(buf));
	if (r % sizeof(struct signalfd_siginfo) != 0)
		PFATAL("read not aligned");
	unsigned i;
	for (i=0; i < r / sizeof(struct signalfd_siginfo); i++) {
		trace_waitpid(trace, &buf[i]);
	}
}
