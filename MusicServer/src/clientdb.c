#include "clientdb.h"
#include "results.h"
#include "loggingFrameWork.h"
#include <netinet/in.h>
#include "musicPlayer.h"


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

static RESULT sendOneUpdate(clntData_t *data, int sockFd);
static RESULT connectToClient(char clientId, int *sockFd);
static void sendDeleteOneToClient(char clientId, u32 m_token);
static void checkAndDeleteCurrentList(char id);

static RESULT deleteFromQueueFunc(CLIENT_DB *cdb,clntid_t id,u32 token);




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
	cdb->deleteFromQueue        = deleteFromQueueFunc;
	
	
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
	clntInfo->clientId = i;
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
	checkAndDeleteCurrentList(id);
	deleteAllDataOfClient(cdb,id);
	memset(cdb->clientName[id],0,MAX_CLIENT_NAME+1);
	cdb->clientIP[id] = 0;
	cdb->clientState[id] = clnt_unregister_state;
	clntData_t *msg = createClientMsg(id,0,client_deleteAllDataId_m);
	addJobToQueue(sendOneUpdateToAllClient, (void *)msg);
	
	return G_OK;
}

/**
 * Add request to client Queue
 */
static RESULT addToClientQueueFunc(CLIENT_DB *cdb,clntid_t id,u32 token,void *msg,u32 size)
{
	RESULT res = G_ERR;
	void *data = NULL;
	if(!msg || !cdb || (id >= MAX_CLIENT) || (cdb->clientState[id] == clnt_unregister_state) )
	{
		LOG_ERROR("Error in add to client queue clntId = %d, clntState %d\n", id, cdb->clientState[id]);
		return G_FAIL;
	}

	//create a copy of user data
	data = malloc(size);
	if(data == NULL)
	{
		LOG_ERROR("Malloc failed");
		return res;
	}
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
			sendDeleteOneToClient(cdb->schData[i].clientId, cdb->schData[i].reqToken);
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
	clntData_t *msg = createClientMsg(cdb->schData[i].clientId,cdb->schData[i].reqToken,client_currentPlaying_m);
	addJobToQueue(sendOneUpdateToAllClient, (void *)msg);
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
	if(cdb->playList->deletePlaying(cdb->playList) != G_OK)
	{
		LOG_ERROR("Error in deleting played record");
	}
	unlink(cdb->schData[i].fileName);
	if(cdb->clientState[cdb->schData[i].clientId] == clnt_active_state)
	{
		cdb->clientState[cdb->schData[i].clientId] = clnt_registered_state;
		LOG_MSG("Successfully Played %s",cdb->schData[i].fileName);
		sendDeleteOneToClient(cdb->schData[i].clientId, cdb->schData[i].reqToken);
	} else {

		LOG_ERROR("Client is not Active");
	}
	return G_OK;

}


static RESULT deleteAllDataOfClient(CLIENT_DB *cdb,clntid_t id)
{
	cdb->playList->deleteFromPlayList(cdb->playList,id);
	return G_OK;
}

static RESULT deleteFromQueueFunc(CLIENT_DB *cdb,clntid_t id,u32 token)
{
	RESULT ret = G_FAIL;
	MPlayer *mPlayer = getMPlayerInstance();
	if(!cdb || id >= MAX_CLIENT)
	{
		LOG_ERROR("FATAL ERROR cdb null or id not matching");
		return ret;
	}
	if(cdb->clientState[id] == clnt_unregister_state)
	{
		LOG_ERROR("delete request from unregistered client id = %d", id);
		ret = G_FAIL;
	}
	else if(cdb->schData[0].clientId == id && cdb->schData[0].reqToken == token)
	{
		if(cdb->schData[0].status == PL_PLAYING)
		{
			mPlayer->stop = 1;
			LOG_MSG("Deleted Playing Data data");
			ret = G_OK;
		}
		else if(cdb->schData[0].status == PL_READY)
		{
			cdb->schData[0].status   = PL_NONE;
			unlink(cdb->schData[0].fileName);
			ret = cdb->playList->deleteFromClientHavingToken(cdb->playList,id,token);
			LOG_MSG("Deleted downloaded data, ret = %d", ret);
		}
		else
		{
			LOG_ERROR("Unable to delete cdb schData");
			ret = G_FAIL;
		}
	}
	else if(cdb->schData[1].clientId == id && cdb->schData[1].reqToken == token)
	{
		if(cdb->schData[1].status == PL_PLAYING)
		{
			mPlayer->stop = 1;
			LOG_MSG("Deleted Playing Data data");
			ret = G_OK;
		}
		else if(cdb->schData[1].status == PL_READY)
		{
			cdb->schData[1].status   = PL_NONE;
			unlink(cdb->schData[1].fileName);
			ret = cdb->playList->deleteFromClientHavingToken(cdb->playList,id,token);
			LOG_MSG("Deleted downloaded data, ret = %d", ret);
		}
		else
		{
			LOG_ERROR("Unable to delete cdb schData");
			ret = G_FAIL;
		}
	}
	else
	{
		ret = cdb->playList->deleteFromClientHavingToken(cdb->playList,id,token);
		LOG_MSG("Directly deleted from queue, ret = %d", ret);
	}
	return ret;
}


void * sendAllUpdateToClient(void *data)
{
	int i = 0, j = 0;
	char isClientConnected = 0;
	int sockFd = 0;
	RESULT sendResult = G_FAIL;
	char clientId = *((char*)data);
	clntData_t clntData;
	CLIENT_DB *cdb = getClientDbInstance();
	if(!cdb)
	{
		LOG_ERROR("Error in client db instance");
		return;
	}
	if(clientId >= MAX_CLIENT)
	{
		LOG_ERROR("Invalid client Id, clntId =%d",clientId);
		return NULL;
	}
	if(cdb->clientState[clientId] == clnt_unregister_state)
	{
		LOG_ERROR("Client is already de registered, clntId =%d",clientId);
		return NULL;
	}
	PLAYLIST *pl = cdb->playList;
	// LOCK the playlist
	pthread_mutex_lock(&pl->playListLock);
	if(pl->playListSize <= 0)
	{
		LOG_MSG("Playlist is empty, nothing to update");
		pthread_mutex_unlock(&pl->playListLock);
		return NULL;
	}
	LIST_NODE *tempNode = NULL;
	for(i=0; i<MAX_CLIENT; i++)
	{
		// skip current client and unregistered client
		if((cdb->clientState[clientId] == clnt_unregister_state) || (i == clientId))
			continue;

		//skip client having not data
		if(pl->pList[i].totalNode == 0)
		{
			continue;
		}
		tempNode = pl->pList[i].first;
		for(j=0; j<pl->pList[i].totalNode; j++)
		{
			// create client message;
			clntData.header.clntId = clientId;
			clntData.header.msgId  = client_update_all;
			clntData.header.isLast = 0;
			clntData.header.token  = getNewToken();
			clntData.header.totalSize = 0;
			clntData.payLoad  = ((PlayListData*)(tempNode->data))->data;
			clntData.payloadSize = ((PlayListData*)(tempNode->data))->size;
			
			//if client is not connected connect with it
			if(!isClientConnected)
			{
				if(connectToClient(clientId, &sockFd) != G_OK)
				{
					LOG_ERROR("Unable to connect with client for sending update");
					break;
				}
				isClientConnected = 1;
			}
			if((sendResult = sendOneUpdate(&clntData, sockFd)) != G_OK)
			{
				LOG_ERROR("Sending update all fail");
				break;
			}
			tempNode = tempNode->next;
			clntData.payLoad = NULL;
			clntData.payloadSize = 0;
		}
		if(sendResult != G_OK)
		{
			break;
		}
	}
	if(isClientConnected)
	{
		closeSocket(sockFd);
	}
	pthread_mutex_unlock(&pl->playListLock);
	
}


static void sendDeleteOneToClient(char clientId, u32 m_token)
{
	int sockFd = 0;
	clntData_t *data = NULL;
	CLIENT_DB *cdb = getClientDbInstance();
	data = createClientMsg(clientId,m_token,client_delete_req_m);
	if(connectToClient(clientId,&sockFd) != G_OK)
	{
		LOG_ERROR("Unable to connect to client");
		free(data);
		return;
	}

	if(sendOneUpdate(data,sockFd) != G_OK)
	{
		LOG_ERROR("Unable to send delete message to client");
		free(data);
		closeSocket(sockFd);
	}
	free(data);
	closeSocket(sockFd);
}

static RESULT sendOneUpdate(clntData_t *data, int sockFd)
{
	RESULT res = G_FAIL;
	u32 size = 0;
	char *buffer = NULL;
	if(!data)
	{
		LOG_ERROR("update data is null");
		return res;
	}
	if((size = encodePacket(data,&buffer)) <=0)
	{
		LOG_ERROR("Error in encoding");
		return res;
	}
	if(sendData(sockFd,size,buffer) < size)
	{
		LOG_ERROR("Unable to send update message");
		free(buffer);
		return res;
	}
	if(buffer != NULL)
		free(buffer);

	return G_OK;
}



static RESULT connectToClient(char clientId, int *sockFd)
{
	char buffer[64] = {0};
	CLIENT_DB *cdb = getClientDbInstance();
	struct in_addr addr = {cdb->clientIP[clientId]};
	sprintf(buffer, "%s", inet_ntoa( addr ) );
	LOG_MSG("Connecting to client for sending update %s",buffer);
	
	*sockFd = createClientSocket(buffer,8091);
	if(*sockFd == -1){
		LOG_ERROR("Unable to connect with client for sending update");
		cdb->deregisterClient(cdb,clientId);
		return G_FAIL;
	}
	/*if(setSocketBlockingEnabled(*sockFd,1) != G_OK)
	{
		LOG_ERROR("Unable to set socket non blocking");
		return G_FAIL;
	}*/
	return G_OK;
}


clntData_t *createClientMsg(char clntId, u32 m_token, msg_type_t id)
{
	clntData_t *clntMessage = NULL;
	clntMessage = (clntData_t *)malloc(sizeof(clntData_t));
	if(clntMessage == NULL)
	{
		LOG_ERROR("Malloc failed");
		return NULL;
	}
	clntMessage->header.clntId     = clntId;
  	clntMessage->header.token      = m_token;
  	clntMessage->header.totalSize  = 0;
  	clntMessage->header.msgId      = id;
  	clntMessage->header.isLast     = 1;
  	clntMessage->header.signature  = SIGNATURE;
  	clntMessage->payLoad           = NULL;
  	clntMessage->payloadSize       = 0;

	return clntMessage;
}

void *sendOneUpdateToAllClient(void *msg)
{
	clntData_t *reqMsg = NULL;
	reqMsg = (clntData_t*)msg;
	if(reqMsg == NULL)
	{
		LOG_ERROR("Msg is NULL");
		return;
	}
	char i =0;
	int sockFd = 0;
	CLIENT_DB *cdb = getClientDbInstance();
	if(!cdb)
	{
		LOG_ERROR("Error in client db instance");
		return;
	}	
	for(i=0; i<MAX_CLIENT; i++)
	{
		// skip current client and unregistered client
		if(cdb->clientState[i] == clnt_unregister_state)
			continue;
		if((i == reqMsg->header.clntId) && (reqMsg->header.msgId != client_currentPlaying_m))
			continue;

		if(connectToClient(i, &sockFd) != G_OK)
		{
			LOG_ERROR("Unable to connect with client for sending update");
			cdb->deregisterClient(cdb,i);
			continue;
		}
		if(sendOneUpdate(reqMsg, sockFd) != G_OK)
		{
			LOG_ERROR("Sending update all fail");
		}
		closeSocket(sockFd);
	}
	if(reqMsg->payLoad != NULL)
		free(reqMsg->payLoad);

	free(reqMsg);
}

static void checkAndDeleteCurrentList(char id)
{
	CLIENT_DB *cdb = getClientDbInstance();
	MPlayer *mPlayer = getMPlayerInstance();
	if(cdb->schData[0].clientId == id )
	{
		switch(cdb->schData[0].status)
		{
			case PL_PLAYING:
				mPlayer->stop = 1;
				break;
			case PL_READY:
				cdb->schData[0].status   = PL_NONE;
				unlink(cdb->schData[0].fileName);
				break;
			default:
				break;
		}
	}
	if(cdb->schData[1].clientId == id )
	{
		switch(cdb->schData[1].status)
		{
			case PL_PLAYING:
				mPlayer->stop = 1;
				break;
			case PL_READY:
				cdb->schData[1].status   = PL_NONE;
				unlink(cdb->schData[1].fileName);
				break;
			default:
				break;
		}
	}
}
