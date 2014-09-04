#include "packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "results.h"
#ifdef __WINDOWS__
    #include<QtEndian>
#else
	#include <arpa/inet.h>
#endif

#define HEADER_SIZE		12
#define DECODED_HEADER_SIZE	7

char logBuffer[256] = {0};

static u32 token = 0;

static RESULT encodeHeader(uchar *buff, header_t *head);
static RESULT decodeHeader(uchar *buf, header_t *head);


u32 encodePacket(clntData_t *msg, uchar **retBuf)
{
	u32 totalSize = 0;
    uchar *buf;
	totalSize = (HEADER_SIZE + msg->payloadSize);
    msg->header.totalSize = totalSize;
    buf = (uchar *)malloc(totalSize);
	if(buf == NULL){
        sprintf(logBuffer,"[%s : %d] : Malloc failed\n",__FUNCTION__,__LINE__);
		return 0;
	}
	if(encodeHeader(buf, &msg->header) != G_OK)
	{
		free(buf);
		return 0;
	}
	if(msg->payloadSize != 0)
	{
		memcpy(buf+HEADER_SIZE, msg->payLoad, msg->payloadSize);
	}
	*retBuf = buf;
	return totalSize;
}

RESULT decodePacket(clntData_t *msg, uchar *buf, u32 size)
{
    int tempSize = 0;
	if(!buf || !msg)
	{
		printf("[%s : %d] : Error in Packet decoding\n",__FUNCTION__,__LINE__);
		return G_FAIL;
	}
	if(decodeHeader(buf, &msg->header) != G_OK)
	{
		free(buf);
		return G_FAIL;
	}
	msg->payloadSize = 0;
    tempSize = size - DECODED_HEADER_SIZE; /*Not a magic number, size after header */
	if( tempSize > 0 )
	{
        msg->payLoad = (uchar *)malloc(tempSize+1);
		if(msg->payLoad == NULL)
		{
			printf("[%s : %d] : Error in Packet decoding\n",__FUNCTION__,__LINE__);
			free(buf);
			return G_FAIL;
		}
		memcpy(msg->payLoad,buf+DECODED_HEADER_SIZE,tempSize);
		memset(msg->payLoad+tempSize+1,0,1); //NULL Terminated string
        msg->payloadSize = tempSize+1;
	}
	return G_OK;
}

/**
 * Header for each packet
 * 1st Signature
 * 2nd totalSize
 * 3rd msgId
 * 4th clntId
 * 5th token
 * 6th isLast
 */
static RESULT encodeHeader(uchar *buff, header_t *head)
{
	if(!buff || !head)
	{
        sprintf(logBuffer,"[%s : %d] : Error in header encoding\n",__FUNCTION__,__LINE__);
		return G_FAIL;
	}
	memcpy(buff, &head->signature, 1);

#ifdef __WINDOWS__
    qToBigEndian<u32>(head->totalSize,buff+1);
#else
    head->totalSize = htonl(head->totalSize);
    memcpy(buff+1, &head->totalSize, 4);
#endif

	memcpy(buff+5, &head->msgId, 1);
	memcpy(buff+6, &head->clntId, 1);
#ifdef __WINDOWS__
    qToBigEndian<u32>(head->token,buff+7);
#else
	head->token = htonl(head->token);
    memcpy(buff+7, &head->token, 4);
#endif
    memcpy(buff+11,&head->isLast,1);
	
	return G_OK;
}

static RESULT decodeHeader(uchar *buf, header_t *head)
{
	if(!buf || !head)
	{
		printf("[%s : %d] : Error in header decoding\n",__FUNCTION__,__LINE__);
		return G_FAIL;
	}
	// NOTE While decoding signature and length will not be present
	// in buffer it will be discarded at socket recv ends itself
	memcpy(&head->msgId,buf,1);
	memcpy(&head->clntId,buf+1,1);
    memcpy(&head->token,buf+2,4);
#ifdef __WINDOWS__
    head->token =  qFromBigEndian(head->token);
#else
	head->token = ntohl(head->token);
#endif
    memcpy(&head->isLast,buf+6,1);
	return G_OK;
}

u32 getNewToken()
{
	token++;
	return token;
}
u32 getCurrentToken()
{
	return token;
}
