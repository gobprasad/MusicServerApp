#ifndef __CLIENT_DB_H__
#define __CLIENT_DB_H__


#include "queue.h"
#include "packet.h"
#include "playlist.h"

#define MAX_CLIENT				10
#define MAX_CLIENT_NAME			128
#define DOWNLOAD_FILE_NAME_SIZE	64
#define PLAY_FILE_NAME			"/tmp/"

typedef char	clntid_t;

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
	RESULT (*addToQueue)(struct clientDb *,clntid_t,CLIENT_DATA *);
	RESULT (*getClientIpAddress)(struct clientDb *,clntid_t,u32 *);
	RESULT (*getClientRequestForDownloading)(struct clientDb *,MP3_FILE_REQ *);
	RESULT (*setDownloadingStatus)(struct clientDb *, MP3_FILE_REQ *);
	RESULT (*getClientRequestForPlaying)(struct clientDb *, char *);
	RESULT (*setPlayingStatus)(struct clientDb *);

}CLIENT_DB;

CLIENT_DB *getClientDbInstance();
#endif // __CLIENT_DB_H__
