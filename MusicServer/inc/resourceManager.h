#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

#include "results.h"
#include "queue.h"
#include "serverRoutine.h"
#include "packet.h"

#define MP3FILE_LOC	"/tmp/Mserver"

typedef enum
{
	rm_clntMsg_m,
	rm_mplayerDone_m,
	rm_mplayerStop_m,
	rm_mplayerErr_m,
	rm_dw_complete_m,
	rm_dw_err_m,
	rm_lastMsg_m
	
}rmMsgID_e;

typedef struct
{
	rmMsgID_e msgId;
	void *data;
}rmMsg_t;

typedef void(*rmStateFunc)(struct rmMan *,rmMsg_t *);

typedef enum
{
	rm_Idle,
	rm_Downloading,
	rm_Playing,
	rm_Playing_Downloading,
	rm_Playing_Downloaded,
	rm_LastState
}rmState_e;



typedef struct rmMan
{
	QUEUE rmQueue;
	rmState_e rmState;
	rmStateFunc rmRunState[rm_LastState][rm_lastMsg_m];
	RESULT (*postMsgToRm)(struct rmMan *, rmMsg_t *);
	void (*runRM)(struct rmMan *);
}RManager;


RManager *getRManagerInstance();

#endif //__RESOURCE_MANAGER_H__
