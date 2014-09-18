#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/tcp.h>
#include <linux/socket.h>
//#include <netinet/tcp.h>

#define F_SETSIG    10
#define SOL_TCP          6
#define TIMEVAL_SUBTRACT(a,b) (((a).tv_sec - (b).tv_sec) * 1000000 + (a).tv_usec - (b).tv_usec)


int connect_tcp(char *host, int port){
	static struct hostent *phe;
	if(!( phe = gethostbyname(host)) ){
		perror("gethostbyname()");
		abort();
	}

	struct sockaddr_in sin;     /* an Internet endpoint address         */
	int s;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons ( port);
	memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);

	if((s = socket(PF_INET, SOCK_STREAM, 0))<0){
		perror("socket()");
		abort();
	}
	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("connect()");
		abort();
	}

/*
	//sigset_t m_sigset;
	int flags;

	fcntl(s, F_SETOWN, (int) getpid());
	fcntl(s, F_SETSIG, signum);
	flags = fcntl(s, F_GETFL);
	flags |= O_NONBLOCK|O_ASYNC;
	fcntl(s, F_SETFL, flags);

	int option = 1;
	if(setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &option, sizeof(option)))
		perror("SO_KEEPALIVE");

	if(setsockopt(s, SOL_TCP, TCP_KEEPIDLE, &option, sizeof(option)))
		perror("TCP_KEEPIDLE");

	if(setsockopt(s, SOL_TCP, TCP_KEEPINTVL, &option, sizeof(option)))
		perror("TCP_KEEPINTVL");

	if(setsockopt(s, SOL_TCP, TCP_KEEPCNT, &option, sizeof(option)))
		perror("TCP_KEEPCNT");
*/



	return s;
}
#include <poll.h>


void handler(int s){
	printf("SIG:%i\n", s);
}

int main(){
	signal(SIGURG, handler);

	int sd = connect_tcp("localhost", 10000);

	if (fcntl(sd, F_SETOWN, getpid()) < 0) {
		perror("fcntl()");
		exit(-1);
	}

	char buf[128];
	while(1){
		//char bufa[] = "Dupa!";
		//send(sd, bufa, strlen(bufa), MSG_OOB);
		struct pollfd pfd[1] = {{sd, POLLIN|POLLPRI|POLLERR|POLLHUP,0}}; //

		int flags = fcntl(sd, F_GETFL, 0);
		int ret = fcntl(sd, F_SETFL, flags | O_NONBLOCK);

		int r = poll(&pfd, 1, 1000);
		printf("poll() = %i\n",r);
		if(r<0)
			perror("poll()");
		if(r>0){
			int r = recv(sd, buf, sizeof(buf)-1, MSG_PEEK);
			printf("MSG_PEEK %i\n",r);

			r = recv(sd, buf, sizeof(buf)-1, 0);
			if(r>0){
				buf[r] = '\0';
				printf("%i >%s<\n", r, buf);
			}else
				printf(".");
		}
	}

	return(0);
}
