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
#include <fcntl.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include "loggingFrameWork.h"

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
	fd_set writeFDs;
	struct time_val timeout1;
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
	
	if(setSocketBlockingEnabled(sockFd,1) != G_OK)
	{
		LOG_ERROR("Unable to set socket non blocking");
		return G_FAIL;
	}

   	FD_ZERO(&writeFDs);
   	FD_SET(sockFd, &writeFDs);

   	timeout1.tv_sec = 0;
   	timeout1.tv_usec = 500000;

	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		if(select (sockFd+1, (fd_set *)NULL, &writeFDs, (fd_set *)NULL, (struct timeval *)(&timeout1))  > 0 )
		{
        	return sockfd;
		}
    	else
    	{
			printf("\n Error : Connect Failed \n");
			return -1;
    	}
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
	struct timeval tVal;
	tVal.tv_sec  = 3;
	tVal.tv_usec = 0;
	int retVal;
	
	//printf("Receiving from sockfd %d\n",sockFd);
	unsigned int totalRecv = 0;
	int receive = 0;
	fd_set readfds, activeFdSet;
	FD_ZERO(&activeFdSet);
	FD_SET(sockFd, &activeFdSet);
	while(	totalRecv < size )
	{
		readfds = activeFdSet;
		if((retVal = select(sockFd+1, &readfds, NULL, NULL, &tVal)) == -1)
		{
			LOG_ERROR("FATAL ERROR : Select fail");
			return 0;
		}
		if(retVal == 0)
		{
			LOG_ERROR("Timeout in Receiving");
			return 0;
		}
		if(!FD_ISSET(sockFd, &readfds))
		{
			LOG_WARN("My Fd is not set");
			continue;
		}
		receive = recv(sockFd ,buf+totalRecv , size-totalRecv , 0);
		if(receive > 0 )
		{
			totalRecv += receive;
			//TODO need to check this one;
		} else if(receive < 0){
			return 0;
		} else {
			LOG_WARN("Client Disconnected %d Returned in recv\n",receive);
			break;
		}
	}
	FD_CLR(sockFd,&activeFdSet);
	return totalRecv;
}

/**
 * General API : send Data  Blocking call
 * Send data to socket of given size from the given buffer
 */
unsigned int sendData(int sockFd, unsigned int size, char *buf)
{
	struct timeval tVal;
	tVal.tv_sec  = 3;
	tVal.tv_usec = 0;
	int retVal = 0;
	
	unsigned int totalSend = 0;

	fd_set writefds, activeFdSet;
	FD_ZERO(&activeFdSet);
	FD_SET(sockFd, &activeFdSet);
	
	//printf("Sending to sockfd %d\n",sockFd);
	int sent = 0;
	while(totalSend < size )
	{
		writefds = activeFdSet;
		if((retVal = select(sockFd+1, NULL, &writefds, NULL, &tVal)) == -1)
		{
			LOG_ERROR("FATAL ERROR : Select fail");
			return 0;
		}
		if(retVal == 0)
		{
			LOG_ERROR("Timeout in Sending");
			return 0;
		}
		if(!FD_ISSET(sockFd, &writefds))
		{
			LOG_WARN("My Fd is not set");
			continue;
		}
		sent = write(sockFd ,buf+totalSend , size-totalSend);
		if(sent > 0 )
		{
			totalSend += sent;
		} else if(sent < 0){
			return 0;
		} else {
			LOG_ERROR("Client Disconnected %d Returned in send\n",sent);
			break;
		}
	}
	FD_CLR(sockFd,&activeFdSet);
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

/** Returns true on success, or false if there was an error */
RESULT setSocketBlockingEnabled(int fd, char blocking)
{
   if (fd < 0) return G_FAIL;

#ifdef WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? G_OK : G_FAIL;
#else
   //int flags = fcntl(fd, F_GETFL, 0);
   //if (flags < 0) return G_FAIL;
   //flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
   return (ioctl(fd, (int)FIONBIO,&blocking) == 0) ? G_OK : G_FAIL;
#endif
}

