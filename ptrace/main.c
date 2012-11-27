#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <sys/reg.h>

#ifdef __x86_64__
#define SC_NUMBER  (8 * ORIG_RAX)
#define SC_RETCODE (8 * RAX)
#else
#define SC_NUMBER  (4 * ORIG_EAX)
#define SC_RETCODE (4 * EAX)
#endif


/*
http://strace.git.sourceforge.net/git/gitweb.cgi?p=strace/strace;a=blob;f=README-linux-ptrace;hb=HEAD
*/

static void parent_syscall(int pid) {
	struct user_regs_struct u_in;
	ptrace(PTRACE_GETREGS, pid, 0, &u_in);
	int syscall = u_in.orig_eax;
	int sc_number = ptrace(PTRACE_PEEKUSER, pid, SC_NUMBER, NULL);
	int sc_retcode = ptrace(PTRACE_PEEKUSER, pid, SC_RETCODE, NULL);
	fprintf(stderr, "[.] syscall entry syscall=0x%x sc=%i rc=%i\n",
		syscall, sc_number, sc_retcode);
}

static void parent_loop(int pid) {
	int r;
	// the child send SIGSTOP to itself, continue :)
	waitpid(pid, NULL, WCONTINUED);

	int sysgood = 0;
	r = ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
	if (r == 0) {
		sysgood = 0x80;
	} else {
		perror("ptrace(PTRACE_O_TRACESYSGOOD)");
	}

	int within_syscall = 0;
	int inject_signal = 0;
	while (1) {
		int status;

		ptrace(PTRACE_SYSCALL, pid, 0, inject_signal);
		inject_signal = 0;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			int exit_status = WEXITSTATUS(status);
			fprintf(stderr, "[.] exit with status = %i\n", exit_status);
			exit(exit_status);
		}
		if (WIFSIGNALED(status)) {
			int signal = WTERMSIG(status);
			fprintf(stderr, "[.] terminated by a signal %i\n", signal);
			break;
		}
		if (WIFSTOPPED(status)) {
			int signal = WSTOPSIG(status);
			if (signal != (SIGTRAP | sysgood)) {
				if (signal != SIGTRAP) {
					inject_signal = signal;
					fprintf(stderr, "[.] got a signal %i\n", signal);
				} else {
					fprintf(stderr, "[.] got a SIGTRAP, execve in action\n");					
				}
				continue;
			} else {
				if (!within_syscall) {
					parent_syscall(pid);
				} else {
					parent_syscall(pid);
					//fprintf(stderr, "[.] syscall exit\n");
				}
				within_syscall = !within_syscall;
			}
			continue;
		}
		fprintf(stderr, "[.] status %x not understood!\n", status);
	}
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
		exit(-1);
	}

	int pid = fork();
	if (pid == -1) {
		perror("fork()");
		exit(-1);
	}
	if (pid != 0) {
		// parent
		parent_loop(pid);
	} else {
		// child
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		// wait for parent to stand up.
		raise(SIGSTOP);

		char buf[1024];
		argv_join(buf, sizeof(buf), &argv[1], " ");
		fprintf(stderr, "[+] pid=%i, running: %s\n", getpid(), buf);
		
		execvp(argv[1], &argv[1]);
		perror("execvp()");
	}
	exit(-1);
}


