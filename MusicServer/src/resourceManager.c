#include "resourceManager.h"
#include "serverRoutine.h"
#include "gthreads.h"
#include "results.h"
#include "queue.h"
#include "server.h"
#include "clientdb.h"
#include "musicPlayer.h"
#include <stdio.h>
#include <pthread.h>

static RManager *rmInstance = NULL;

pthread_mutex_t rmLock;
pthread_cond_t  rmCond;

static void runResourceManager(RManager *);
static RESULT postMsgToRmQueue(RManager *rm, rmMsg_t *rmsg);
static void initRmManager(RManager *rm);
static void servRequest(RManager *rm, rmMsg_t * rmsg);
static void servClientRequest(clntMsg_t *msg);
static void servMplayerResponse(RManager *rm, rmMsg_t * rmsg);

//State Machine Functions
static void printRMStateError(RManager *rm, rmMsg_t * rmsg);
static void moveToIdle(RManager *rm, rmMsg_t * rmsg);
static void moveToDownloading(RManager *rm, rmMsg_t * rmsg);
static void moveToPlaying(RManager *rm, rmMsg_t * rmsg);
static void moveToPlayingDownloading(RManager *rm, rmMsg_t * rmsg);
static void moveToPlayingDownloaded(RManager *rm, rmMsg_t * rmsg);
static void callSchedular(RManager *rm, rmMsg_t * rmsg);
static char getNextFileName(RManager *rm);
static char getNextFileNameIndex(RManager *rm, u32 requestId,char clntId);
//State Machine Functions

static void handleDownloadComplete(RManager *rm,rmMsg_t * rmsg);
static void handleDownloadError(RManager *rm,rmMsg_t * rmsg);

RManager *getRManagerInstance()
{
	if(rmInstance == NULL)
	{
		rmInstance = (RManager *)malloc(sizeof(RManager));
		// Constructor, Initialize Resource Manager Instance
		initRmManager(rmInstance);
	}
	return rmInstance;
}

static void initRmManager(RManager *rm)
{
	initQueue(&rm->rmQueue);
	pthread_mutex_init(&rmLock,NULL);
	pthread_cond_init(&rmCond,NULL);
	rm->postMsgToRm = postMsgToRmQueue;
	rm->runRM       = runResourceManager;
	rm->getNextFileName = getNextFileName;
	rm->currentReq[0]   = NULL;
	rm->currentReq[1]   = NULL;
	rm->rmState     = rm_Idle;
	rm->fileNameBit = 0;
	
	rm->rmRunState[rm_Idle][rm_clntMsg_m]     = callSchedular; //check first msg id
	rm->rmRunState[rm_Idle][rm_mplayerDone_m] = printRMStateError;
	rm->rmRunState[rm_Idle][rm_mplayerStop_m] = printRMStateError;
	rm->rmRunState[rm_Idle][rm_mplayerErr_m]  = printRMStateError;
	rm->rmRunState[rm_Idle][rm_dw_complete_m] = printRMStateError;
	rm->rmRunState[rm_Idle][rm_dw_err_m]      = printRMStateError;

	rm->rmRunState[rm_Downloading][rm_clntMsg_m]     = NULL;
	rm->rmRunState[rm_Downloading][rm_mplayerDone_m] = printRMStateError;
	rm->rmRunState[rm_Downloading][rm_mplayerStop_m] = printRMStateError;
	rm->rmRunState[rm_Downloading][rm_mplayerErr_m]  = printRMStateError;
	rm->rmRunState[rm_Downloading][rm_dw_complete_m] = moveToPlaying; // MOve state to playing
	rm->rmRunState[rm_Downloading][rm_dw_err_m]      = moveToIdle; // move to Idle; call scheduling

	rm->rmRunState[rm_Playing][rm_clntMsg_m]     = callSchedular;
	rm->rmRunState[rm_Playing][rm_mplayerDone_m] = moveToIdle;// Move to Idle
	rm->rmRunState[rm_Playing][rm_mplayerStop_m] = printRMStateError;
	rm->rmRunState[rm_Playing][rm_mplayerErr_m]  = moveToIdle;
	rm->rmRunState[rm_Playing][rm_dw_complete_m] = printRMStateError;
	rm->rmRunState[rm_Playing][rm_dw_err_m]      = printRMStateError;

	rm->rmRunState[rm_Playing_Downloading][rm_clntMsg_m]     = NULL;
	rm->rmRunState[rm_Playing_Downloading][rm_mplayerDone_m] = moveToDownloading;
	rm->rmRunState[rm_Playing_Downloading][rm_mplayerStop_m] = printRMStateError;
	rm->rmRunState[rm_Playing_Downloading][rm_mplayerErr_m]  = moveToDownloading; // Move to rmDownloading
	rm->rmRunState[rm_Playing_Downloading][rm_dw_complete_m] = moveToPlayingDownloaded; // Move to Downloaded
	rm->rmRunState[rm_Playing_Downloading][rm_dw_err_m]      = moveToPlaying;// Move to playing; call schedular for next song

	rm->rmRunState[rm_Playing_Downloaded][rm_clntMsg_m]     = NULL;
	rm->rmRunState[rm_Playing_Downloaded][rm_mplayerDone_m] = moveToPlaying; // send to MusicPlayer; move to playing
	rm->rmRunState[rm_Playing_Downloaded][rm_mplayerStop_m] = printRMStateError;
	rm->rmRunState[rm_Playing_Downloaded][rm_mplayerErr_m]  = moveToPlaying; //send to MusicPlayer; move to playing
	rm->rmRunState[rm_Playing_Downloaded][rm_dw_complete_m] = printRMStateError;
	rm->rmRunState[rm_Playing_Downloaded][rm_dw_err_m]      = printRMStateError;
	
}

static RESULT postMsgToRmQueue(RManager *rm, rmMsg_t *rmsg)
{
	if(!rm || !rmsg)
		return G_ERR;
	
	if(enqueue(&rm->rmQueue,(void *)rmsg) != G_OK)
	{
		return G_ERR;
	}
	pthread_cond_signal(&rmCond);
	return G_OK;
}

static void runResourceManager(RManager *rm)
{
	void *rmsg = NULL;
	if(!rm){
		printf("Error in satarting Resource Manager\n");
		return;
	}
	printf("running %s\n",__FUNCTION__);
	// Never return from here
	while(1)
	{
		pthread_mutex_lock(&rmLock);
			pthread_cond_wait(&rmCond, &rmLock);
		pthread_mutex_unlock(&rmLock);
		while( (rmsg = dequeue(&rm->rmQueue)) != NULL)
			servRequest(rm, (rmMsg_t *)rmsg);
		//free rm message
		free(rmsg);

	}
}

static void servRequest(RManager *rm, rmMsg_t * rmsg)
{
	switch(rmsg->msgId)
	{
		case rm_clntMsg_m:
			servClientRequest((clntMsg_t *)rmsg->data);
			break;
		case rm_mplayerDone_m:
		case rm_mplayerStop_m:
		case rm_mplayerErr_m:
			servMplayerResponse(rm, rmsg);
			break;
		case rm_dw_complete_m:
			handleDownloadComplete(rm,rmsg);
			break;
		case rm_dw_err_m:
			handleDownloadError(rm,rmsg);
			break;
		default:
			printf("Error : [%s : %d] : default case\n");
			break;
	}
	printf("RM CURRENT STATE %d\n",rm->rmState);
	// Run the state machine on any msg received
	if( rm->rmRunState[rm->rmState][rmsg->msgId] != NULL)
	{
		rm->rmRunState[rm->rmState][rmsg->msgId](rm,rmsg);
	}
	free(rmsg);
}

static void servClientRequest(clntMsg_t *msg)
{
	if(!msg){
		printf("Error : [%s : %d] : clntMsg_t is null\n");
		return;
	}
	CLIENT_DB *cdb = getClientDbInstance();
	CLIENT_INFO clntInfo;
	CLIENT_DATA clntMusicData;
	if(!cdb)
	{
		printf("Error : [%s : %d] : CLIENT_DB is null\n",__FUNCTION__,__LINE__);
		msg->clntData.header.msgId = resErr_m;
		//Post message to threadPool Queue
		addJobToQueue(sendNACKandClose, (void *)msg);
		return;
	}
	
	switch(msg->clntData.header.msgId)
	{
		case register_m:
			printf("register clnt request\n");
			clntInfo.clientIP = msg->cli_addr;
			if(msg->clntData.payloadSize == 0 || !msg->clntData.payLoad )
			{
				msg->clntData.header.msgId = resErr_m;
				//Post message to threadPool Queue
				addJobToQueue(sendNACKandClose, (void *)msg);
				return;
			}
			memset(clntInfo.clientName,0,MAX_CLIENT_NAME);
			if(msg->clntData.payloadSize > MAX_CLIENT_NAME)
			{
				memcpy(clntInfo.clientName, msg->clntData.payLoad,MAX_CLIENT_NAME-1);
			}
			else
			{
				memcpy(clntInfo.clientName, msg->clntData.payLoad,msg->clntData.payloadSize);
			}
			printf("calling registerClient in DB for User %s\n",clntInfo.clientName);
			if( cdb->registerClient(cdb,&clntInfo) != G_OK)
			{
				msg->clntData.header.msgId = resErr_m;
				//Post message to threadPool Queue
				addJobToQueue(sendNACKandClose, (void *)msg);
				return;
			}
			// Copy the clntId returned by ClientDb and return to client
			// All the subsequent messages will be identified by this Id only
			msg->clntData.header.msgId = resOk_m;
			msg->clntData.header.clntId = clntInfo.clientId;
			printf("Everything is ok:register client\n");
			
			break;
		case add_m:
			if(msg->clntData.payLoad == NULL)
			{
				msg->clntData.header.msgId = resErr_m;
				break;
			}
			/*if(decodeMusicAddData(msg->clntData.payLoad, &clntMusicData) != G_OK)
			{
				msg->clntData.header.msgId = resErr_m;
				break;
			}*/
			clntMusicData.requestId = msg->clntData.header.token;
			if( cdb->addToQueue(cdb,msg->clntData.header.clntId,msg->clntData.header.token,msg->clntData.payLoad,msg->clntData.payLoad) != G_OK)
			{
				msg->clntData.header.msgId = resErr_m;
				break;
			}
			msg->clntData.header.msgId = resOk_m;
			break;
		case deregister_m:

			break;
		case delete_m:

			break;
	}
	//Post message to threadPool Queue
	addJobToQueue(sendACKandClose, (void *)msg);
}


static void servMplayerResponse(RManager *rm, rmMsg_t *rmsg)
{
	if(rm->currentReq[0] != NULL)
	{
		if(rm->currentReq[0]->fileState == MP3_FILE_PLAYING)
		{
			printf("INFO [%s : %d] MP3 File Playing Completed successfully, clnt id %d, token %u\n",__FUNCTION__,__LINE__,rm->currentReq[0]->clntId,rm->currentReq[0]->requestId);
			free(rm->currentReq[0]);
			rm->currentReq[0] = NULL;
			return;
		}	
	}
	if(rm->currentReq[1] != NULL)
	{
		if(rm->currentReq[1]->fileState == MP3_FILE_PLAYING)
		{
			printf("INFO [%s : %d] MP3 File Playing Completed successfully, clnt id %d, token %u\n",__FUNCTION__,__LINE__,rm->currentReq[1]->clntId,rm->currentReq[1]->requestId);
			free(rm->currentReq[1]);
			rm->currentReq[1] = NULL;
			return;
		}	
	}
}



//state Machine functions
static void printRMStateError(RManager *rm, rmMsg_t * rmsg)
{
	printf("ERROR STATE MACHINE current state %d -> msg Id %d\n",rm->rmState,rmsg->msgId);
}

static void moveToIdle(RManager *rm, rmMsg_t * rmsg)
{
	rm->rmState = rm_Idle;
	callSchedular(rm,rmsg);	
}

static void moveToDownloading(RManager *rm, rmMsg_t * rmsg)
{
	rm->rmState = rm_Downloading;
}

static void moveToPlaying(RManager *rm, rmMsg_t *rmsg)
{
	MP3_FILE_REQ *req = NULL;
	char index = -1;
	if(rm->rmState == rm_Downloading){
		req = (MP3_FILE_REQ *)rmsg->data;
		index = getNextFileNameIndex(rm,req->requestId,req->clntId);
		if(index == -1 )
		{
			printf("[%s : %d] Index can not be -1\n",__FUNCTION__,__LINE__);
			rm->rmState = rm_Idle;
			return;
		}
		
		MPlayer *mp = getMPlayerInstance();
		if(mp == NULL){
			printf("[%s : %d] Fatal Error cant get MPlayer Instance\n",__FUNCTION__,__LINE__);
			rm->rmState = rm_Idle;
			return;
		}
		sprintf(mp->fileName,"%s_%d.mp3",MP3FILE_LOC,index);
		printf("Sending Request to MPlayer to Play %s\n",mp->fileName);
		if(addJobToQueue(mp->playSong,(void *)mp) != G_OK){
			printf("[%s : %d] Fatal Error cant sent to job queue\n",__FUNCTION__,__LINE__);
			rm->rmState = rm_Idle;
			return;
		}
		req->fileState = MP3_FILE_PLAYING;

		//make rm state rm_Playing and call scheduling
		rm->rmState = rm_Playing;
		callSchedular(rm,rmsg);
	} 
	else if(rm->rmState == rm_Playing_Downloading || rm->rmState == rm_Playing_Downloaded) {
		//TODO call schedular for next song
		rm->rmState = rm_Playing;
		callSchedular(rm,rmsg);
	}
}
static void moveToPlayingDownloading(RManager *rm, rmMsg_t * rmsg)
{
	rm->rmState = rm_Playing_Downloading;
}
static void moveToPlayingDownloaded(RManager *rm, rmMsg_t * rmsg)
{
	rm->rmState = rm_Playing_Downloaded;
}


static void callSchedular(RManager *rm, rmMsg_t *rmsg)
{
	char fileName = rm->getNextFileName(rm);
	u32 ipAddress = 0;
	u32 requestId = 0;
	if(fileName == -1)
	{
		printf("[%s : %d] Nothing to done both request is under processing\n",__FUNCTION__,__LINE__);
		return;

	}
	CLIENT_DB *cdb = getClientDbInstance();
	if(cdb == NULL)
	{
		printf("[%s : %d] Client DB instance is NULL\n",__FUNCTION__,__LINE__); 
	}
	char clientId = -1;
	if(cdb->getNextScheduledClient(cdb,&clientId) != G_OK)
	{
		printf("[%s : %d] No client to schedule\n",__FUNCTION__,__LINE__);
		return;
	}
	printf("Scheduled client is %d\n",clientId);
	if(cdb->getClientFirstToken(cdb,clientId,&requestId) != G_OK)
	{
		printf("[%s : %d] No client to schedule, no one has req pending\n",__FUNCTION__,__LINE__);
		return;
	}
	if(cdb->getClientIpAddress(cdb,clientId,&ipAddress) != G_OK)
	{
		printf("[%s : %d] Error client ip address not found\n",__FUNCTION__,__LINE__);
		return;
	}
	
	MP3_FILE_REQ *mp3Req = (MP3_FILE_REQ*)malloc(sizeof(MP3_FILE_REQ));
	mp3Req->clntId = clientId;
	mp3Req->fileName = fileName;
	mp3Req->ipaddress = ipAddress;
	mp3Req->requestId = requestId;
	mp3Req->fileState = MP3_DOWNLOADING;
	
	rm->currentReq[fileName] = mp3Req;

	switch(rm->rmState){
		case rm_Idle:
			rm->rmState = rm_Downloading;
			break;
		case rm_Playing:
			rm->rmState = rm_Playing_Downloading;
			break;
		default:
			printf("[%s : %d]Invalid Event in CallSchedular %d\n",__FUNCTION__,__LINE__,rm->rmState);	
			break;
	}
	//TODO set clientState schedule1
	addJobToQueue(getMP3FileFromClient, (void*)mp3Req);
}

static char getNextFileName(RManager * rm)
{
	if(rm->currentReq[0] == NULL)
	{
		return 0;
	} else if(rm->currentReq[1] == NULL){
		return 1;
	} else {
		return -1;
	}
}


static char getNextFileNameIndex(RManager *rm, u32 requestId,char clntId)
{
	if(rm->currentReq[0] != NULL)
	{
		if(rm->currentReq[0]->clntId == clntId && rm->currentReq[0]->requestId == requestId)
			return 0;
	}
	if(rm->currentReq[1] != NULL)
	{
		if(rm->currentReq[1]->clntId == clntId && rm->currentReq[1]->requestId == requestId)
			return 1;
	}
	return -1;
}


static void handleDownloadComplete(RManager *rm,rmMsg_t * rmsg)
{
	if(rm->currentReq[0] != NULL)
	{
		if(rm->currentReq[0]->fileState == MP3_DOWNLOADING)
		{
			rm->currentReq[0]->fileState = MP3_FILE_READY;
			printf("INFO [%s : %d] MP3 File Downloaded successfully, clnt id %d, token %u\n",__FUNCTION__,__LINE__,rm->currentReq[0]->clntId,rm->currentReq[0]->requestId);
			return;
		}	
	}
	if(rm->currentReq[1] != NULL)
	{
		if(rm->currentReq[1]->fileState == MP3_DOWNLOADING)
		{
			rm->currentReq[1]->fileState = MP3_FILE_READY;
			printf("INFO [%s : %d] MP3 File Downloaded successfully, clnt id %d, token %u\n",__FUNCTION__,__LINE__,rm->currentReq[1]->clntId,rm->currentReq[1]->requestId);
			return;
		}	
	}
}

static void handleDownloadError(RManager *rm,rmMsg_t * rmsg)
{
	if(rm->currentReq[0] != NULL)
	{
		if(rm->currentReq[0]->fileState == MP3_DOWNLOADING)
		{
			printf("ERROR [%s : %d] MP3 File Downloading Error, clnt id %d, token %u\n",__FUNCTION__,__LINE__,rm->currentReq[0]->clntId,rm->currentReq[0]->requestId);
			free(rm->currentReq[0]);
			rm->currentReq[0] = NULL;
			return;
		}	
	}
	if(rm->currentReq[1] != NULL)
	{
		if(rm->currentReq[1]->fileState == MP3_DOWNLOADING)
		{
			printf("ERROR [%s : %d] MP3 File Downloading Error, clnt id %d, token %u\n",__FUNCTION__,__LINE__,rm->currentReq[1]->clntId,rm->currentReq[1]->requestId);
			free(rm->currentReq[1]);
			rm->currentReq[1] = NULL;
			return;
		}	
	}
}

