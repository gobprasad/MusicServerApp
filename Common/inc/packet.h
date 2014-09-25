#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>
#include "results.h"
typedef uint32_t u32;
typedef uint8_t  u8;

#ifdef __WINDOWS__
    #include <QtGlobal>
#else
    typedef unsigned char uchar;
#endif

#define SIGNATURE	0x55
#define CLNT_SIGNATURE	0x13

typedef struct
{
	u32	totalSize;
	u32	token;
	u8	msgId;
	u8	clntId;
	u8	signature;
    u8  isLast;
}header_t;

typedef enum
{
	register_m = 1,
	deregister_m,
	add_m,
	delete_m,
	update_m,
	resOk_m,
	resErr_m,
	getFile_m,
	client_delete_req_m,
	client_update_all,
	client_update_all_finish,
	last_m
}msg_type_t;

typedef struct
{
	header_t header;
    uchar *payLoad;
	u32 payloadSize;
}clntData_t;

u32 encodePacket(clntData_t *msg, uchar **retBuf);
RESULT decodePacket(clntData_t *msg, uchar *buf, u32 size);

u32 getNewToken();
u32 getCurrentToken();

#endif
