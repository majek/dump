#include <stdio.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <wait.h>

#include <linux/user.h>
#include <linux/stddef.h>

unsigned int debug_reg = 0x0;
	
void signal_handler(int sig){
	switch(sig){
	case SIGTRAP:
		printf("#%i got SIGTRAP\n", getpid());
		break;
	default:
		printf("#%i odebrano %i\n", getpid(), sig);
	}
}

int main(void)
{
	pid_t pid;
	unsigned int dr0, dr7;
	int status;
	
	signal(SIGTRAP, signal_handler);

	switch (pid = fork()) {
	case -1:
		perror("fork");
		break;
	case 0: /*  child process starts        */
		printf("#%i child started\n", getpid());
		if(ptrace(PTRACE_TRACEME, 0, 0, 0))
			perror("ptrace");
		
		/* Inform parent, (yep, our signal will be send to parent as SIGCHLD)
		  that we're ready to set our DR0 and DR7.*/
		kill(getpid(), SIGURG);
		
		/* invoke SIGTRAP for our process, and SIGCHLD for parent */
		asm volatile ("incl %0" : "=m"(debug_reg ));
		
		printf("#%i child stopped\n", getpid());
                break;
                /*  child process ends  */
        default:/*  parent process starts       */
		printf("#%i parent started\n", getpid());
		wait(&status); // Wait for child's signal
			
		dr0 = ptrace(PTRACE_PEEKUSER, pid, offsetof (struct user, u_debugreg[0]), 0);
		dr7 = ptrace(PTRACE_PEEKUSER, pid, offsetof (struct user, u_debugreg[7]), 0);
		printf("before change: DR0=0x%08x DR7=0x%08x\n", dr0, dr7);
			
		ptrace(PTRACE_POKEUSER, pid, offsetof (struct user, u_debugreg[0]), &debug_reg);
		ptrace(PTRACE_POKEUSER, pid, offsetof (struct user, u_debugreg[7]), 0x50101);
		
		dr0 = ptrace(PTRACE_PEEKUSER, pid, offsetof (struct user, u_debugreg[0]), 0);
		dr7 = ptrace(PTRACE_PEEKUSER, pid, offsetof (struct user, u_debugreg[7]), 0);
		printf("after change:  DR0=0x%08x DR7=0x%08x\n", dr0, dr7);
		
		if (ptrace(PTRACE_CONT, pid, 0, 0) != 0)
			perror("PTRACE_CONT");
		printf("#%i parent stopped\n", getpid());
		break;
	}
	return 0;
}

