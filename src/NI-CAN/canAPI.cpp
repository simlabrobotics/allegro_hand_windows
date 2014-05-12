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
//project headers
extern "C" {
#include "NI-CAN/nican.h"
}
#include "canDef.h"
#include "canAPI.h"

CANAPI_BEGIN


#define CH_COUNT			(int)1 // number of CAN channels


int hd[CH_COUNT], ret_c;
long id;
unsigned char sdata[8], rdata[8];
unsigned int dlc, flags;
unsigned long timestamp;


#define canMSG_MASK             0x00ff      // Used to mask the non-info bits
#define canMSG_RTR              0x0001      // Message is a remote request
#define canMSG_STD              0x0002      // Message has a standard ID
#define canMSG_EXT              0x0004      // Message has an extended ID
#define canMSG_WAKEUP           0x0008      // Message to be sent / was received in wakeup mode
#define canMSG_NERR             0x0010      // NERR was active during the message
#define canMSG_ERROR_FRAME      0x0020      // Message is an error frame
#define canMSG_TXACK            0x0040      // Message is a TX ACK (msg is really sent)
#define canMSG_TXRQ             0x0080      // Message is a TX REQUEST (msg is transfered to the chip)



//#define _DUMP_RXFRAME (1)

/* NI-CAN Status    */
NCTYPE_STATUS Status=0; 

/* NI-CAN handles */
NCTYPE_OBJH TxHandle=0;

/* NI-CAN Frame Type for Write Object */ 
NCTYPE_CAN_DATA	Transmit;

/* NI-CAN Frame Type for Write Object */ 
NCTYPE_CAN_FRAME TxFrame;

/* NI-CAN Frame Type for Read Object */ 
NCTYPE_CAN_STRUCT RxFrame;

/* This function converts the absolute time obtained from ncReadMult into a
   string. */
void AbsTimeToString(NCTYPE_ABS_TIME *time, char *TimeString)
{

   SYSTEMTIME	stime;
   FILETIME		localftime;

   FileTimeToLocalFileTime((FILETIME *)(time), &localftime);
   FileTimeToSystemTime(&localftime, &stime);
	  sprintf(TimeString, "%02d:%02d:%02d.%03d",
            stime.wHour, stime.wMinute, stime.wSecond, 
            stime.wMilliseconds);
}

/* Print a description of an NI-CAN error/warning. */
void PrintStat(NCTYPE_STATUS Status, char *source) 
{
	char StatusString[1024];
     
	if (Status != 0) 
	{
		ncStatusToString(Status, sizeof(StatusString), StatusString);
		printf("\n%s\nSource = %s\n", StatusString, source);

		// On error, close object handle.
		printf("<< CAN: Close\n");
		ncCloseObject(TxHandle);
		TxHandle = 0;
		hd[0] = TxHandle;
		//hd[1] = TxHandle;
		//exit(1);
	}
}

/* Print read frame */
void PrintRxFrame()
{
	char output[15];
	char CharBuff[50];
	AbsTimeToString(&RxFrame.Timestamp, &output[0]);
	printf("%s     ", output);
	sprintf (&CharBuff[0], "%8.8X", RxFrame.ArbitrationId);
	printf("%s     ", CharBuff);
	sprintf (&CharBuff[0], "%s","CAN Data Frame");
	printf("%s     ", CharBuff); 
	sprintf (&CharBuff[0], "%1d", RxFrame.DataLength);
	printf("%s     ", CharBuff); 
	for (int j=0; j<RxFrame.DataLength; j++)
	{
		sprintf (CharBuff, " %02X", RxFrame.Data[j]);
		printf("%s", CharBuff); 
	}
	printf("\n");
}

int canWrite(int handle,
			 long id, 
			 void * msg,
			 unsigned int dlc,
			 unsigned int /*flag*/)
{
	if (!handle)
		return -1;

	TxFrame.IsRemote = NC_FRMTYPE_DATA;
	TxFrame.ArbitrationId = id;
	TxFrame.DataLength = dlc;
	for (int i=0; i<dlc; i++)
		TxFrame.Data[i] = ((NCTYPE_UINT8_P)msg)[i];
	Status = ncWrite(handle, sizeof(NCTYPE_CAN_FRAME), &TxFrame);
	if (Status < 0)
	{
		PrintStat(Status, "ncWrite");
		return Status;
	}
	return Status;
}

int canRead(int handle,
			long * id,
			void * msg,
			unsigned int * dlc,
			unsigned int * /*flag*/,
			unsigned long * time)
{
	if (!handle)
		return -1;

	memset(&RxFrame, 0, sizeof(NCTYPE_CAN_FRAME));
	//RxFrame.FrameType = NC_FRMTYPE_DATA;
	RxFrame.DataLength = *dlc;
	Status = ncRead(handle, sizeof(NCTYPE_CAN_STRUCT), &RxFrame);
	if (Status < 0 || NC_FRMTYPE_DATA != RxFrame.FrameType)
	{
		PrintStat(Status, "ncRead");
		return Status;
	}
	else if (Status == CanWarnOldData)
	{
		(*dlc) = 0;
		return Status;
	}

	(*id) = RxFrame.ArbitrationId;
	//(*time) = (FILETIME)(RxFrame.Timestamp);
	(*dlc) = RxFrame.DataLength;
	for (int i=0; i<RxFrame.DataLength; i++)
		((NCTYPE_UINT8_P)msg)[i] = RxFrame.Data[i];
#ifdef _DUMP_RXFRAME
	PrintRxFrame();
#endif
	return Status;
}

int canReadWait(int handle,
				long * id,
				void * msg,
				unsigned int * dlc,
				unsigned int * /*flag*/,
				unsigned long * time,
				unsigned long timeout)
{
	if (!handle)
		return -1;

	NCTYPE_STATE currentState;
	Status = ncWaitForState(handle, NC_ST_READ_AVAIL, timeout, &currentState);
	if (Status < 0)
	{
		PrintStat(Status, "ncWaitForState");
		return Status;
	}

	memset(&RxFrame, 0, sizeof(NCTYPE_CAN_FRAME));
	RxFrame.FrameType = NC_FRMTYPE_DATA;
	RxFrame.DataLength = *dlc;
	Status = ncRead(handle, sizeof(NCTYPE_CAN_STRUCT), &RxFrame);
	if (Status < 0)
	{
		PrintStat(Status, "ncRead");
		return Status;
	}

	(*id) = RxFrame.ArbitrationId;
	//(*time) = (FILETIME)(RxFrame.Timestamp);
	(*dlc) = RxFrame.DataLength;
	for (int i=0; i<RxFrame.DataLength; i++)
		((NCTYPE_UINT8_P)msg)[i] = RxFrame.Data[i];
#ifdef _DUMP_RXFRAME
	PrintRxFrame();
#endif
	return Status;
}


int command_can_open(int ch)
{
	NCTYPE_ATTRID		AttrIdList[8];
	NCTYPE_UINT32		AttrValueList[8];
	//NCTYPE_UINT32		Baudrate = 125000;  // BAUD_125K
	NCTYPE_UINT32		Baudrate = NC_BAUD_1000K;
	char				Interface[15];
	
	sprintf_s(Interface, "CAN%d", ch);
	
	// Configure the CAN Network Interface Object	
	AttrIdList[0] =     NC_ATTR_BAUD_RATE;   
	AttrValueList[0] =  Baudrate;
	AttrIdList[1] =     NC_ATTR_START_ON_OPEN;
	AttrValueList[1] =  NC_TRUE;
	AttrIdList[2] =     NC_ATTR_READ_Q_LEN;
	AttrValueList[2] =  100;
	AttrIdList[3] =     NC_ATTR_WRITE_Q_LEN;
	AttrValueList[3] =  10;	
	AttrIdList[4] =     NC_ATTR_CAN_COMP_STD;
	AttrValueList[4] =  0;//0xCFFFFFFF;
	AttrIdList[5] =     NC_ATTR_CAN_MASK_STD;
	AttrValueList[5] =  NC_CAN_MASK_STD_DONTCARE;
	AttrIdList[6] =     NC_ATTR_CAN_COMP_XTD;
	AttrValueList[6] =  0;//0xCFFFFFFF;
	AttrIdList[7] =     NC_ATTR_CAN_MASK_XTD;
	AttrValueList[7] =  NC_CAN_MASK_XTD_DONTCARE;

	
	printf("<< CAN: Config\n");
	Status = ncConfig(Interface, 2, AttrIdList, AttrValueList);
	if (Status < 0) 
	{
		PrintStat(Status, "ncConfig");
		return Status;
	}
	printf("   - Done\n");
    
	// open the CAN Network Interface Object
	printf("<< CAN: Open Channel\n");
	Status = ncOpenObject (Interface, &TxHandle);
	if (Status < 0) 
	{
		PrintStat(Status, "ncOpenObject");
		return Status;
	}
	hd[0] = TxHandle;
	//hd[1] = TxHandle;
	printf("   - Done\n");
	return 1;
}

int command_can_open_ex(int ch, int type, int index)
{
	return command_can_open(ch);
}

int command_can_reset(int ch)
{
	printf("<< CAN: Reset Bus\n");

	// stop the CAN Network
	Status = ncAction(TxHandle, NC_OP_STOP, 0);
	if (Status < 0) 
	{
		PrintStat(Status, "ncAction(NC_OP_STOP)");
		return Status;
	}
	// start the CAN Network
	Status = ncAction(TxHandle, NC_OP_START, 0);
	if (Status < 0) 
	{
		PrintStat(Status, "ncAction(NC_OP_START)");
		return Status;
	}

	/*while (true)
	{
		memset(rdata, NULL, sizeof(rdata));
		Status = canRead(TxHandle, &id, rdata, &dlc, &flags, &timestamp);
		if (Status != 0) break;
		printf("    %ld+%ld (%d)", id-id%128, id%128, dlc);
		for(int nd=0; nd<(int)dlc; nd++) printf(" %3d ", rdata[nd]);
		printf("\n");
	}*/

	/*int nc;

	printf("<< CAN: Reset Bus\n");
	for(nc=0; nc<CH_COUNT; nc++) {
		ret_c = canResetBus(hd[nc]);
		if (ret_c < 0) return ret_c;
		printf("    Ch.%2d (OK)\n", nc, ret_c);
	}*/
	printf("   - Done\n");
	Sleep(200);
	return 1;
}

int command_can_close(int ch)
{
	if (!TxHandle)
		return 0;

	printf("<< CAN: Close\n");
	Status = ncCloseObject(TxHandle);    
	if (Status < 0)
	{
		PrintStat(Status, "ncCloseObject");
		return Status;
	}
	TxHandle = 0;
	hd[0] = TxHandle;
	//hd[1] = TxHandle;
	printf("   - Done\n");
	return 1;
}

int command_can_query_id(int ch)
{
	long Txid;
	unsigned char data[8];

	Txid = ((unsigned long)ID_CMD_QUERY_ID<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(TxHandle, Txid, data, 0, STD);

	return 0;
}

int command_can_sys_init(int ch, int period_msec)
{
	long Txid;
	unsigned char data[8];

	Txid = ((unsigned long)ID_CMD_SET_PERIOD<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	data[0] = (unsigned char)period_msec;
	canWrite(TxHandle, Txid, data, 1, STD);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_MODE_TASK<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(TxHandle, Txid, data, 0, STD);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(TxHandle, Txid, data, 0, STD);

	return 0;
}

int command_can_start(int ch)
{
	long Txid;
	unsigned char data[8];

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(TxHandle, Txid, data, 0, STD);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_ON<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(TxHandle, Txid, data, 0, STD);

	return 0;
}

int command_can_stop(int ch)
{
	long Txid;
	unsigned char data[8];

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_OFF<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(TxHandle, Txid, data, 0, STD);

	return 0;
}

int command_can_AHRS_set(int ch, unsigned char rate, unsigned char mask)
{
	long Txid;
	unsigned char data[8];

	Txid = ((unsigned long)ID_CMD_AHRS_SET<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	data[0] = (unsigned char)rate;
	data[1] = (unsigned char)mask;
	canWrite(TxHandle, Txid, data, 2, STD);

	return 0;
}

int write_current(int ch, int findex, short* pwm)
{
	long Txid;
	unsigned char data[8];

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
		canWrite(TxHandle, Txid, data, 8, STD);
	}
	else
		return -1;
	
	return 0;
}

int get_message(int ch, char* cmd, char* src, char* des, int* len, unsigned char* data, int blocking)
{
	long id;

	memset(rdata, NULL, sizeof(rdata));
	Status = canRead(TxHandle, &id, rdata, &dlc, &flags, &timestamp);
	if (Status != 0) return Status;
	//printf("    %ld+%ld (%d)", id-id%128, id%128, dlc);
	//for(int nd=0; nd<(int)dlc; nd++) printf(" %3d ", rdata[nd]);
	//printf("\n");

	*cmd = (char)( (id >> 6) & 0x1f );
	*des = (char)( (id >> 3) & 0x07 );
	*src = (char)( id & 0x07);
	*len = (int)dlc;
	for(int nd=0; nd<(int)dlc; nd++) data[nd] = rdata[nd];

	return 0;
}



CANAPI_END
