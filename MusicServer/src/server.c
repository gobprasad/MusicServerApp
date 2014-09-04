#include "server.h"
#include "gthreads.h"
#include "serverRoutine.h"
#include <netdb.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>


#define MAX_CONNECTION	5
#define SERVER_ADDRESS	"192.168.2.2"
#define SERVER_PORT	8090

void startServerThread()
{
	pthread_t serverThread;
	pthread_create(&serverThread,NULL,startServerSocket,NULL);
	// Sleep for one second
	sleep(1);
}

// SERVER SOCKET THREAD ROUTINE
// ALWAYS ON For Clients to connect
void *startServerSocket(void *arg)
{
	struct serverArg *servArg = (struct serverArg *)arg;
	struct sockaddr_in  cli_addr;
	socklen_t clilen = sizeof(struct sockaddr_in);
	int clntSockFd = -1;

	int sockFd = createServerSocket(SERVER_ADDRESS, SERVER_PORT);
	if(sockFd < 0){
		printf("Fatal Error... Socket Creation Failed\n");
		exit(0);
	}

	//free(servArg);

	listen(sockFd,MAX_CONNECTION);
	
	while(1)
	{	
		// Accept connection from clients; Blocking call
		clntSockFd = accept(sockFd, (struct sockaddr *)&cli_addr, &clilen);
		if (clntSockFd < 0)
			printf("ERROR on accept\n");
		
		// Create Job Args
		clntMsg_t *newClntMsg = (clntMsg_t *)malloc(sizeof(clntMsg_t));
		newClntMsg->clntSockFd = clntSockFd;
		newClntMsg->cli_addr   = cli_addr.sin_addr.s_addr;
		printf("connected to sockfd %d\n",clntSockFd);
		// Add clnt handling to Job Queue
		addJobToQueue(handleClient, (void *)newClntMsg);
	}
}
