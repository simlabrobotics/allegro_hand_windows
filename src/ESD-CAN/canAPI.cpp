/*======================*/
/*       Includes       */
/*======================*/
//system headers
#include <stdio.h>
#include <errno.h>
#ifndef _WIN32
#include <inttypes.h>
#include <pthread.h>
#include <syslog.h>
#include <unistd.h>
#endif
#include <windows.h>
#include <malloc.h>
#include <assert.h>

#include "canDef.h"
#include "canAPI.h"
#include "ESD-CAN/ntcan.h"

CANAPI_BEGIN


#define CH_COUNT			(int)4 // number of CAN channels

static NTCAN_HANDLE canDev[CH_COUNT] = { // CAN channel handles
	(NTCAN_HANDLE)-1,
	(NTCAN_HANDLE)-1,
	(NTCAN_HANDLE)-1,
	(NTCAN_HANDLE)-1
}; 

/*========================================*/
/*       Public functions (CAN API)       */
/*========================================*/
void allowMessage(int bus, int id, int mask){
    int i;
    DWORD retvalue;
	for(i=0;i<2048;i++){
		if((i & ~mask)==id){
			retvalue = canIdAdd(canDev[bus],i);
			if(retvalue != NTCAN_SUCCESS){
#ifndef _WIN32
			  syslog(LOG_ERR, "allowMessage(): canIdAdd() failed with error %d", retvalue);
#endif
			  printf("allowMessage(): canIdAdd() failed with error %ld", retvalue);
			}
		}
	}
}

int initCAN(int bus){
    DWORD retvalue;
#ifndef _WIN32
    pthread_mutex_init(&commMutex, NULL);
#endif
    
    retvalue = canOpen(bus, 0, TX_QUEUE_SIZE, RX_QUEUE_SIZE, TX_TIMEOUT, RX_TIMEOUT, &canDev[bus]);
    if(retvalue != NTCAN_SUCCESS){
#ifndef _WIN32
        syslog(LOG_ERR, "initCAN(): canOpen() failed with error %d", retvalue);
#endif
		printf("initCAN(): canOpen() failed with error %ld", retvalue);
        return(1);
    }
    
    retvalue = canSetBaudrate(canDev[bus], 0); // 1 = 1Mbps, 2 = 500kbps, 3 = 250kbps
    if(retvalue != 0)
    {
#ifndef _WIN32
        syslog(LOG_ERR, "initCAN(): canSetBaudrate() failed with error %d", retvalue);
#endif
		printf("initCAN(): canSetBaudrate() failed with error %ld", retvalue);
        return(1);
    }
    
    // Mask 3E0: 0000 0011 1110 0000
    //allowMessage(bus, 0x0000, 0x03E0); // Messages sent directly to host
    //allowMessage(bus, 0x0403, 0x03E0); // Group 3 messages
    //allowMessage(bus, 0x0406, 0x03E0); // Group 6 messages

	unsigned long Txid;
	Txid = ((unsigned long)(ID_CMD_QUERY_CONTROL_DATA) <<6) | ((unsigned long)ID_DEVICE_MAIN <<3) | ((unsigned long)ID_DEVICE_SUB_01);
	allowMessage(bus, Txid, 0);
	Txid = ((unsigned long)(ID_CMD_QUERY_CONTROL_DATA) <<6) | ((unsigned long)ID_DEVICE_MAIN <<3) | ((unsigned long)ID_DEVICE_SUB_02);
	allowMessage(bus, Txid, 0);
	Txid = ((unsigned long)(ID_CMD_QUERY_CONTROL_DATA) <<6) | ((unsigned long)ID_DEVICE_MAIN <<3) | ((unsigned long)ID_DEVICE_SUB_03);
	allowMessage(bus, Txid, 0);
	Txid = ((unsigned long)(ID_CMD_QUERY_CONTROL_DATA) <<6) | ((unsigned long)ID_DEVICE_MAIN <<3) | ((unsigned long)ID_DEVICE_SUB_04);
	allowMessage(bus, Txid, 0);
	Txid = ((unsigned long)(ID_DEVICE_MAIN)<<3) | ((unsigned long)ID_COMMON);
	allowMessage(bus, Txid, 0x38);
	
    return(0);
}

void freeCAN(int bus){
    canClose(canDev[bus]);
}

int canReadMsg(int bus, int *id, int *len, unsigned char *data, int blocking){
    CMSG    msg;
    DWORD   retvalue;
    long    msgCt = 1;
    int     i;
    
    if(blocking){
        retvalue = canRead(canDev[bus], &msg, &msgCt, NULL);
    }else{
        retvalue = canTake(canDev[bus], &msg, &msgCt);
    }
    if(retvalue != NTCAN_SUCCESS){
#ifndef _WIN32
        syslog(LOG_ERR, "canReadMsg(): canRead/canTake error: %ld", retvalue);
#endif
		//printf("canReadMsg(): canRead/canTake error: %ld", retvalue);
        if(retvalue == NTCAN_RX_TIMEOUT)
            return(1);
        else
            return(2);
    }
    if(msgCt == 1){
        *id = msg.id;
        *len = msg.len;
        for(i = 0; i < msg.len; i++)
            data[i] = msg.data[i];
            
        return(0);
    }
    
    return(1); // No message received, return err
}

int canSendMsg(int bus, int id, char len, unsigned char *data, int blocking){
    CMSG    msg;
    DWORD   retvalue;
    long    msgCt = 1;
    int     i;
    
    msg.id = id;
    msg.len = len & 0x0F;
    for(i = 0; i < len; i++)
        msg.data[i] = data[i];
    
    if(blocking){
        retvalue = canWrite(canDev[bus], &msg, &msgCt, NULL);
    }else{
        retvalue = canSend(canDev[bus], &msg, &msgCt);
    }
    
    if(retvalue != NTCAN_SUCCESS){
#ifndef _WIN32
        syslog(LOG_ERR, "canSendMsg(): canWrite/Send() failed with error %d", retvalue);
#endif
		printf("canSendMsg(): canWrite/Send() failed with error %ld", retvalue);
        return(1);
    }
    return 0;
}

/*========================================*/
/*       CAN API                          */
/*========================================*/
int command_can_open(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	DWORD ret;

	printf("<< CAN: Open Channel...\n");
	ret = initCAN(ch);
	if (ret != 0) return ret;
	printf("\t- Ch.%2d (OK)\n", ch);
	printf("\t- Done\n");

	return 0;
}

int command_can_open_ex(int ch, int type, int index)
{
	return command_can_open(ch);
}

int command_can_reset(int ch)
{
	return -1;
}

int command_can_close(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	DWORD ret;

	printf("<< CAN: Close...\n");
	ret = canClose(canDev[ch]);
	if (ret != 0) return ret;
	canDev[ch] = (NTCAN_HANDLE)-1;
	printf("\t- Done\n");

	return 0;
}

int command_can_query_id(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_QUERY_ID<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(ch, Txid, 0, data, TRUE);

	return 0;
}

int command_can_sys_init(int ch, int period_msec)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_SET_PERIOD<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	data[0] = (unsigned char)period_msec;
	ret = canSendMsg(ch, Txid, 1, data, TRUE);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_MODE_TASK<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(ch, Txid, 0, data, TRUE);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(ch, Txid, 0, data, TRUE);

	return 0;
}

int command_can_start(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(ch, Txid, 0, data, TRUE);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_ON<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(ch, Txid, 0, data, TRUE);

	return 0;
}

int command_can_stop(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_OFF<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(ch, Txid, 0, data, TRUE);

	return 0;
}

int command_can_AHRS_set(int ch, unsigned char rate, unsigned char mask)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_AHRS_SET<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	data[0] = (unsigned char)rate;
	data[1] = (unsigned char)mask;
	ret = canSendMsg(ch, Txid, 2, data, TRUE);

	return 0;
}

int write_current(int ch, int findex, short* pwm)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	int ret;

	if (findex >= 0 && findex < 4)
	{
		data[0] = (unsigned char)( (pwm[0] >> 8) & 0x00ff);
		data[1] = (unsigned char)(pwm[0] & 0x00ff);

		data[2] = (unsigned char)( (pwm[1] >> 8) & 0x00ff);
		data[3] = (unsigned char)(pwm[1] & 0x00ff);

		data[4] = (unsigned char)( (pwm[2] >> 8) & 0x00ff);
		data[5] = (unsigned char)(pwm[2] & 0x00ff);

		data[6] = (unsigned char)( (pwm[3] >> 8) & 0x00ff);
		data[7] = (unsigned char)(pwm[3] & 0x00ff);

		Txid = ((unsigned long)(ID_CMD_SET_TORQUE_1 + findex)<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
		ret = canSendMsg(ch, Txid, 8, data, TRUE);
	}
	else
		return -1;
	
	return 0;
}

int get_message(int ch, char* cmd, char* src, char* des, int* len, unsigned char* data, int blocking)
{
	int Rxid;
	unsigned char rdata[8];
	int dlc;
	int ret;

	memset(rdata, NULL, sizeof(rdata));
	ret = canReadMsg(ch, &Rxid, &dlc, rdata, TRUE);
	if (ret != 0) return ret;
	//printf("    %ld+%ld (%d)", Rxid-Rxid%128, Rxid%128, dlc);
	//for(int nd=0; nd<(int)dlc; nd++) printf(" %3d ", rdata[nd]);
	//printf("\n");

	*cmd = (char)( (Rxid >> 6) & 0x1f );
	*des = (char)( (Rxid >> 3) & 0x07 );
	*src = (char)( Rxid & 0x07);
	*len = (int)dlc;
	for(int nd=0; nd<(int)dlc; nd++) data[nd] = rdata[nd];

	return 0;
}



CANAPI_END
