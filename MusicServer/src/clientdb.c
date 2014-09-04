#include "clientdb.h"
#include "results.h"

static CLIENT_DB *clientDBinstance = NULL;
static void initClientDb(CLIENT_DB *cdb);
static RESULT registerClient(CLIENT_DB *cdb, CLIENT_INFO *clntInfo);
static RESULT deregisterClient(CLIENT_DB *cdb, clntid_t id);
static RESULT addToClientQueue(CLIENT_DB *cdb,clntid_t id,u32 token,void *msg,u32 size);



static RESULT getForDownloading(CLIENT_DB *cdb,clntid_t id,void *res);
static RESULT getForPlaying(CLIENT_DB *cdb,clntid_t id);

static RESULT deleteDownloading(CLIENT_DB *cdb,clntid_t id);
static RESULT deletePlaying(CLIENT_DB *cdb,clntid_t id);

static RESULT deleteAllDataOfClient(CLIENT_DB *cdb,clntid_t id);


static RESULT getClientIpAddress(CLIENT_DB *cdb,clntid_t id, u32 *address);
static RESULT setClientStateActive(CLIENT_DB *cdb, clntid_t id);

static RESULT sendUpdateToClient(CLIENT_DB *cdb, clntid_t id);

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
	cdb->currentScheduleClient = 0;
	cdb->registerClient			= registerClient;
	cdb->deregisterClient		= deregisterClient;
	cdb->addToQueue				= addToClientQueue;

	cdb->getClientIpAddress     = getClientIpAddress;
	cdb->setClientStateActive   = setClientStateActive;
	
	//Get PlayList Class Instance
	playList = getPlayListInstance();
}


/**
 * Register Client
 * Add to database, create client data and add unique ID
 */

static RESULT registerClient(CLIENT_DB *cdb, CLIENT_INFO *clntInfo)
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
		//printf("Client is connecting second time\n");
		// deregister old session
		deregisterClient(cdb, i);
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
	
	return G_OK;
}

/**
 * De Register Client
 * Delete Client Instance
 */
static RESULT deregisterClient(CLIENT_DB *cdb, clntid_t id)
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
	cdb->deleteAllDataOfClient(id);
	memset(cdb->clientName[id],0,MAX_CLIENT_NAME+1);
	cdb->clientIP[id] = 0;
	cdb->clientState[id] = clnt_unregister_state;
	
	return G_OK;
}

/**
 * Add request to client Queue
 */
static RESULT addToClientQueue(CLIENT_DB *cdb,clntid_t id,u32 token,void *msg,u32 size)
{
	RESULT res = G_ERR;
	if(!msg || !cdb || (id >= MAX_CLIENT) || (cdb->clientState[id] == clnt_unregister_state) )
	{
		//printf("Error in add to client queue\n");
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

static RESULT getClientIpAddress(CLIENT_DB *cdb,clntid_t id, u32 *address)
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

static RESULT getClientRequestForDownloading(CLIENT_DB *cdb)
{



}

static RESULT setDownloadingStatus(CLIENT_DB *cdb, STATUS)
{


}

static RESULT getClientRequestForPlaying(CLIENT_DB *cdb)
{



}

static RESULT setPlayingStatus(CLIENT_DB *cdb, STATUS)
{


}
