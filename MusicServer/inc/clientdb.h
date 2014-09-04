#ifndef __CLIENT_DB_H__
#define __CLIENT_DB_H__


#include "queue.h"
#include "packet.h"
#include "playlist.h"

#define MAX_CLIENT		10
#define MAX_CLIENT_NAME		128
#define MAX_SONG_NAME		128
#define MAX_ALBUM_NAME		64

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

typedef struct clientDb
{
	clntid_t clientId[MAX_CLIENT];
	char clientName[MAX_CLIENT][MAX_CLIENT_NAME];
	unsigned int clientIP[MAX_CLIENT];
	CLIENT_STATE clientState[MAX_CLIENT];
	PLAYLIST *playList;


	RESULT (*registerClient)(struct clientDb *,CLIENT_INFO *);
	RESULT (*deregisterClient)(struct clientDb *,clntid_t);
	RESULT (*addToQueue)(struct clientDb *,clntid_t,CLIENT_DATA *);
	RESULT (*getOneFromQueue)(struct clientDb *,clntid_t, void *);
	RESULT (*deleteOneFromQueue)(struct clientDb *,clntid_t);
	RESULT (*getClientIpAddress)(struct clientDb *,clntid_t,u32 *);
	RESULT (*getClientFirstToken)(struct clientDb *,clntid_t,u32 *);
	RESULT (*setClientStateActive)(struct clientDb *,clntid_t);
	RESULT (*getNextScheduledClient)(struct clientDb *,u32 *);
}CLIENT_DB;

CLIENT_DB *getClientDbInstance();
#endif // __CLIENT_DB_H__
