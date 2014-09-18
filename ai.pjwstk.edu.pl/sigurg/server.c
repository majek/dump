#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>         /* bind(), listen(), accept()      */
//#include <linux/in.h>           /* struct sockaddr_in              */
#include <sys/wait.h>           /* waitpid()                       */
#include <unistd.h>             /* fork()                          */
#include <signal.h>             /* signal()                        */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>

#define SPORT 10000             /* port serwera                    */

// Właściwy kod:

void sigchld_handler (int sig){
	int pid;

	while ((pid = waitpid (-1, NULL, WNOHANG)) > 0);
}


void handler(int s){
	printf("SIG:%i\n", s);
}

int main ()
{
	signal(SIGURG, handler);
	int sd, ret;
	struct sockaddr_in saddr, /* adres lokalnego gniazda        */
			caddr; /* adres zdalnego gniazda         */

	sd = socket (PF_INET, SOCK_STREAM, 0);
	if (sd < 0){
		perror ("socket()");
		exit (1);
	}

	saddr.sin_family = PF_INET;
	saddr.sin_port = htons (SPORT);
	saddr.sin_addr.s_addr = INADDR_ANY;

	int ign;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&ign, sizeof(ign));


	if (bind (sd, (struct sockaddr *) &saddr, sizeof (saddr))){
		perror ("bind()");
		exit (1);
	}

	if (listen (sd, 5)){
		perror ("listen()");
		exit (1);
	}

	signal (SIGCHLD, sigchld_handler);
	while (1){
		socklen_t alen;
		alen = sizeof (saddr);
		int cd = accept (sd, (struct sockaddr *) &caddr, &alen);
		if (cd < 0){
			perror ("accept()");
			exit (1);
		}
		setsockopt(cd, SOL_SOCKET, SO_REUSEADDR, (char *)&ign, sizeof(ign));
		printf ("Polaczenie od klienta %s:%i\n", inet_ntoa (caddr.sin_addr),
				ntohs (caddr.sin_port));
		if ((ret = fork ()) == 0){			/* Jesteśmy dzieckiem */
			/*
			dup2 (cd, 0);
			dup2 (cd, 1);
			dup2 (cd, 2);
			close (sd);
			printf ("\nProsze czekac. Uruchamiam powloke ...\n\n");
			// send (cd, "\nProsze czekac. Uruchamiam powloke ...\n\n", 40, 0);
			if (execl ("/bin/bash", "bash", "-i", NULL) < 0){
				perror("execl()");
				exit(1);
			}
			*/
			if (fcntl(cd, F_SETOWN, getpid()) < 0) {
				perror("fcntl()");
				exit(-1);
			}


			struct pollfd pfd[1] = {{cd, POLLIN|POLLPRI|POLLERR|POLLHUP,0}};
			signal(SIGURG, handler);

			int flags = fcntl(cd, F_GETFL, 0);
			fcntl(cd, F_SETFL, flags | O_NONBLOCK);

			int r = 1;
			while(r >= 0){
				r = poll(&pfd[0], 1, 5000);
				printf("poll() = %i\n",r);
				if(r<0)
					perror("poll()");
				if(r>0){
					char buf[128];
					int ret = recv(cd, buf, sizeof(buf)-1, MSG_PEEK);
					printf("MSG_PEEK %i\n",ret);

					ret = recv(cd, buf, sizeof(buf)-1, 0);
					if(ret>0){
						buf[ret] = '\0';
						printf("%i >%s<\n", ret, buf);
					}else{
						printf(".");
						exit(1);
					}
				}
			}

			exit(0);
		}else if (ret < 0){
			perror ("fork()");
			exit(1);
		}

		/* Jesteśmy rodzicem */
		close (cd);
	}
}
