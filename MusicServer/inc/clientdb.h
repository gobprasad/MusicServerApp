#ifndef __CLIENT_DB_H__
#define __CLIENT_DB_H__

#include "commonInclude.h"
#include "queue.h"
#include "packet.h"
#include "playlist.h"


typedef struct
{
	clntid_t clientId;
	char clientName[MAX_CLIENT_NAME];
	unsigned int clientIP;

}CLIENT_INFO;

typedef struct
{
	unsigned int requestId;
	char *reqMsg;
	unsigned int size;
}CLIENT_DATA;

typedef enum
{
	clnt_unregister_state = 0,
	clnt_registered_state,
	clnt_active_state,
	clnt_last_state
}CLIENT_STATE;

typedef struct scheduleData
{
	char clientId;
	u32  clientIP;
	u32  reqToken;
	PL_STATE status;
	char fileName[DOWNLOAD_FILE_NAME_SIZE];
}scheduledData;


typedef struct clientDb
{
	clntid_t clientId[MAX_CLIENT];
	char clientName[MAX_CLIENT][MAX_CLIENT_NAME];
	unsigned int clientIP[MAX_CLIENT];
	CLIENT_STATE clientState[MAX_CLIENT];
	PLAYLIST *playList;
	scheduledData schData[2];
	int scheduleIndex;


	RESULT (*registerClient)(struct clientDb *,CLIENT_INFO *);
	RESULT (*deregisterClient)(struct clientDb *,clntid_t);
	RESULT (*addToQueue)(struct clientDb *,clntid_t id,u32 token,void *msg,u32 size);
	RESULT (*getClientIpAddress)(struct clientDb *,clntid_t,u32 *);
	RESULT (*getClientRequestForDownloading)(struct clientDb *,MP3_FILE_REQ *);
	RESULT (*setDownloadingStatus)(struct clientDb *, MP3_FILE_REQ *);
	RESULT (*getClientRequestForPlaying)(struct clientDb *, char *);
	RESULT (*setPlayingStatus)(struct clientDb *);
	RESULT (*deleteFromQueue)(struct clientDb *,clntid_t, u32);
}CLIENT_DB;

CLIENT_DB *getClientDbInstance();
void * sendAllUpdateToClient(void *data);
void *sendOneUpdateToAllClient(void *msg);


#endif // __CLIENT_DB_H__
