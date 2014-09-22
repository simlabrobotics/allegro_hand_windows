

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
#else
#include <windows.h>
#endif
#include <malloc.h>
#include <assert.h>
//project headers
extern "C" {
#include "EasySYNC/USBCanPlusDllF.h"
}
#include "canDef.h"
#include "canAPI.h"


CANAPI_BEGIN

/*=========================================*/
/*       Global file-scope variables       */
/*=========================================*/

CANHANDLE canDev[MAX_BUS] = { 0, };

/*==========================================*/
/*       Private functions prototypes       */
/*==========================================*/
int canReadMsg(CANHANDLE h, int *id, int *len, unsigned char *data, int blocking);
int canSendMsg(CANHANDLE h, int id, char len, unsigned char *data, int blocking);

/*========================================*/
/*       Public functions (CAN API)       */
/*========================================*/
CANHANDLE initCAN(int bus){
	char szAdapter[10];
	const char szBitrate[] = "1000";
	const char szAcceptanceCode[] = "000"; 
	const char szAcceptanceMask[] = "000"; 
	int numAdapters;
	CANHANDLE handle;
	CAN_STATUS status = canplus_getFirstAdapter(szAdapter, 10);
	if (status <= 0) {
		printf("initCAN(): canplus_getFirstAdapter() failed with error %ld\n", status);
		switch (status) {
			case ERROR_CANPLUS_MEMORY_ERROR:		printf("szAdapter buffer size (size variable) is less than 9Bytes.");
			case ERROR_CANPLUS_FAIL:				printf("Unable to get information about adapters.");
			case ERROR_CANPLUS_NO_DEVICE:			printf("No devices found.");
			default: printf("Unknown error");
		}
		return -1;
	}
	numAdapters = status;
	printf("initCAN(): Number of CanPlus Adapters %d\n", numAdapters);
	printf("initCAN(): Serial number of first adapter %s\n", szAdapter);

	while (bus > 0) {
		status = canplus_getNextAdapter(szAdapter, 10);
		if (status <= 0) {
			printf("initCAN(): canplus_getNextAdapter() failed with error %ld\n", status);
			switch (status) {
				case ERROR_CANPLUS_MEMORY_ERROR:		printf("szAdapter buffer size (size variable) is less than 9Bytes.");
				case ERROR_CANPLUS_FAIL:				printf("canplus_getFirstAdapter was not called.");
				case ERROR_CANPLUS_NO_DEVICE:			printf("No devices found.");
				default: printf("Unknown error");
			}
			return -1;
		}
		printf("initCAN(): Serial number of next adapter %s\n", szAdapter);
		bus--;
	}

	handle = canplus_Open(
		szAdapter, //NULL: select the first USB2-F-7x01 adapter. 
		szBitrate, 
		NULL/*szAcceptanceCode*/, //11-bit (0x000 to 0x7FF) or 29-bit (0x00000000 to 0x1FFFFFFF) code used in filtering specific or ranges of CAN messages.
		NULL/*szAcceptanceMask*/, //11-bit (0x000 to 0x7FF) or 29-bit (0x00000000 to 0x1FFFFFFF) code used in filtering specific or ranges of CAN messages
		0x0 // CANPLUS_FLAG_TIMESTAMP: The timestamp function will be enabled by the USB2-F-7x01
		);
	if (handle <= 0) {
		printf("initCAN(): canplus_Open() failed with error %ld\n", handle);
		switch (status) {
			case ERROR_CANPLUS_FAIL:				printf("Unable to open communication to USB2-F-7x01 device.");
			case ERROR_CANPLUS_OPEN_SUBSYSTEM:		printf("Unable to open CAN channel.");
			case ERROR_CANPLUS_COMMAND_SUBSYSTEM:	printf("Failed in setting other parameters (e.g. setting timestamp, bitrate, acceptance code or acceptance mask)");
			default: printf("Unknown error");
		}
		return -1;
	}

	status = canplus_SetTimeouts(handle, 1, 1);
	if (status <= 0) {
		printf("initCAN(): canplus_SetTimeouts() failed with error %ld\n", status);
		switch (status) {
			case ERROR_CANPLUS_FAIL:				printf("CAN channel timeouts could not be configured.");
			default: printf("Unknown error");
		}
		return -1;
	}

	return handle;
}

int freeCAN(CANHANDLE h){
	CAN_STATUS status;

	status = canplus_Close(h);
	if (status <= 0) {
		printf("freeCAN(): canplus_Close() failed with error %ld\n", status);
		switch (status) {
			case ERROR_CANPLUS_FAIL:				printf("CAN channel could not be closed.");
			default: printf("Unknown error");
		}
		return -1;
	}

	return 0;
}

int resetCAN(CANHANDLE h){
	CAN_STATUS status;

	status = canplus_Reset(h);
	if (status <= 0) {
		printf("resetCAN(): canplus_Reset() failed with error %ld\n", status);
		switch (status) {
			case ERROR_CANPLUS_FAIL:				printf("CAN channel could not be reset.");
			default: printf("Unknown error");
		}
		return -1;
	}

	return 0;
}

int canReadMsg(CANHANDLE h, int *id, int *len, unsigned char *data, int blocking){
	CANMsg msg;
	CAN_STATUS status;
	int i;

	// We execute the "Read" function
	//
	status = canplus_Read(h, &msg);
	if (status == ERROR_CANPLUS_NO_MESSAGE) {
		//printf("canReadMsg(): The receive buffer is empty.\n");
		return 0;
	}
	else if (status < 0) {
		printf("canReadMsg(): canplus_Read() failed with error %ld\n", status);
		switch (status) {
			default: printf("Unknown error");
		}
		return status;
	}

	*id = msg.id;
	*len = msg.len;
	for(i = 0; i < msg.len; i++)
		data[i] = msg.data[i];

	return 0;
}

int canSendMsg(CANHANDLE h, int id, char len, unsigned char *data, int blocking){
	CANMsg msg;
	CAN_STATUS status;
	int i;

	msg.id = id;
	msg.len = len & 0x0F;
	for(i = 0; i < len; i++)
        msg.data[i] = data[i];
	msg.flags = 0x0; // CANMSG_EXTENDED: 0x80, CANMSG_RTR: 0x40

	status = canplus_Write(h, &msg);
	if (status <= 0)
	{
		printf("canSendMsg(): canplus_Write() failed with error %ld\n", status);
		switch (status) {
			case ERROR_CANPLUS_FAIL:				printf("Standard/Extended Frame write Failure.");
			default: printf("Unknown error");
		}
		return status;
	}

	return 0;
}

/*========================================*/
/*       CAN API                          */
/*========================================*/
int command_can_open(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	CANHANDLE ret;

	printf("<< CAN: Open Channel...\n");
	ret = initCAN(ch);
	if (ret < 0) return ret;
	canDev[ch] = ret;
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
	assert(ch >= 0 && ch < CH_COUNT);

	printf("<< CAN: Reset...\n");

	int status = resetCAN(canDev[ch]);
	if (status < 0)
		return status;

	printf("\t- Done\n");
	return -1;
}

int command_can_close(int ch)
{
	assert(ch >= 0 && ch < CH_COUNT);

	printf("<< CAN: Close...\n");

	int status = freeCAN(canDev[ch]);
	if (status < 0)
		return status;

	printf("\t- Done\n");
	canDev[ch] = -1;
	return 0;
}

int command_can_query_id(int ch)
{
	assert(ch >= 0 && ch < MAX_BUS);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_QUERY_ID<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(canDev[ch], Txid, 0, data, TRUE);

	return 0;
}

int command_can_sys_init(int ch, int period_msec)
{
	assert(ch >= 0 && ch < MAX_BUS);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_SET_PERIOD<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	data[0] = (unsigned char)period_msec;
	ret = canSendMsg(canDev[ch], Txid, 1, data, TRUE);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_MODE_TASK<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(canDev[ch], Txid, 0, data, TRUE);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(canDev[ch], Txid, 0, data, TRUE);

	return 0;
}

int command_can_start(int ch)
{
	assert(ch >= 0 && ch < MAX_BUS);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(canDev[ch], Txid, 0, data, TRUE);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_ON<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(canDev[ch], Txid, 0, data, TRUE);

	return 0;
}

int command_can_stop(int ch)
{
	assert(ch >= 0 && ch < MAX_BUS);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_OFF<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	ret = canSendMsg(canDev[ch], Txid, 0, data, TRUE);

	return 0;
}

int command_can_AHRS_set(int ch, unsigned char rate, unsigned char mask)
{
	assert(ch >= 0 && ch < MAX_BUS);

	long Txid;
	unsigned char data[8];
	int ret;

	Txid = ((unsigned long)ID_CMD_AHRS_SET<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	data[0] = (unsigned char)rate;
	data[1] = (unsigned char)mask;
	ret = canSendMsg(canDev[ch], Txid, 2, data, TRUE);

	return 0;
}

int write_current(int ch, int findex, short* pwm)
{
	assert(ch >= 0 && ch < MAX_BUS);

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
		ret = canSendMsg(canDev[ch], Txid, 8, data, TRUE);
	}
	else
		return -1;
	
	return 0;
}

int get_message(int ch, char* cmd, char* src, char* des, int* len, unsigned char* data, int /*blocking*/) // non-blocking read is not supported.
{
	int err;
	unsigned long Rxid;

	err = canReadMsg(canDev[ch], (int*)&Rxid, len, data, TRUE);
	if (!err)
	{
		/*printf("    %ld+%ld (%d)", Rxid-Rxid%128, Rxid%128, len);
		for(int nd=0; nd<(*len); nd++) printf(" %3d ", data[nd]);
		printf("\n");*/

		*cmd = (char)( (Rxid >> 6) & 0x1f );
		*des = (char)( (Rxid >> 3) & 0x07 );
		*src = (char)( Rxid & 0x07);
	}
	else
	{
		return err;
	}
	return 0;
}



CANAPI_END
