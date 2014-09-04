#include "gthreads.h"
#include "gsocket.h"
#include "queue.h"
#include "results.h"
#include "packet.h"
#include <stdio.h>
#include <stdlib.h>


/*
LIST testList;


void printCB(void *data);

void InsertFirst(){
	int *num = (int*)malloc(sizeof(int));
	printf("Enter a Number to Insert : ");
	scanf("%d",num);
	addFirst(&testList,num);
}

void InsertLast(){
	int *num = (int*)malloc(sizeof(int));
	printf("Enter a Number to Insert : ");
	scanf("%d",num);
	addLast(&testList,num);
}

void DelFirst(){
	int *num = (int *)deleteFirst(&testList);
	if(num != NULL){
		printf("Deleted Item is %d\n",*num);
	} else {
		printf("Error in Delete\n");
	}
}

void DelLast(){
	int *num = (int *)deleteLast(&testList);
	if(num != NULL){
		printf("Deleted Item is %d\n",*num);
	} else {
		printf("Error in Delete\n");
	}
}

void exitProgram(){
	int *num = NULL;
	while( (num = deleteFirst(&testList)) != NULL){
		printf("Deleted Item %d\n",*num);
	}
	exit(0);
}

void printContent(){
	printList(&testList,printCB);

}

void printCB(void *data){
	printf("%d->",*(int*)data);

}

void printListHelp(){
	printf("1 : Insert to first\n");
	printf("2 : Insert to Last\n");
	printf("3 : Delete First\n");
	printf("4 : Delete Last\n");
	printf("5 : Print List Content\n");
	printf("6 : Exit\n");
	printf("\nEnter any one from 1-5\n");
}
int main(){
	initList(&testList);
	int choice = -1;
	while(1){
		printListHelp();
		scanf("%d",&choice);
		switch(choice){
			case 1:
				InsertFirst();
				break;
			case 2:
				InsertLast();
				break;
			case 3:
				DelFirst();
				break;
			case 4:
				DelLast();
				break;
			case 5:
				printContent();
				break;
			case 6:
				exitProgram();
				break;
			default:
				printf("Enter num 1-6 only\n\n");
				break;
		}
	}
}
*/



/*
void printFunctions(void *data){
	printf(" %d \n",*(int*)data);
	//sleep();
}

#define SERV_ADDRESS "192.168.2.4"
int main(){
	pthread_t th;
	int *data = NULL;
	int count = 0, clntSockFd = -1;
	createThreadPool();
	int sockFd = createServerSocket(SERV_ADDRESS, 8081);
	if(sockFd < 0){
		printf("Fatal Error... Socket Creation Failed\n");
		exit(0);
	}

	listen(sockFd,MAX_CONNECTION);
	
	while(1)
	{	
		// Accept connection from clients; Blocking call
		clntSockFd = accept(sockFd, (struct sockaddr *)&cli_addr, &clilen);
		if (clntSockFd < 0)
			printf("ERROR on accept\n");
		
		// Create Job Args
		CLNT_SOC_ARGS *newArgs = (CLNT_SOC_ARGS *)malloc(sizeof(CLNT_SOC_ARGS));
		newArgs->clntSockFd = clntSockFd;
		newArgs->cli_addr   = cli_addr;
		
		// Add clnt handling to Job Queue
		addJobToQueue(handleClntSocketRoutine, (void *)newArgs);
	}
}
*/
/*
int main(){
	CIRCULAR_QUEUE q;
	initCircQueue(&q,4096,10);
	int i = 0;
	for(i=0; i<100; i++)
		printf("%#x\n",(unsigned int)getCircularQueueNext(&q));

	cleanCircQueue(&q);
}*/



int main(){
	gen_msg_t in, out;
	in.header.token = 1000;
	in.header.msgId = update_m;
	in.header.clntId = 5;
	in.header.signature = 55;
	in.payLoad = NULL;
	in.payloadSize = 0;

	void *buff = NULL;
	printf("Encoded size %d\n",encodePacket(&in,&buff));
	
	if(decodePacket(&out,buff+5,0) == G_OK)
	{
		printf("token %d, msgId %d, clntId %d, signature %d\n",out.header.token,out.header.msgId,out.header.clntId,out.header.signature);

	} else {
		printf("Error in decoding\n");
	}
}
