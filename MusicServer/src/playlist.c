#include "playlist.h"

#define MAX_PLAYLIST	100

static  PLAYLIST *plInstance = NULL;
static void initPlayList(PLAYLIST *pl);


RESULT downlaodingCheck(void *);
RESULT playingCheck(void *);
static RESULT playListSchedular(PLAYLIST *pl, PL_STATE state, PlayListData *respClnt);
static RESULT addToPlayList(PLAYLIST *pl,clntid_t clntId,u32 tok,void *clntData);
static RESULT deleteDownloading(PLAYLIST *pl,clntid_t clientId);
static RESULT deletePlaying(PLAYLIST *pl,clntid_t clientId);
static RESULT deleteFromPlayList(PLAYLIST *pl,clntid_t clientId);

PLAYLIST *getPlayListInstance()
{
	if(plInstance == NULL)
	{
		plInstance = (PLAYLIST *)malloc(sizeof(PLAYLIST));
		// Constructor, Initialize Play List Instance
		initPlayList(plInstance);
	}
	return plInstance;
}

static void initPlayList(PLAYLIST *pl)
{
	int i = 0;
	for(i = 0; i<MAX_CLIENT; i++)
	{
		pl->initList(&pl->pList[i]);
	}
	pthread_mutex_init(&pl->playListLock,NULL);
	pl->playListSize =  0;
	pl->currentDownloadingClient = 0;
	pl->currentPlayingClient     = 0;

	pl->addToPlayList			= addToPlayList;
	pl->deleteDownloading		= deleteDownloading;
	pl->deleteFromPlayList  	= deleteFromPlayList;
	pl->deletePlaying       	= deletePlaying;
	pl->getOneForDownloading	= getOneForDownloading;
	pl->getOneForPlaying        = getOneForPlaying;
}


static RESULT addToPlayList(PLAYLIST *pl,clntid_t clntId,u32 tok,void *clntData, u32 size)
{
	RESULT res = G_ERR;
	if(pl->playListSize >= MAX_PLAYLIST)
	{
		printf("INFO : [%s : %d] : Playlist is full\n",__FUNCTION__,__LINE__);
		return res;
	}
	PlayListData *plD = NULL;
	plD = (PlayListData *)malloc(sizeof(PlayListData));
	if(plD == NULL)
	{
		printf("INFO [%s : %d] Malloc failed\n");
		return G_ERR;
	}
	plD->id			= clntId;
	plD->token		= tok;
	plD->plStatus   = PL_WAITING;
	plD->data       = clntData;
	plD->size       = size;
	pthread_mutex_lock(&pl->playListLock);
	res  = addLast(&pl->pList[clntId],(void *)plD);
	pl->playListSize++;
	pthread_mutex_unlock(&pl->playListLock);
	return res;
	
}

static RESULT deleteDownloading(PLAYLIST *pl)
{
	RESULT res = G_ERR;
	pthread_mutex_lock(&pl->playListLock);
	res = deleteOneFromListHaving(&pl->pList[pl->currentDownloadingClient], downlaodingCheck);
	pl->playListSize = (res == G_OK)?pl->playListSize--:pl->playListSize;
	pthread_mutex_unlock(&pl->playListLock);
	return res;
}

static RESULT deletePlaying(PLAYLIST *pl)
{
	RESULT res = G_ERR;
	pthread_mutex_lock(&pl->playListLock);
	res = deleteOneFromListHaving(&pl->pList[pl->currentPlayingClient], playingCheck);
	pl->playListSize = (res == G_OK)?pl->playListSize--:pl->playListSize;
	pthread_mutex_unlock(&pl->playListLock);
	return res;
}

static RESULT deleteFromPlayList(PLAYLIST *pl,clntid_t clientId)
{
	pthread_mutex_lock(&pl->playListLock);
	pl->playListSize -= pl->pList[clientId].totalNode;
	deleteAllFromList(&pl->pList[clientId]);
	pthread_mutex_unlock(&pl->playListLock);
	return G_OK;
}

static RESULT getOneForDownloading(PLAYLIST *pl,clntid_t *clientId, u32 *pltoken)
{
	RESULT res = G_FAIL;
	PlayListData * respClnt = NULL;
	pthread_mutex_lock(&pl->playListLock);
	res = playListSchedular(pl,PL_DOWNLOADING,respClnt);
	if(res == G_OK)
	{
		*pltoken  = respClnt->token;
		*clientId = respClnt->id;
		pl->currentDownloadingClient = respClnt->id;
		respClnt->plStatus = PL_DOWNLOADING;
	}
	pthread_mutex_unlock(&pl->playListLock);
	return res;
}

static RESULT getOneForPlaying(PLAYLIST *pl,clntid_t *clientId, u32 *pltoken)
{
	RESULT res = G_FAIL;
	PlayListData * respClnt = NULL;
	pthread_mutex_lock(&pl->playListLock);
	res = playListSchedular(pl,PL_PLAYING,respClnt);
	if(res == G_OK)
	{
		*pltoken  = respClnt->token;
		*clientId = respClnt->id;
		pl->currentPlayingClient = respClnt->id;
		respClnt->plStatus = PL_PLAYING;
	}
	pthread_mutex_unlock(&pl->playListLock);
	return res;
}

RESULT downlaodingCheck(void *data)
{
	PlayListData *plD = NULL;
	plD = (PlayListData *)data;
	if(plD->plStatus == PL_DOWNLOADING)
	{
		free(plD->data);
		return G_OK;
	}

	return G_FAIL;
}

RESULT playingCheck(void *data)
{
	PlayListData *plD = NULL;
	plD = (PlayListData *)data;
	if(plD->plStatus == PL_PLAYING)
	{
		free(plD->data);
		return G_OK;
	}

	return G_FAIL;
}

static RESULT playListSchedular(PLAYLIST *pl, PL_STATE state, PlayListData *respClnt)
{
	int i = 0;
	PlayListData *clntData = NULL;
	int currentClient = pl->currentDownloadingClient;
	int found = 0;
	
	if(state == PL_DOWNLOADING)
		state = PL_WAITING;
	else if(state == PL_PLAYING)
		state = PL_READY;
	else
		return G_ERR;
	
	for(i=0; i<MAX_CLIENT; i++)
	{
		currentClient++;
		currentClient = currentClient%MAX_CLIENT;

		if(pl->pList[currentClient]->first == NULL)
			continue;

		clntData = (PlayListData *)(pl->pList[currentClient]->first->data);
		if(clntData->plStatus == state)
		{
			found = 1;
			break;
		}
	}

	if(found)
	{
		respClnt = clntData;
		return G_OK;
	}

	// We dont have any other client to schedule
	// Schedule the current client
	if(pl->pList[pl->currentClient]->first != NULL && pl->pList[pl->currentClient]->first->next != NULL)
	{
		clntData = (PlayListData *)(pl->pList[currentClient]->first->next->data)
		if(clntData->plStatus == state)
		{
			respClnt = clntData;
			return G_OK;
		} else {
			respClnt = NULL;
			return G_FAIL;
		}
	}
	respClnt = NULL;
	return G_FAIL;
}