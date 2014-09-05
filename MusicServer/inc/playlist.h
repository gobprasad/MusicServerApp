#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__
#include <pthread.h>
#include "List.h"
#include "results.h"

typedef enum
{
	PL_NONE,
	PL_WAITING,
	PL_DOWNLOADING,
	PL_READY,
	PL_PLAYING,
	PL_LAST
}PL_STATE;

typedef struct playListD
{
	u32 token;
	clntid_t id;
	PL_STATE plStatus;
	void *data;
	u32 size;
}PlayListData;

typedef struct playList
{
	LIST pList[MAX_CLIENT];
	int maxList;
	int currentClient;
	pthread_mutex_t playListLock;


	RESULT (*addToPlayList)(struct playList *,clntid_t id,u32 clientMask,void *data);
	RESULT (*deleteFromPlayList)(struct playList *,clntid_t id,u32 clientMask,void *data);
	RESULT (*getOneForPlaying)(struct playList *);
	RESULT (*getOneForDownloading)(struct playList *);
	RESULT (*deletePlaying)(struct playList *);
	RESULT (*deleteDownloading)(struct playList *);

}PLAYLIST;

PLAYLIST *getPlayListInstance();

#endif
