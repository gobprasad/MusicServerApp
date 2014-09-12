#include "clientdb.h"
#include "results.h"
#include "loggingFrameWork.h"

static CLIENT_DB *clientDBinstance = NULL;
static void initClientDb(CLIENT_DB *cdb);


// Public Member Function
static RESULT registerClientFunc(CLIENT_DB *cdb, CLIENT_INFO *clntInfo);
static RESULT deregisterClientFunc(CLIENT_DB *cdb, clntid_t id);

static RESULT addToClientQueueFunc(CLIENT_DB *cdb,clntid_t id,u32 token,void *msg,u32 size);

static RESULT getClientRequestForDownloadingFunc(CLIENT_DB *cdb,MP3_FILE_REQ *req);
static RESULT setDownloadingStatusFunc(CLIENT_DB *cdb, MP3_FILE_REQ *resp);


static RESULT getClientRequestForPlayingFunc(CLIENT_DB *cdb, char **playFile);
static RESULT setPlayingStatusFunc(CLIENT_DB *cdb);

static RESULT getClientIpAddressFunc(CLIENT_DB *cdb,clntid_t id, u32 *address);


//Private Member Function
static RESULT deleteAllDataOfClient(CLIENT_DB *cdb,clntid_t id);
static RESULT setClientStateActive(CLIENT_DB *cdb, clntid_t id);
//static RESULT sendUpdateToClient(CLIENT_DB *cdb, clntid_t id);


/**
 * Get Client DB instance Singleton
 */
CLIENT_DB *getClientDbInstance()
{
	if(clientDBinstance == NULL)
	{
		clientDBinstance = (CLIENT_DB *)malloc(sizeof(CLIENT_DB));
		// Constructor, Initialize client database
		initClientDb(clientDBinstance);
	}
	return clientDBinstance;
}

/**
 *  Constructor of ClientDB
 */
static void initClientDb(CLIENT_DB *cdb)
{
	int i = 0;
	for(i=0; i<MAX_CLIENT; i++)
	{
		cdb->clientId[i] = i;
		memset(cdb->clientName[i],0,MAX_CLIENT_NAME+1);
		cdb->clientIP[i] = 0;
		cdb->clientState[i] = clnt_unregister_state;
	}
	cdb->scheduleIndex = 0;
	
	cdb->registerClient			= registerClientFunc;
	cdb->deregisterClient		= deregisterClientFunc;
	cdb->addToQueue				= addToClientQueueFunc;
	cdb->getClientIpAddress     = getClientIpAddressFunc;
	cdb->getClientRequestForDownloading = getClientRequestForDownloadingFunc;
	cdb->setDownloadingStatus   = setDownloadingStatusFunc;
	cdb->getClientRequestForPlaying = getClientRequestForPlayingFunc;
	cdb->setPlayingStatus       = setPlayingStatusFunc;
	
	
	//Get PlayList Class Instance
	cdb->playList = getPlayListInstance();
	cdb->schData[0].status = PL_NONE;
	memset(cdb->schData[0].fileName,0,DOWNLOAD_FILE_NAME_SIZE);
	cdb->scheduleIndex     = 0;
}


/**
 * Register Client
 * Add to database, create client data and add unique ID
 */

static RESULT registerClientFunc(CLIENT_DB *cdb, CLIENT_INFO *clntInfo)
{
	if(!cdb || !clntInfo)
	{
		return G_FAIL;
	}
	int i =0;
	// Check client is already registered
	// if so delete old entry and assign new entry
	// It may happen after reboot or after client crash
	for(i=0;i<MAX_CLIENT;i++)
	{
		if(cdb->clientIP[i] == clntInfo->clientIP)
			break;
	}
	
	if(i < MAX_CLIENT)
	{
		LOG_MSG("Client is connecting second time\n");
		// deregister old session
		deregisterClientFunc(cdb, i);
	}
	
	for(i=0;i<MAX_CLIENT;i++)
	{
		if(cdb->clientState[i] == clnt_unregister_state)
			break;
	}
	if(i >= MAX_CLIENT)
	{
		return G_FAIL;
	}
	
	cdb->clientId[i] = i;
	strncpy(cdb->clientName[i],clntInfo->clientName,MAX_CLIENT_NAME);
	cdb->clientIP[i] = clntInfo->clientIP;
	cdb->clientState[i] = clnt_registered_state;

	// only for testing
	char *msg = (char*)malloc(10);
	addToClientQueueFunc(cdb,i,455,(void*)msg,10);
	return G_OK;
}

/**
 * De Register Client
 * Delete Client Instance
 */
static RESULT deregisterClientFunc(CLIENT_DB *cdb, clntid_t id)
{
	if(!cdb )
	{
		return G_FAIL;
	}
	if(id >= MAX_CLIENT)
	{
		LOG_ERROR("Invalid client Id, clntId =%d",id);
		return G_FAIL;
	}
	if(cdb->clientState[id] == clnt_unregister_state)
	{
		LOG_ERROR("Client is already de registered, clntId =%d",id);
		return G_FAIL;
	}
	deleteAllDataOfClient(cdb,id);
	memset(cdb->clientName[id],0,MAX_CLIENT_NAME+1);
	cdb->clientIP[id] = 0;
	cdb->clientState[id] = clnt_unregister_state;
	
	return G_OK;
}

/**
 * Add request to client Queue
 */
static RESULT addToClientQueueFunc(CLIENT_DB *cdb,clntid_t id,u32 token,void *msg,u32 size)
{
	RESULT res = G_ERR;
	if(!msg || !cdb || (id >= MAX_CLIENT) || (cdb->clientState[id] == clnt_unregister_state) )
	{
		LOG_ERROR("Error in add to client queue\n");
		return G_FAIL;
	}

	//create a copy of user data
	void *data = malloc(size);
	memcpy(data,msg,size);
	res = cdb->playList->addToPlayList(cdb->playList,id,token,data,size);
	if(res != G_OK)
		free(data);

	return res;
}

static RESULT getClientIpAddressFunc(CLIENT_DB *cdb,clntid_t id, u32 *address)
{
	if(!cdb )
	{
		return G_FAIL;
	}
	if(id >= MAX_CLIENT)
	{
		LOG_ERROR("Invalid client Id, clntId =%d, client IP %u",id);
		return G_FAIL;
	}
	if(cdb->clientState[id] == clnt_unregister_state)
	{
		LOG_ERROR("Client is de-registered, clntId =%d",id);
		return G_FAIL;
	}
	if(cdb->clientIP[id] == 0)
	{
		return G_FAIL;
	}
	*address = cdb->clientIP[id];
	return G_OK;
}

static RESULT setClientStateActive(CLIENT_DB *cdb, clntid_t id)
{
	if(!cdb )
	{
		return G_FAIL;
	}
	if(id >= MAX_CLIENT)
	{
		//printf("Invalid client Id, clntId =%d\n",clientInfo->clientId);
		return G_FAIL;
	}
	if(cdb->clientState[id] == clnt_unregister_state)
	{
		//printf("Client is already de registered, clntId =%d\n",clientInfo->clientId);
		return G_FAIL;
	}
	cdb->clientState[id] = clnt_active_state;
	return G_OK;
}

static RESULT getClientRequestForDownloadingFunc(CLIENT_DB *cdb,MP3_FILE_REQ *req)
{
	RESULT res = G_FAIL;
	if(!req)
	{
		LOG_ERROR("Request Message is null");
		return res;
	}
	if(cdb->schData[cdb->scheduleIndex].status != PL_NONE)
	{
		//Current schedule position is already occupied
		LOG_WARN("No postion empty for scheduling");
		return G_FAIL;
	}
	res = cdb->playList->getOneForDownloading(cdb->playList,&req->clntId,&req->requestId);
	if(res != G_OK)
	{
		LOG_WARN("NO Client to schedule");
		return res;
	}
	sprintf(cdb->schData[cdb->scheduleIndex].fileName,"%s_%d_%u.mp3",PLAY_FILE_NAME,req->clntId,req->requestId);
	req->fileName = cdb->schData[cdb->scheduleIndex].fileName;
	req->ipaddress = cdb->clientIP[req->clntId];
	req->fileState = MP3_DOWNLOADING;
	
	cdb->schData[cdb->scheduleIndex].clientId = req->clntId;
	cdb->schData[cdb->scheduleIndex].reqToken = req->requestId;
	cdb->schData[cdb->scheduleIndex].status = PL_DOWNLOADING;

	// Get the next scheduling position
	cdb->scheduleIndex = (cdb->scheduleIndex+1)%2;

	return res;
	
}

static RESULT setDownloadingStatusFunc(CLIENT_DB *cdb, MP3_FILE_REQ *resp)
{
	RESULT res = G_FAIL;
	int i = 0;
	if(!resp)
	{
		LOG_ERROR("Response Message is null\n");
		return res;
	}
	for(i=0; i<2; i++)
	{
		if(cdb->schData[i].clientId == resp->clntId &&
			cdb->schData[i].reqToken == resp->requestId &&
			cdb->schData[i].status == PL_DOWNLOADING )
			break;
	}
	if(i >= 2)
	{
		LOG_ERROR("FATAL ERROR, MissMatch between downloading request and response");
		return res;
	}
	
	switch(resp->fileState)
	{
		case MP3_FILE_READY:
			cdb->schData[i].status = PL_READY;
			cdb->playList->setPlayListStatus(cdb->playList,resp->clntId,PL_READY);
			LOG_MSG("File %s downloaded successfully",resp->fileName);
			break;
		case MP3_FILE_ERROR:
			cdb->schData[i].status = PL_NONE;
			cdb->playList->deleteDownloading(cdb->playList);
			LOG_MSG("File %s download Error",resp->fileName);
			unlink(resp->fileName);
			break;
		default:
			LOG_ERROR("File Download default case");
			break;
	}
	//delete the response message
	free(resp);
	return G_OK;

}

static RESULT getClientRequestForPlayingFunc(CLIENT_DB *cdb, char **playFile)
{
	RESULT res = G_FAIL;
	int i = 0;
	for(i=0; i<2; i++)
	{
		if(cdb->schData[i].status == PL_READY)
			break;
	}
	if(i >= 2)
	{
		LOG_WARN("No File ready for Playing");
		return res;
	}

	// File ready for playing
	*playFile = cdb->schData[i].fileName;
	cdb->schData[i].status  = PL_PLAYING;
	res = cdb->playList->setPlayListStatus(cdb->playList,cdb->schData[i].clientId,PL_PLAYING);
	if(res != G_OK)
	{
		LOG_ERROR("Unable to set Playing status in playList");
	}
	setClientStateActive(cdb,cdb->schData[i].clientId);
	return G_OK;
}

static RESULT setPlayingStatusFunc(CLIENT_DB *cdb)
{
	RESULT res = G_FAIL;
	int i = 0;
	for(i=0; i<2; i++)
	{
		if(cdb->schData[i].status == PL_PLAYING)
			break;
	}
	if(i >= 2)
	{
		LOG_ERROR("FATAL ERROR, MissMatch between Playing response and schedular array content");
		return res;
	}
	cdb->schData[i].status = PL_NONE;
	cdb->playList->deletePlaying(cdb->playList);
	unlink(cdb->schData[i].fileName);
	LOG_MSG("Successfully Played %s",cdb->schData[i].fileName);
	return G_OK;

}


static RESULT deleteAllDataOfClient(CLIENT_DB *cdb,clntid_t id)
{
	cdb->playList->deleteFromPlayList(cdb->playList,id);
	return G_OK;
}

