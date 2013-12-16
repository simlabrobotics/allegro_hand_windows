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
#include "Kvaser/canlib.h"

CANAPI_BEGIN


#define CH_COUNT			(int)2 // number of CAN channels

static int hCAN[CH_COUNT] = {-1, -1}; // CAN channel handles

int command_can_open(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	canStatus ret;
	
	printf("<< CAN: Initialize Library...\n");
	canInitializeLibrary();
	printf("\t- Done\n");
	Sleep(200);

	printf("<< CAN: Open Channel...\n");
	hCAN[ch] = canOpenChannel(ch, canOPEN_EXCLUSIVE);
	if (hCAN[ch] < 0) return -1;
	printf("\t- Ch.%2d (OK)\n", ch);
	printf("\t- Done\n");
	Sleep(200);

	printf("<< CAN: Set Bus Parameter...\n");
	ret = canSetBusParams(hCAN[ch], BAUD_1M, 0, 0, 0, 0, 0);
	if (ret < 0) return -2;
	printf("\t- Done\n");
	Sleep(200);

	printf("<< CAN: Bus On...\n");
	ret = canBusOn(hCAN[ch]);
	if (ret < 0) return -3;
	printf("\t- Done\n");
	Sleep(200);

	return 0;
}

int command_can_reset(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	canStatus ret;

	printf("<< CAN: Reset Bus...\n");
	ret = canResetBus(hCAN[ch]);
	if (ret < 0) return ret;
	printf("\t- Done\n");
	Sleep(200);

	return 0;
}

int command_can_close(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	canStatus ret;

	printf("<< CAN: Close...\n");
	ret = canClose(hCAN[ch]);
	if (ret < 0) return ret;
	hCAN[ch] = 0;
	printf("\t- Done\n");
	Sleep(200);
	return 0;
}

int command_can_sys_init(int ch, int period_msec)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	canStatus ret;

	Txid = ((unsigned long)ID_CMD_SET_PERIOD<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	data[0] = (unsigned char)period_msec;
	ret = canWrite(hCAN[ch], Txid, data, 1, STD);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_MODE_TASK<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canWrite(hCAN[ch], Txid, data, 0, STD);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canWrite(hCAN[ch], Txid, data, 0, STD);

	return 0;
}

int command_can_start(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	canStatus ret;

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canWrite(hCAN[ch], Txid, data, 0, STD);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_ON<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canWrite(hCAN[ch], Txid, data, 0, STD);

	return 0;
}

int command_can_stop(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	canStatus ret;

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_OFF<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canWrite(hCAN[ch], Txid, data, 0, STD);

	return 0;
}

int write_current(int ch, int findex, short* pwm)
{
	assert(ch >= 0 && ch < CH_COUNT);

	long Txid;
	unsigned char data[8];
	canStatus ret;

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
		ret = canWrite(hCAN[ch], Txid, data, 8, STD);
	}
	else
		return -1;
	
	return 0;
}

int get_message(int ch, char* cmd, char* src, char* des, int* len, unsigned char* data, int blocking)
{
	long Rxid;
	unsigned char rdata[8];
	unsigned int dlc;
	unsigned int flag;
	unsigned long time;
	canStatus ret;

	memset(rdata, NULL, sizeof(rdata));
	ret = canRead(hCAN[ch], &Rxid, rdata, &dlc, &flag, &time);
	if (ret != canOK) return ret;
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
