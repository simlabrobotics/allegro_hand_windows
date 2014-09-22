/****************************************************************************
 *
 *                EasySYNC USBCANPlusDllF.h - USB CAN Plus DLL header file
 *
 ****************************************************************************
 * FileName:        USBCANPlusDllF.h
 * Dependencies:    See INCLUDES section below
 * Device:          USBCANPlus - USB2-F-7001, USB2-F-7101
 * Compiler:        Microsoft Visual C++ 2005, 2008
 * Company:         EasySYNC Ltd.
 *
 * Software License Agreement
 *
 * The software supplied herewith by EasySYNC Ltd.
 * (the “Company”) for its USBCANPlus product is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on EasySYNC USBCANPlus products. The software is owned by the
 * Company and/or its supplier, and is protected under applicable copyright
 * laws. All rights are reserved. Any use in violation of the foregoing 
 * restrictions may subject the user to criminal sanctions under applicable 
 * laws, as well as to civil liability for the breach of the terms and 
 * conditions of this license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * Author                Date         Rev             Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Gurinder Singh       05/14/09      1.0             Original.
 * Gurinder Singh       07/14/09      1.1             Added CALLBACK API, s command fix -  array overwrite issue 
 * Bob Recny		11/29/09      1.2             Supports USB2-F-7001 and USB2-F-7101
 *                                                    Updated text descriptions to indicate EasySYNC
 *****************************************************************************************************************/


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the USBCANPLUSDLLF_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// USBCANPLUSDLLF_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifdef USBCANPLUSDLLF_EXPORTS
#define USBCANPLUS_API __declspec(dllexport) 
#else
#define USBCANPLUS_API __declspec(dllimport) 
#endif

//#ifdef USBCANPLUSDLLF_EXPORTS
//#define USBCANPLUS_API __declspec(dllexport) WINAPI
//#else
//#define USBCANPLUS_API __declspec(dllimport) WINAPI
//#endif
 


typedef long CANHANDLE;

typedef enum CHANNEL_STATE {INIT, OPEN, CLOSE};
typedef struct
{
	char serialNumber[9];
	CANHANDLE hnd;
	CHANNEL_STATE ch_state;
} DevInfo;

typedef int	CAN_STATUS;

#define ERROR_CANPLUS_OK	1

#define ERROR_CANPLUS_FAIL	-1
//#define ERROR_CANPLUS_GENERAL -1
#define ERROR_CANPLUS_OPEN_SUBSYSTEM -2
#define ERROR_CANPLUS_COMMAND_SUBSYSTEM -3
#define ERROR_CANPLUS_NOT_OPEN -4
#define ERROR_CANPLUS_TX_FIFO_FULL -5
#define ERROR_CANPLUS_INVALID_PARAM -6
#define ERROR_CANPLUS_NO_MESSAGE -7
#define ERROR_CANPLUS_MEMORY_ERROR -8
#define ERROR_CANPLUS_NO_DEVICE -9
#define ERROR_CANPLUS_TIMEOUT -10
#define ERROR_CANPLUS_INVALID_HARDWARE -11

// Message Flags
#define CANMSG_EXTENDED 0x80	// Extended CAN Id
#define CANMSG_RTR		0x40	// Remote frame

#define CANPLUS_FLAG_TIMESTAMP 0x10000000
#define CANPLUS_FLAG_QUEUE_REPLACE 0x20000000
#define CANPLUS_FLAG_BLOCK 0x40000000
#define CANPLUS_FLAG_SLOW 0x80000000
#define CANPLUS_FLAG_NO_LOCAL_SEND 0x01000000

// Status Bits
#define CANSTATUS_EWARN 0x01
#define CANSTATUS_RXWARN 0x02
#define CANSTATUS_TXWARN 0x04
#define CANSTATUS_RXBP 0x08
#define CANSTATUS_TXBP 0x10
#define CANSTATUS_TXBO 0x20
#define CANSTATUS_RXB1OVFL 0x40
#define CANSTATUS_RXB0OVFL 0x80

// Flushflag bits

#define FLUSH_WAIT 0x00
#define FLUSH_DONTWAIT 0x01
#define FLUSH_EMPTY_INQUEUE 0x02

// CAN Frame
typedef struct {
    unsigned long id;        // Message id
    unsigned long timestamp; // timestamp in milliseconds	
    unsigned char flags;     // [extended_id|1][RTR:1][reserver:6]
    unsigned char len;       // Frame size (0.8)
    unsigned char data[8]; // Databytes 0..7
} CANMsg;


// Alternative CAN Frame
typedef struct {
	unsigned long id;        // Message id
	unsigned long timestamp; // timestamp in milliseconds
	unsigned char flags;     // [extended_id|1][RTR:1][reserver:6]
	unsigned char len;       // Frame size (0.8)
} CANMsgEx;

typedef unsigned char CANDATA;

#ifdef __cplusplus 
extern "C" {
#endif

	USBCANPLUS_API 
	CANHANDLE __stdcall canplus_Open(
		const char* szID,
		const char* szBitrate,
		const char* acceptance_code,
		const char* acceptance_mask,
		unsigned long flags
	);

	USBCANPLUS_API  
	CAN_STATUS __stdcall canplus_Close(
		CANHANDLE handle
	);

	USBCANPLUS_API 
	CAN_STATUS __stdcall canplus_getFirstAdapter(
		char *szAdapter,
		int size
	);
	
	
	USBCANPLUS_API
	CAN_STATUS __stdcall canplus_getNextAdapter(
		char *szAdapter,
		int size
	);
	
	USBCANPLUS_API
	CAN_STATUS __stdcall canplus_Read(
		CANHANDLE handle,
		CANMsg *msg
	);
	USBCANPLUS_API
	CAN_STATUS __stdcall canplus_ReadN(
		CANHANDLE handle,
		CANMsg *msg
	);

//	// To be Done in PRO release
//	USBCANPLUS_API
//	CAN_STATUS canplus_ReadEx(
//		CANHANDLE handle,
//		CANMsgEx *msg
////		CANDATA *pdata
//	);

	////To Be done in PRO release
	//USBCANPLUS_API
	//CAN_STATUS canplus_ReadFirst(
	//    CANHANDLE handle,
	//	unsigned int selectid,
	//	unsigned char selectflags,
	//	CANMsg *msg
	//);

	////To Be done in PRO release
	//USBCANPLUS_API
	//CAN_STATUS canplus_ReadFirstEx(
	//    CANHANDLE handle,
	//	unsigned int selectid,
	//	unsigned char selectflags,
	//	CANMsgEx *msg
	//	// CANDATA *pdata	
	//);

	USBCANPLUS_API
	CAN_STATUS __stdcall canplus_Write(
	    CANHANDLE handle,
		CANMsg *msg
	);
	
	////To Be done in PRO release
	//USBCANPLUS_API
	//CAN_STATUS canplus_WriteEx(
	//    CANHANDLE handle,
	//	CANMsgEx *msg
	//	// CANDATA *pdata
	//);
	
	USBCANPLUS_API
	int __stdcall canplus_Status(
	    CANHANDLE handle
	);
	
	USBCANPLUS_API
	CAN_STATUS __stdcall canplus_VersionInfo(
	    CANHANDLE handle,
		char* verinfo
	);
	
	USBCANPLUS_API
	CAN_STATUS __stdcall canplus_Flush(
	    CANHANDLE handle
	);
	
	USBCANPLUS_API
	CAN_STATUS __stdcall canplus_Reset(
	    CANHANDLE handle
	);
	
    USBCANPLUS_API
    CAN_STATUS __stdcall canplus_Listen(
	     CANHANDLE handle
	);
	
	USBCANPLUS_API
	CAN_STATUS __stdcall canplus_SetTimeouts(
	    CANHANDLE handle,
		unsigned int receiveTimeout,
		unsigned int transmitTimeout
	);
	
	typedef void (__stdcall *LPFNDLL_RECEIVE_CALLBACK)(CANMsg*);
	
	USBCANPLUS_API
	CAN_STATUS __stdcall canplus_setReceiveCallBack(
	    CANHANDLE handle,
		LPFNDLL_RECEIVE_CALLBACK cbfn
	);
	
#ifdef __cplusplus
}
#endif

/*
class USBCANPLUSDLLF_API CUSBCanPlusDllF {
public:
	CUSBCanPlusDllF(void);
	// TODO: add your methods here.
};

extern USBCANPLUSDLLF_API int nUSBCanPlusDllF;

USBCANPLUSDLLF_API int fnUSBCanPlusDllF(void);
*/