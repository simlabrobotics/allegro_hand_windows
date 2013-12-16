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
#include "Softing/Can_def.h"
#include "Softing/CANL2.h"
}
#include "canDef.h"
#include "canAPI.h"

CANAPI_BEGIN


#define CH_COUNT			(int)2 // number of CAN channels

static CAN_HANDLE hCAN[CH_COUNT] = {-1, -1}; // CAN channel handles



int canWrite(CAN_HANDLE handle,
			 unsigned long id, 
			 void * msg,
			 unsigned int dlc,
			 int mode)
{
	if (handle < 0)
		return -1;

	int ret = CANL2_send_data(handle, id, mode, dlc, (unsigned char*)msg);
	if (ret)
	{
		printf("CAN write error\n");
		return ret;
	}

	return 0;
}

int command_can_open(int ch)
{
	assert(ch >= 1 && ch <= CH_COUNT);

	int ret = 0;
	
	////////////////////////////////////////////////////////////////////////
	// Init Channel
	char ch_name[256];
	sprintf_s(ch_name, 256, "CAN-ACx-PCI_%d", ch);
	printf("Open CAN channel: %s...\n", ch_name);

	ret = INIL2_initialize_channel(&hCAN[ch-1], ch_name);
	if (ret)
	{
		printf("\tError: CAN open\n");
		return ret;
	}

	///////////////////////////////////////////////////////////////////////
	// Reset Chip
//	ret = CANL2_reset_chip(hCAN[ch-1]);
//	if (ret)
//	{
//		printf("\tError: CAN reset chip\n");
//		INIL2_close_channel(hCAN[ch-1]);
//		hCAN[ch-1] = 0;
//		return ret;
//	}

	///////////////////////////////////////////////////////////////////////
	// Init Chip
//	ret = CANL2_initialize_chip(hCAN[ch-1], 1, 1, 4, 3, 0);
//	if (ret)
//	{
//		printf("\tError: CAN set baud rate\n");
//		INIL2_close_channel(hCAN[ch-1]);
//		hCAN[ch-1] = 0;
//		return ret;
//	}
	
	///////////////////////////////////////////////////////////////////////
	// Set Acceptance
//	ret = CANL2_set_acceptance(hCAN[ch-1], m_id, 0x7ff, m_id, 0x1fffffff);
//	if (ret)
//	{
//		printf("\tError: CAN set acceptance\n");
//		INIL2_close_channel(hCAN[ch-1]);
//		hCAN[ch-1] = 0;
//		return ret;
//	}
	
	///////////////////////////////////////////////////////////////////////
	// Set Out Control
//	ret = CANL2_set_output_control(hCAN[ch-1], -1);

	///////////////////////////////////////////////////////////////////////
	// Enable FIFO
	L2CONFIG L2Config;
	L2Config.fBaudrate = 1000.0;
	L2Config.bEnableAck = 0;
	L2Config.bEnableErrorframe = 0;
	L2Config.s32AccCodeStd = 0;
	L2Config.s32AccMaskStd = 0;
	L2Config.s32AccCodeXtd = 0;
	L2Config.s32AccMaskXtd = 0;
	L2Config.s32OutputCtrl = GET_FROM_SCIM;
	L2Config.s32Prescaler = 1;
	L2Config.s32Sam = 0;
	L2Config.s32Sjw = 1;
	L2Config.s32Tseg1 = 4;
	L2Config.s32Tseg2 = 3;
	L2Config.hEvent = (void*)-1;
	ret = CANL2_initialize_fifo_mode(hCAN[ch-1], &L2Config);
	if (ret)
	{
		printf("\tError: CAN set fifo mode\n");
		INIL2_close_channel(hCAN[ch-1]);
		hCAN[ch-1] = 0;
		return ret;
	}

	return 0;
}

int command_can_reset(int ch)
{
	return -1;
}

int command_can_close(int ch)
{
	INIL2_close_channel(hCAN[ch-1]);
	hCAN[ch-1] = 0;
	return 0;
}

int command_can_sys_init(int ch, int period_msec)
{
	long Txid;
	unsigned char data[8];

	Txid = ((unsigned long)ID_CMD_SET_PERIOD<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	data[0] = (unsigned char)period_msec;
	canWrite(hCAN[ch-1], Txid, data, 1, STD);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_MODE_TASK<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(hCAN[ch-1], Txid, data, 0, STD);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(hCAN[ch-1], Txid, data, 0, STD);

	return 0;
}

int command_can_start(int ch)
{
	long Txid;
	unsigned char data[8];

	Txid = ((unsigned long)ID_CMD_QUERY_STATE_DATA<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(hCAN[ch-1], Txid, data, 0, STD);

	Sleep(10);

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_ON<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(hCAN[ch-1], Txid, data, 0, STD);

	return 0;
}

int command_can_stop(int ch)
{
	long Txid;
	unsigned char data[8];

	Txid = ((unsigned long)ID_CMD_SET_SYSTEM_OFF<<6) | ((unsigned long)ID_COMMON <<3) | ((unsigned long)ID_DEVICE_MAIN);
	canWrite(hCAN[ch-1], Txid, data, 0, STD);

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
		canWrite(hCAN[ch-1], Txid, data, 8, STD);
	}
	else
		return -1;
	
	return 0;
}

int get_message(int ch, char* cmd, char* src, char* des, int* len, unsigned char* data, int blocking)
{
	int ret;
	can_msg msg;
	PARAM_STRUCT param;
	
	ret = CANL2_read_ac(hCAN[ch-1], &param);

	switch (ret)
	{
	case CANL2_RA_XTD_DATAFRAME :
		msg.msg_id = param.Ident;
		msg.STD_EXT = EXT;
		msg.data_length = param.DataLength;
		
		msg.data[0] = param.RCV_data[0];
		msg.data[1] = param.RCV_data[1];
		msg.data[2] = param.RCV_data[2];
		msg.data[3] = param.RCV_data[3];
		msg.data[4] = param.RCV_data[4];
		msg.data[5] = param.RCV_data[5];
		msg.data[6] = param.RCV_data[6];
		msg.data[7] = param.RCV_data[7];

		break;

	case CANL2_RA_DATAFRAME:

		msg.msg_id = param.Ident;
		msg.STD_EXT = STD;
		msg.data_length = param.DataLength;
		
		msg.data[0] = param.RCV_data[0];
		msg.data[1] = param.RCV_data[1];
		msg.data[2] = param.RCV_data[2];
		msg.data[3] = param.RCV_data[3];
		msg.data[4] = param.RCV_data[4];
		msg.data[5] = param.RCV_data[5];
		msg.data[6] = param.RCV_data[6];
		msg.data[7] = param.RCV_data[7];

		break;

	case CANL2_RA_NO_DATA:
		return -1;

	default:
		return -1;
	}

	*cmd = (char)( (msg.msg_id >> 6) & 0x1f );
	*des = (char)( (msg.msg_id >> 3) & 0x07 );
	*src = (char)( msg.msg_id & 0x07 );
	*len = (int)( msg.data_length );
	for(int nd=0; nd<(*len); nd++) data[nd] = msg.data[nd];

	return 0;
}



CANAPI_END
