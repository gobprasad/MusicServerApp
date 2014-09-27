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
#include "loggingFrameWork.h"

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
static void moveToPlayingScheduleNext(RManager *rm, rmMsg_t * rmsg);
static void moveToPlayingDownloaded(RManager *rm, rmMsg_t * rmsg);
static void callSchedular(RManager *rm, rmMsg_t * rmsg);
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
	rm->rmState     = rm_Idle;
	
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
	rm->rmRunState[rm_Playing_Downloading][rm_dw_err_m]      = moveToPlayingScheduleNext;// Move to playing; call schedular for next song

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
		LOG_ERROR("Error in satarting Resource Manager");
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
			LOG_ERROR("default case");
			break;
	}
	// Run the state machine on any msg received
	if( rm->rmRunState[rm->rmState][rmsg->msgId] != NULL)
	{
		rm->rmRunState[rm->rmState][rmsg->msgId](rm,rmsg);
	}
	free(rmsg);
	LOG_MSG("RM CURRENT STATE %d",rm->rmState);
}

static void servClientRequest(clntMsg_t *msg)
{
	int isRegisterMsg = 0;
	if(!msg){
		LOG_ERROR("clntMsg_t is null");
		return;
	}
	CLIENT_DB *cdb = getClientDbInstance();
	CLIENT_INFO clntInfo;
	CLIENT_DATA clntMusicData;
	if(!cdb)
	{
		LOG_ERROR("CLIENT_DB is null");
		msg->clntData.header.msgId = resErr_m;
		//Post message to threadPool Queue
		sendNACKandClose((void *)msg);
		return;
	}
	
	switch(msg->clntData.header.msgId)
	{
		case register_m:
			LOG_MSG("register clnt request");
			clntInfo.clientIP = msg->cli_addr;
			if(msg->clntData.payloadSize == 0 || !msg->clntData.payLoad )
			{
				msg->clntData.header.msgId = resErr_m;
				//Post message to threadPool Queue
				sendNACKandClose((void *)msg);
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
			LOG_MSG("calling registerClient in DB for User %s",clntInfo.clientName);
			if( cdb->registerClient(cdb,&clntInfo) != G_OK)
			{
				msg->clntData.header.msgId = resErr_m;
				//Post message to threadPool Queue
				sendNACKandClose((void *)msg);
				return;
			}
			// Copy the clntId returned by ClientDb and return to client
			// All the subsequent messages will be identified by this Id only
			msg->clntData.header.msgId = resOk_m;
			msg->clntData.header.clntId = clntInfo.clientId;
			//Send Ack to client and close connection
			sendACKandClose((void *)msg);

			// send playlist update to current client
			addJobToQueue(sendAllUpdateToClient,(void *)&(cdb->clientId[clntInfo.clientId]));
			break;
		case add_m:
			LOG_MSG("register clnt request");
			if(msg->clntData.payLoad == NULL)
			{
				msg->clntData.header.msgId = resErr_m;
				LOG_ERROR("PayLoad is NULL");
				sendNACKandClose((void *)msg);
				return;
			}
			clntMusicData.requestId = msg->clntData.header.token;
			if( cdb->addToQueue(cdb,msg->clntData.header.clntId,msg->clntData.header.token,msg->clntData.payLoad,msg->clntData.payloadSize) != G_OK)
			{
				msg->clntData.header.msgId = resErr_m;
				LOG_ERROR("Error in addToQueue");
				sendNACKandClose((void *)msg);
				return;
			}
			msg->clntData.header.msgId = resOk_m;

			//Create a copy of request msg and send to all connected clients.
			clntData_t *copyMsg = NULL;
			copyMsg = (clntData_t*)malloc(sizeof(clntData_t));
			copyMsg->header.msgId = client_add_m;
			memcpy(copyMsg,&msg->clntData,sizeof(clntData_t));
			copyMsg->payLoad = (uchar *)malloc(msg->clntData.payloadSize);
			memcpy(copyMsg->payLoad,msg->clntData.payLoad,msg->clntData.payloadSize);
			
			//Send Ack to client and close connection
			sendACKandClose((void *)msg);

			//send one update to all connected clients
			addJobToQueue(sendOneUpdateToAllClient,(void *)copyMsg);

			break;
		case deregister_m:

			break;
		case delete_m:

			break;
	}
	
}


static void servMplayerResponse(RManager *rm, rmMsg_t *rmsg)
{
	CLIENT_DB *cdb = getClientDbInstance();
	//Playing done remove client request from database
	cdb->setPlayingStatus(cdb);
}



//state Machine functions
static void printRMStateError(RManager *rm, rmMsg_t * rmsg)
{
	LOG_ERROR("ERROR STATE MACHINE current state %d -> msg Id %d",rm->rmState,rmsg->msgId);
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
	MPlayer *mp = getMPlayerInstance();
	CLIENT_DB *cdb = getClientDbInstance();
	if(mp == NULL){
		LOG_ERROR("Fatal Error cant get MPlayer Instance");
		rm->rmState = rm_Idle;
		return;
	}
	switch(rm->rmState)
	{
		case rm_Downloading:
		case rm_Playing_Downloaded:
			if(cdb->getClientRequestForPlaying(cdb,&mp->fileName) != G_OK)
			{
				LOG_ERROR("File is not ready for Playing");
				rm->rmState = rm_Idle;
				break;
			}
			LOG_MSG("Sending file %s to MP3 Player for playing", mp->fileName);
			if(addJobToQueue(mp->playSong,(void *)mp) != G_OK)
			{
				LOG_ERROR("Fatal Error cant sent to job queue");
				rm->rmState = rm_Idle;
				break;
			}
			rm->rmState = rm_Playing;
			callSchedular(rm,rmsg);
			break;
		case rm_Playing_Downloading:
			rm->rmState = rm_Downloading;
			//callSchedular(rm,rmsg);
			break;

		default:
			LOG_ERROR("default case hit");
			break;
	}	
}

static void moveToPlayingScheduleNext(RManager *rm, rmMsg_t * rmsg)
{
	rm->rmState = rm_Playing;
	callSchedular(rm,rmsg);
}
static void moveToPlayingDownloaded(RManager *rm, rmMsg_t * rmsg)
{
	rm->rmState = rm_Playing_Downloaded;
}


static void callSchedular(RManager *rm, rmMsg_t *rmsg)
{
	CLIENT_DB *cdb = getClientDbInstance();
	if(cdb == NULL)
	{
		LOG_ERROR("Client DB instance is NULL"); 
		return;
	}

	MP3_FILE_REQ *mp3Req = (MP3_FILE_REQ*)malloc(sizeof(MP3_FILE_REQ));	
	if(cdb->getClientRequestForDownloading(cdb,mp3Req) != G_OK)
	{
		LOG_MSG("No Client to schedule");
		free(mp3Req);
		return;
	}
	switch(rm->rmState){
		case rm_Idle:
			rm->rmState = rm_Downloading;
			break;
		case rm_Playing:
			rm->rmState = rm_Playing_Downloading;
			break;
		default:
			LOG_ERROR("Invalid Event in CallSchedular %d",rm->rmState);	
			break;
	}
	//TODO set clientState schedule1
	addJobToQueue(getMP3FileFromClient, (void*)mp3Req);
}


static void handleDownloadComplete(RManager *rm,rmMsg_t * rmsg)
{
	LOG_MSG("File download completed successfully");
	MP3_FILE_REQ *mp3Req;
	mp3Req = (MP3_FILE_REQ *)rmsg->data;
	mp3Req->fileState = MP3_FILE_READY;
	CLIENT_DB *cdb = getClientDbInstance();
	if(cdb->setDownloadingStatus(cdb,mp3Req) != G_OK)
	{
		LOG_ERROR("Fatal Error in setting download status MP3_FILE_READY");
	}
}

static void handleDownloadError(RManager *rm,rmMsg_t * rmsg)
{
	LOG_WARN("Error in download");
	MP3_FILE_REQ *mp3Req;
	mp3Req = (MP3_FILE_REQ *)rmsg->data;
	mp3Req->fileState = MP3_FILE_ERROR;
	CLIENT_DB *cdb = getClientDbInstance();
	if(cdb->setDownloadingStatus(cdb,mp3Req) != G_OK)
	{
		LOG_ERROR("Fatal Error in setting download status MP3_FILE_ERROR");
	}
}
