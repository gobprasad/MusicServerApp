#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__
#include <pthread.h>
#include "list.h"
#include "results.h"
#include "commonInclude.h"
#include "serverRoutine.h"

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
	u32 playListSize;
	char currentPlayingClient;
	char currentDownloadingClient;
	pthread_mutex_t playListLock;


	RESULT (*addToPlayList)(struct playList *,clntid_t id,u32 token,void *data,u32 size);
	RESULT (*deleteFromPlayList)(struct playList *,clntid_t id);
	RESULT (*getOneForPlaying)(struct playList *,clntid_t *clientId, u32 *pltoken);
	RESULT (*getOneForDownloading)(struct playList *,clntid_t *clientId, u32 *pltoken);
	RESULT (*deletePlaying)(struct playList *);
	// Need to make it token based otherwise problem may happen
	RESULT (*deleteDownloading)(struct playList *);
	RESULT (*setPlayListStatus)(struct playList *,clntid_t clientId,PL_STATE state);

}PLAYLIST;

PLAYLIST *getPlayListInstance();

#endif
