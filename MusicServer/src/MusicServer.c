#include "resourceManager.h"
#include "clientdb.h"
#include "gthreads.h"
#include "server.h"

#include <stdlib.h>
#include <stdio.h>

int main(){

	CLIENT_DB *cdb = getClientDbInstance();
	if(!cdb){
		printf("Client DB initialization Error\n");
		exit(0);
	}
	createThreadPool();
	printf("ThreadPool started\n");
	startServerThread();
	printf("Server started\n");
	RManager *rm = getRManagerInstance();
	rm->runRM(rm);
}
