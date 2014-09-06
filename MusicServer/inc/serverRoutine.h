#ifndef __SERVER_ROUTINE_H__
#define __SERVER_ROUTINE_H__
#include "packet.h"

void *handleClient(void *args);
void *sendNACKandClose(void *arg);
void *sendACKandClose(void *arg);
void *getMP3FileFromClient(void *msg);

typedef enum
{
	MP3_INIT,
	MP3_DOWNLOADING,
	MP3_FILE_ERROR,
	MP3_FILE_READY,
	MP3_FILE_PLAYING
}MP3_FILE_STATE;
typedef struct
{
	u32 requestId;
	char clntId;
	u32 ipaddress;
	char *fileName;
	MP3_FILE_STATE fileState;
	
}MP3_FILE_REQ;



#endif //__SERVER_ROUTINE_H__
