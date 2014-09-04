#ifndef __SERVER_H__
#define __SERVER_H__

#include "packet.h"

typedef struct
{
	int clntSockFd;
	u32 cli_addr;
	clntData_t clntData;
}clntMsg_t;

void *startServerSocket(void *arg);

#endif //__SERVER_H__
