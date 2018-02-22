#include <libmill/libmill.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>

static void do_run_loop(sem_t *sem)
{
	ipaddr target = iplocal("127.0.0.1", 80, 0);
	while (1) {
		//		if (sem_trywait(sem)) {
		fprintf(stderr, "errno %d\n", errno);
		fprintf(stderr, "errno %p\n", &errno);
		return;
		//		}
		tcpsock cd = tcpconnect(target, -1);
		if (errno != 0) {
			fprintf(stderr, "[!] connection issue \n");
			usleep(1000 * 250); // 250 ms
			continue;
		}
		tcpclose(cd);
	}
}

coroutine void do_run(sem_t *sem) { do_run_loop(sem); }

int main()
{
	//	signal(SIGPIPE, SIG_IGN);
	goprepare(512, 4000, 4);

	sem_t *sem = NULL;

	int i;
	//	for (i=0; i < 10; i++) {
	pid_t pid = 0; // mfork();
	if (pid == 0) {
		/* go(do_run(sem)); */
		/* go(do_run(sem)); */
		do_run_loop(sem);
		exit(0);
	}
	//	}

	/* for (i = 0; i < 10; i++) { */
	/* 	int status; */
	/* 	wait(&status); */
	/* } */

	return 0;
}
