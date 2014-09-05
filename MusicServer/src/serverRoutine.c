#include "serverRoutine.h"
#include "resourceManager.h"
#include "server.h"
#include "packet.h"
#include "results.h"
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

static void freeClientMsg(clntMsg_t *newClntMsg);
static void postFileTransferStatus(rmMsg_t *newRmMsg);


void *handleClient(void *msg)
{
	u8 signature;
	u32 totSize = 0;
	void *clntBuf = NULL;
	char buf[5] = {'\0'};
	u32 msgLen = 0, recevLen = 0;

	clntMsg_t *newClntMsg = (clntMsg_t *)msg;

	/*printf("Client Connected - %d.%d.%d.%d\n", (int)(newClntMsg->cli_addr.sin_addr.s_addr&0xFF),(int)((newClntMsg->cli_addr.sin_addr.s_addr&0xFF00)>>8),(int)((newClntMsg->cli_addr.sin_addr.s_addr&0xFF0000)>>16),(int)((newClntMsg->cli_addr.sin_addr.s_addr&0xFF000000)>>24));*/


	// Get the Signature and file Size to receive
	receiveData(newClntMsg->clntSockFd, 5, buf);
	memcpy(&signature,buf,1);

	// Signature Validation
	if(signature != SIGNATURE)
	{
		printf("Unknown Client\n");
		closeSocket(newClntMsg->clntSockFd);
		freeClientMsg(newClntMsg);
		return;
	}
	memcpy(&totSize,buf+1,4);
	totSize = ntohl(totSize);
	if(totSize > 4096)
	{
		printf("Total Size of client Data is %d, Error\n",totSize);
		closeSocket(newClntMsg->clntSockFd);
		freeClientMsg(newClntMsg);
		return;
	}
	// Recieve all Data from Client
	clntBuf = malloc(totSize-5);
	receiveData(newClntMsg->clntSockFd, totSize-5, clntBuf);
	
	//decode client Packet
	if(decodePacket(&newClntMsg->clntData, clntBuf, totSize-5) != G_OK){
		free(clntBuf);
		sendNACKandClose((void *)newClntMsg);
		return;
	}

	
	// create new message for Resource Manager
	rmMsg_t *newRmMsg = (rmMsg_t *)malloc(sizeof(rmMsg_t));
	if(newRmMsg == NULL)
	{
		printf("Error [%s : %d] : Malloc failed\n",__FUNCTION__,__LINE__);
		free(clntBuf);
		sendNACKandClose((void *)newClntMsg);
		return;
	}
	// fill new message for resource manager
	newRmMsg->msgId = rm_clntMsg_m;
	newRmMsg->data    = (void *)newClntMsg;

	// Get Resource Manager Instance and post message
	RManager *rm = getRManagerInstance();
	
	if(rm->postMsgToRm(rm, newRmMsg) != G_OK)
	{
		free(newRmMsg);
		free(clntBuf);
		sendNACKandClose((void *)newClntMsg);
	}
	
	free(clntBuf);
	return;
}

void *sendNACKandClose(void *arg)
{
	clntMsg_t *newClntMsg = (clntMsg_t*)arg;
	// Update Error Response
	newClntMsg->clntData.header.msgId = resErr_m;
	int size = 0;
	void *buf = NULL;
	size = encodePacket(&newClntMsg->clntData,&buf);
	if(size <= 0)
	{
		printf("[%s : %d] : Error while sending NACK\n");
		closeSocket(newClntMsg->clntSockFd);
		freeClientMsg(newClntMsg);
		return;
	}
	// Send Data to client
	sendData(newClntMsg->clntSockFd,size,buf);
	closeSocket(newClntMsg->clntSockFd);
	freeClientMsg(newClntMsg);
}

void *sendACKandClose(void *arg)
{
	clntMsg_t *newClntMsg = (clntMsg_t*)arg;
	// Update Error Response
	newClntMsg->clntData.header.msgId = resOk_m;
	int size = 0;
	void *buf = NULL;
	// buffer will be created inside encode Packet;
	size = encodePacket(&newClntMsg->clntData,&buf);

	if(size <= 0 || !buf)
	{
		printf("[%s : %d] : Error while sending NACK\n");
		closeSocket(newClntMsg->clntSockFd);
		freeClientMsg(newClntMsg);
		return;
	}
	// Send Data to client
	size = sendData(newClntMsg->clntSockFd,size,buf);
	printf("Total Size sent %d\n",size);
	if(buf != NULL)
		free(buf);

	//if this is last packet from client close the connection or read new packet
	if(newClntMsg->clntData.header.isLast == 1)
	{
		closeSocket(newClntMsg->clntSockFd);
		freeClientMsg(newClntMsg);
	} else {
		
		if(newClntMsg->clntData.payLoad != NULL)
			free(newClntMsg->clntData.payLoad);
		memset(&newClntMsg->clntData,0, sizeof(newClntMsg->clntData));
		addJobToQueue(handleClient, (void *)newClntMsg);
	}
}

static void freeClientMsg(clntMsg_t *newClntMsg)
{
	if(!newClntMsg)
		return;

	if(newClntMsg->clntData.payLoad != NULL)
		free(newClntMsg->clntData.payLoad);

	free(newClntMsg);
}

void *getMP3FileFromClient(void *msg)
{
	int sockFd = -1;
	uchar buffer[4096];
	char clntSig = CLNT_SIGNATURE;
	u32 fileSize = 0;
	u32 totalRead = 0;
	int fd  = -1;
	u32 reqID = 0;

	// create new message for Resource Manager
	rmMsg_t *newRmMsg = (rmMsg_t *)malloc(sizeof(rmMsg_t));
	if(newRmMsg == NULL)
	{
		printf("Error [%s : %d] : FATAL ERROR Resource Manager may stuck Malloc failed\n",__FUNCTION__,__LINE__);
		return;
	}
	MP3_FILE_REQ *req = (MP3_FILE_REQ*)msg;
	
	// fill new message for resource manager
	newRmMsg->msgId = rm_dw_err_m;
	newRmMsg->data    = (void *)req;

	if(req == NULL)
	{
		printf("[%s : %d ] Request Message is NULL\n",__FUNCTION__,__LINE__);
		postFileTransferStatus(newRmMsg);
		return NULL;
	}
	
	struct in_addr addr = {req->ipaddress};
	
	sprintf(buffer, "%s", inet_ntoa( addr ) );
	printf("Sending request for MP3 file to clntId %d, token %d, address %s\n",req->clntId,req->requestId,buffer);
	
	sockFd = createClientSocket(buffer,8091);
	if(sockFd == -1){
		printf("[%s : %d ] Unable to connect with client\n",__FUNCTION__,__LINE__);
		postFileTransferStatus(newRmMsg);
		return NULL;
	}
	memcpy(buffer,&clntSig,1);
	memcpy(buffer+1,&req->clntId,1);
	reqID = htonl(req->requestId);
	memcpy(buffer+2,&reqID,4);
	if(sendData(sockFd,6,buffer) < 6)
	{
		printf("[%s : %d ] Unable to send \n",__FUNCTION__,__LINE__);
		postFileTransferStatus(newRmMsg);
		closeSocket(sockFd);
		return NULL;
	}
	if(receiveData(sockFd,4,(char*)&fileSize) < 4)
	{
		printf("[%s : %d ] Unable to receive \n",__FUNCTION__,__LINE__);
		postFileTransferStatus(newRmMsg);
		closeSocket(sockFd);
		return NULL;
	}
	fileSize = ntohl(fileSize);
	if(fileSize == 0 || fileSize > (20*1024*1024))
	{
		printf("[%s : %d ] Error File Size %u \n",__FUNCTION__,__LINE__,fileSize);
		postFileTransferStatus(newRmMsg);
		closeSocket(sockFd);
		return NULL;
	}
	fd = open(req->fileName, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if(fd <= 0)
	{
		printf("[%s : %d ] Unable to create file %s\n",__FUNCTION__,__LINE__,req->fileName);
		postFileTransferStatus(newRmMsg);
		closeSocket(sockFd);
		return NULL;
	}
	u32 currentRead = 0;
	while(totalRead < fileSize){
		currentRead = ((fileSize - totalRead) >= 4096)?4096:(fileSize - totalRead);
		currentRead = receiveData(sockFd,currentRead,buffer);
		if(currentRead == 0)
		{
			printf("[%s : %d ] Read return 0 bytes Total Read %u \n",__FUNCTION__,__LINE__,totalRead);
			close(fd);
			closeSocket(sockFd);
			postFileTransferStatus(newRmMsg);
			break;
		}
		write(fd,buffer,currentRead);
		totalRead += currentRead;
	}
	close(fd);
	closeSocket(sockFd);
	newRmMsg->msgId = rm_dw_complete_m;
	postFileTransferStatus(newRmMsg);
}

static void postFileTransferStatus(rmMsg_t *newRmMsg)
{
	// Get Resource Manager Instance and post message
	RManager *rm = getRManagerInstance();
	
	if(rm->postMsgToRm(rm, newRmMsg) != G_OK)
	{
		printf("ERROR [%s : %d ] not able to post message to resource manager\n",__FUNCTION__,__LINE__);
		free(newRmMsg);
	}
}
