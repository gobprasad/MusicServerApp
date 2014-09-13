#include "playlist.h"
#include "loggingFrameWork.h"

#define MAX_PLAYLIST	100

static  PLAYLIST *plInstance = NULL;
static void initPlayList(PLAYLIST *pl);


RESULT downlaodingCheck(void *);
RESULT playingCheck(void *);
static RESULT playListSchedular(PLAYLIST *pl, PL_STATE state, PlayListData **respClnt);
static RESULT addToPlayList(PLAYLIST *pl,clntid_t clntId,u32 tok,void *clntData,u32 size);
static RESULT deleteDownloading(PLAYLIST *pl);
static RESULT deletePlaying(PLAYLIST *pl);
static RESULT deleteFromPlayList(PLAYLIST *pl,clntid_t clientId);
static RESULT setPlayListStatus(PLAYLIST *pl,clntid_t clientId,PL_STATE state);
static RESULT getOneForDownloading(PLAYLIST *pl,clntid_t *clientId, u32 *pltoken);
static RESULT getOneForPlaying(PLAYLIST *pl,clntid_t *clientId, u32 *pltoken);

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
		initList(&pl->pList[i]);
	}
	pthread_mutex_init(&pl->playListLock,NULL);
	pl->playListSize =  0;
	pl->currentDownloadingClient = -1;
	pl->currentPlayingClient     = -1;

	pl->addToPlayList			= addToPlayList;
	pl->deleteDownloading		= deleteDownloading;
	pl->deleteFromPlayList  	= deleteFromPlayList;
	pl->deletePlaying       	= deletePlaying;
	pl->getOneForDownloading	= getOneForDownloading;
	pl->getOneForPlaying        = getOneForPlaying;
	pl->setPlayListStatus       = setPlayListStatus;
}


static RESULT addToPlayList(PLAYLIST *pl,clntid_t clntId,u32 tok,void *clntData, u32 size)
{
	RESULT res = G_ERR;
	if(pl->playListSize >= MAX_PLAYLIST)
	{
		LOG_WARN("Playlist is full\n");
		return res;
	}
	PlayListData *plD = NULL;
	plD = (PlayListData *)malloc(sizeof(PlayListData));
	if(plD == NULL)
	{
		LOG_ERROR("Malloc failed\n");
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
	res = playListSchedular(pl,PL_DOWNLOADING,&respClnt);
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
	res = playListSchedular(pl,PL_PLAYING,&respClnt);
	if(res == G_OK)
	{
		*pltoken  = respClnt->token;
		*clientId = respClnt->id;
		pl->currentPlayingClient = respClnt->id;
		respClnt->plStatus = PL_PLAYING;
		pl->currentPlayingClient = respClnt->id;
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

static RESULT playListSchedular(PLAYLIST *pl, PL_STATE state, PlayListData **respClnt)
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
		//it will run from 0-9
		currentClient = currentClient%(MAX_CLIENT-1);

		if(pl->pList[currentClient].first == NULL)
			continue;

		clntData = (PlayListData *)(pl->pList[currentClient].first->data);
		if(clntData->plStatus == state)
		{
			found = 1;
			break;
		}
	}

	if(found)
	{
		*respClnt = clntData;
		return G_OK;
	}
	if(pl->currentDownloadingClient < 0)
	{
		return G_FAIL;
	}

	// We dont have any other client to schedule
	// Schedule the current client
	if(pl->pList[pl->currentDownloadingClient].first != NULL && pl->pList[pl->currentDownloadingClient].first->next != NULL)
	{
		clntData = (PlayListData *)(pl->pList[pl->currentDownloadingClient].first->next->data);
		if(clntData->plStatus == state)
		{
			*respClnt = clntData;
			return G_OK;
		} else {
			respClnt = NULL;
			return G_FAIL;
		}
	}
	respClnt = NULL;
	return G_FAIL;
}

static RESULT setPlayListStatus(PLAYLIST *pl,clntid_t clientId,PL_STATE state)
{
	RESULT res = G_FAIL;
	LIST_NODE *temp;
	PL_STATE currState;
	PlayListData *currData; 
	if(state == PL_PLAYING)
	{
		currState = PL_READY;
	} else if(state == PL_READY)
	{
		currState = PL_DOWNLOADING;
	} else {

		LOG_ERROR("FATAL ERROR: Unknown playing state");
		return res;
	}
	pthread_mutex_lock(&pl->playListLock);
	temp = pl->pList[clientId].first;
	if(temp == NULL)
	{
		LOG_ERROR("FATAL ERROR: Request not found");
		pthread_mutex_unlock(&pl->playListLock);
		return res;
	}
	while(temp != NULL)
	{
		currData = (PlayListData *)temp->data;
		if(currData->plStatus == currState)
		{
			currData->plStatus = state;
			res = G_OK;
			break;
		}
		temp = temp->next;
	}
	pthread_mutex_unlock(&pl->playListLock);
	return res;
}

