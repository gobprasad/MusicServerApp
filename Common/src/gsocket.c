/*************************************************************
 * gsocket.c : Socket Implementation
 * Copyright (C) 2014  Gobind Prasad (gobprasad@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *
 ************************************************************/


#include "gsocket.h"
#include "results.h"
#include <netdb.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <error.h>

#include <stdio.h>



static unsigned long getIPAddress(char *address );


/**
 * General API : Create Server Socket
 * Creates TCP socket with given name and port
 * returns Socket fd.
 * User has to use this fd for listen and accept
 */
int createServerSocket(char *localAdd, int port)
{
	int sockFd = -1;
	unsigned long localIp = 0;
	struct sockaddr_in myaddr;
	memset((char *)&myaddr, 0, sizeof(myaddr));

	localIp = getIPAddress(localAdd);
	if(!localIp){
		printf("Unable to get local IP address\n");
		return -1;
	}

	if((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("\nUserMsg :Unable to Create TCP Socket\n");
		return -1;
	}

	myaddr.sin_family = AF_INET;
	//myaddr.sin_addr.s_addr = localIp;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);// bind to any ip available
	myaddr.sin_port = htons(port);

	if (bind(sockFd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		printf("\nUserMsg :Bind failed\n");
		return -1;
	}
	return sockFd;
}


/**
 * General API : Create Client Socket
 * Creates TCP socket and connect to given server and port
 * returns Socket fd.
 */
int createClientSocket(char *serverAdd, int port)
{
	int sockfd = -1;
	struct sockaddr_in serv_addr;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Error : Could not create socket \n");
	        return -1;
	} 
	memset(&serv_addr, '0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	
	if(inet_pton(AF_INET, serverAdd, &serv_addr.sin_addr)<=0)
	{
		printf("\n inet_pton error occured\n");
		return -1;
	} 

	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\n Error : Connect Failed \n");
		return -1;
	}
	return sockfd;
}

/**
 * GetIPAddress : return long IP address
 */
static unsigned long getIPAddress(char *address ){ 
    unsigned long hostaddr = 0;
    hostaddr = inet_addr( address );
    if( hostaddr == INADDR_NONE ){
	struct hostent *host = NULL;
	host = gethostbyname( address );
	hostaddr = *(host->h_addr_list[0]);
    }
    return ((hostaddr == INADDR_NONE) ? 0 : hostaddr );
}

/**
 * General API : recevive Data  Blocking call
 * receive data from socket of given size into the given buffer
 */
unsigned int receiveData(int sockFd, unsigned int size, char *buf)
{
	//printf("Receiving from sockfd %d\n",sockFd);
	unsigned int totalRecv = 0;
	int receive = 0;
	while(	totalRecv < size )
	{
		receive = recv(sockFd ,buf+totalRecv , size-totalRecv , 0);
		if(receive > 0 )
		{
			totalRecv += receive;
		} else if(receive == 0){
			continue;
		} else {
			break;
			printf("Client Disconnected %d Returned in recv\n",receive);
		}
	}
	return totalRecv;
}

/**
 * General API : send Data  Blocking call
 * Send data to socket of given size from the given buffer
 */
unsigned int sendData(int sockFd, unsigned int size, char *buf)
{
	unsigned int totalSend = 0;
	printf("Sending to sockfd %d\n",sockFd);
	int sent = 0;
	while(	totalSend < size )
	{
		sent = write(sockFd ,buf+totalSend , size-totalSend);
		if(sent > 0 )
		{
			totalSend += sent;
		} else if(sent == 0){
			continue;
		} else {
			break;
			printf("Client Disconnected %d Returned in send\n",sent);
		}
	}
	return totalSend;
}

/**
 * General API : close socket
 * close socket of specified fd
 */
void closeSocket(int sockFd)
{
	close(sockFd);
}

