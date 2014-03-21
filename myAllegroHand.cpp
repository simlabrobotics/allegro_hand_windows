// myAllegroHand.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <conio.h>
#include <process.h>
#include <tchar.h>
#include "canAPI.h"
#include "rDeviceAllegroHandCANDef.h"
#include "rPanelManipulatorCmdUtil.h"
#include "BHand/BHand.h"

///////////////////////////////////////
// for CAN communication
const double delT = 0.003;
int CAN_Ch = 0;
bool ioThreadRun = false;
uintptr_t ioThread = 0;
int recvNum = 0;
int sendNum = 0;
double statTime = -1.0;
AllegroHand_DeviceMemory_t vars;

///////////////////////////////////////
// for rPanelManipulator
rPanelManipulatorData_t* pSHM = NULL;
double curTime = 0.0;

///////////////////////////////////////
// for BHand library
BHand* pBHand = NULL;
double q[MAX_DOF];
double tau_des[MAX_DOF];
double cur_des[MAX_DOF];


// v3.x
const int enc_offset[MAX_DOF] = { // SAH030AR023
	-1700, -568, -3064, -36,
	-2015, -1687, 188, -772,
	-3763, 782, -3402, 368,
	1059, -2547, -692, 2411
};

const double enc_dir[MAX_DOF] = { // v3.0
	1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0
};
const double motor_dir[MAX_DOF] = { // v3.0
	1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0
};


// v2.x
/*
const int enc_offset[MAX_DOF] = { // SAH030AR022
	1485, -13, 211, 234,
	0, 282, 2174, -2291,
	1698, 1782, -2351, 793,
	466, 2397, 2594, 270
};
*/


/*
const int enc_offset[MAX_DOF] = { // SAH020BR013
	-391,	-64387,	-129,	 532,
	 178,	-66030,	-142,	 547,
	-234,	-64916,	 7317,	 1923,
	 1124,	-1319,	-65983, -65566
};


const double enc_dir[MAX_DOF] = { // v2.0
	1.0, -1.0,  1.0,  1.0,
	1.0, -1.0,  1.0,  1.0,
	1.0, -1.0,  1.0,  1.0,
	1.0,  1.0, -1.0, -1.0
};
const double motor_dir[MAX_DOF] = { // v2.0
	 1.0,  1.0,  1.0, 1.0,
	 1.0, -1.0, -1.0, 1.0,
	-1.0,  1.0,  1.0, 1.0,
	 1.0,  1.0,  1.0, 1.0
};
*/




///////////////////////////////////////
// functions
void PrintInstruction();
void MainLoop();
bool OpenCAN();
void CloseCAN();
int GetCANChannelIndex(const TCHAR* cname);
bool CreateBHandAlgorithm();
void DestroyBHandAlgorithm();
void ComputeTorque();

void ComputeTorque()
{
	/*memset(tau_des, 0, sizeof(tau_des));
	tau_des[0] = 200.0 / 800.0 * motor_dir[0];
	return;*/


	if (!pBHand) return;

	pBHand->SetJointPosition(q); // tell BHand library the current joint positions
	pBHand->UpdateControl(0);
	pBHand->GetJointTorque(tau_des);
}

static unsigned int __stdcall ioThreadProc(void* inst)
{
	char id_des;
	char id_cmd;
	char id_src;
	int len;
	unsigned char data[8];
	unsigned char data_return = 0;
	int i;

	while (ioThreadRun)
	{
		while (0 == get_message(CAN_Ch, &id_cmd, &id_src, &id_des, &len, data, FALSE))
		{
			switch (id_cmd)
			{
			case ID_CMD_QUERY_CONTROL_DATA:
				{
					if (id_src >= ID_DEVICE_SUB_01 && id_src <= ID_DEVICE_SUB_04)
					{
						vars.enc_actual[(id_src-ID_DEVICE_SUB_01)*4 + 0] = (int)(data[0] | (data[1] << 8));
						vars.enc_actual[(id_src-ID_DEVICE_SUB_01)*4 + 1] = (int)(data[2] | (data[3] << 8));
						vars.enc_actual[(id_src-ID_DEVICE_SUB_01)*4 + 2] = (int)(data[4] | (data[5] << 8));
						vars.enc_actual[(id_src-ID_DEVICE_SUB_01)*4 + 3] = (int)(data[6] | (data[7] << 8));
						data_return |= (0x01 << (id_src-ID_DEVICE_SUB_01));
						recvNum++;
					}
					if (data_return == (0x01 | 0x02 | 0x04 | 0x08))
					{
						// convert encoder count to joint angle
						for (i=0; i<MAX_DOF; i++)
							q[i] = (double)(vars.enc_actual[i]*enc_dir[i]-32768-enc_offset[i])*(333.3/65536.0)*(3.141592/180.0);

						//printf("%f\n",q[5]);

						// compute joint torque
						ComputeTorque();

						// convert desired torque to desired current and PWM count
						for (i=0; i<MAX_DOF; i++)
						{
							cur_des[i] = tau_des[i] * motor_dir[i];
							if (cur_des[i] > 1.0) cur_des[i] = 1.0;
							else if (cur_des[i] < -1.0) cur_des[i] = -1.0;
						}

						// send torques
						for (int i=0; i<4;i++)
						{
							// the index order for motors is different from that of encoders

							// 2.0
							/*
							vars.pwm_demand[i*4+3] = (short)(cur_des[i*4+0]*800.0);
							vars.pwm_demand[i*4+2] = (short)(cur_des[i*4+1]*800.0);
							vars.pwm_demand[i*4+1] = (short)(cur_des[i*4+2]*800.0);
							vars.pwm_demand[i*4+0] = (short)(cur_des[i*4+3]*800.0);
							*/
							

							// 3.0
							
							vars.pwm_demand[i*4+3] = (short)(cur_des[i*4+0]*1200.0);
							vars.pwm_demand[i*4+2] = (short)(cur_des[i*4+1]*1200.0);
							vars.pwm_demand[i*4+1] = (short)(cur_des[i*4+2]*1200.0);
							vars.pwm_demand[i*4+0] = (short)(cur_des[i*4+3]*1200.0);							
							

							write_current(CAN_Ch, i, &vars.pwm_demand[4*i]);
							for(int k=0; k<100000; k++);
						}
						sendNum++;
						curTime += delT;

						data_return = 0;
					}
				}
				break;
			}
		}
	}

	return 0;
}

void MainLoop()
{
	bool bRun = true;
	int i;

	while (bRun)
	{
		if (!_kbhit())
		{
			Sleep(5);
			if (pSHM)
			{
				switch (pSHM->cmd.command)
				{
				case CMD_SERVO_ON:
					break;
				case CMD_SERVO_OFF:
					if (pBHand) pBHand->SetMotionType(eMotionType_NONE);
					break;
				case CMD_CMD_1:
					if (pBHand) pBHand->SetMotionType(eMotionType_HOME);
					break;
				case CMD_CMD_2:
					if (pBHand) pBHand->SetMotionType(eMotionType_READY);
					break;
				case CMD_CMD_3:
					if (pBHand) pBHand->SetMotionType(eMotionType_GRASP_3);
					break;
				case CMD_CMD_4:
					if (pBHand) pBHand->SetMotionType(eMotionType_GRASP_4);
					break;
				case CMD_CMD_5:
					if (pBHand) pBHand->SetMotionType(eMotionType_PINCH_IT);
					break;
				case CMD_CMD_6:
					if (pBHand) pBHand->SetMotionType(eMotionType_PINCH_MT);
					break;
				case CMD_CMD_7:
					if (pBHand) pBHand->SetMotionType(eMotionType_ENVELOP);
					break;
				case CMD_CMD_8:
					if (pBHand) pBHand->SetMotionType(eMotionType_GRAVITY_COMP);
					break;
				case CMD_EXIT:
					bRun = false;
					break;
				}
				pSHM->cmd.command = CMD_NULL;
				for (i=0; i<MAX_DOF; i++)
				{
					pSHM->state.slave_state[i].position = q[i];
					pSHM->cmd.slave_command[i].torque = tau_des[i];
				}
				pSHM->state.time = curTime;
			}
		}
		else
		{
			int c = _getch();
			switch (c)
			{
			case 'q':
				bRun = false;
				break;
			
			case 'h':
				if (pBHand) pBHand->SetMotionType(eMotionType_HOME);
				break;
			
			case 'r':
				if (pBHand) pBHand->SetMotionType(eMotionType_READY);
				break;
			
			case 'g':
				if (pBHand) pBHand->SetMotionType(eMotionType_GRASP_3);
				break;

			case 'k':
				if (pBHand) pBHand->SetMotionType(eMotionType_GRASP_4);
				break;
			
			case 'p':
				if (pBHand) pBHand->SetMotionType(eMotionType_PINCH_IT);
				break;
			
			case 'm':
				if (pBHand) pBHand->SetMotionType(eMotionType_PINCH_MT);
				break;
			
			case 'a':
				if (pBHand) pBHand->SetMotionType(eMotionType_GRAVITY_COMP);
				break;

			case 'e':
				if (pBHand) pBHand->SetMotionType(eMotionType_ENVELOP);
				break;

			case 'o':
				if (pBHand) pBHand->SetMotionType(eMotionType_NONE);
				break;
			}
		}
	}
}

bool OpenCAN()
{
	int ret;
	
	CAN_Ch = GetCANChannelIndex(_T("USBBUS1"));

	printf(">CAN(%d): open\n", CAN_Ch);
	ret = command_can_open(CAN_Ch);
	if(ret < 0)
	{
		printf("ERROR command_canopen !!! \n");
		return false;
	}

	recvNum = 0;
	sendNum = 0;
	statTime = 0.0;

	ioThreadRun = true;
	ioThread = _beginthreadex(NULL, 0, ioThreadProc, NULL, 0, NULL);
	printf(">CAN: starts listening CAN frames\n");

	printf(">CAN: system init\n");
	ret = command_can_sys_init(CAN_Ch, 3/*msec*/);
	if(ret < 0)
	{
		printf("ERROR command_can_sys_init !!! \n");
		command_can_close(CAN_Ch);
		return false;
	}

	printf(">CAN: start periodic communication\n");
	ret = command_can_start(CAN_Ch);
	if(ret < 0)
	{
		printf("ERROR command_can_start !!! \n");
		command_can_stop(CAN_Ch);
		command_can_close(CAN_Ch);
		return false;
	}

	return true;
}

void CloseCAN()
{
	int ret;

	printf(">CAN: stop periodic communication\n");
	ret = command_can_stop(CAN_Ch);
	if(ret < 0)
	{
		printf("ERROR command_can_stop !!! \n");
	}

	if (ioThreadRun)
	{
		printf(">CAN: stoped listening CAN frames\n");
		ioThreadRun = false;
		WaitForSingleObject((HANDLE)ioThread, INFINITE);
		CloseHandle((HANDLE)ioThread);
		ioThread = 0;
	}

	printf(">CAN(%d): close\n", CAN_Ch);
	ret = command_can_close(CAN_Ch);
	if(ret < 0) printf("ERROR command_can_close !!! \n");
}

void PrintInstruction()
{
	printf("--------------------------------------------------\n");
	printf("myAllegroHand\n");
	printf("Keyboard Commands:\n\n");

	
	printf("H: Home Position (PD control)\n");
	printf("R: Ready Position (used before grasping)\n");	
	printf("G: Three-Finger Grasp\n");
	printf("K: Four-Finger Grasp\n");
	printf("P: Two-finger pinch (index-thumb)\n");
	printf("M: Two-finger pinch (middle-thumb)\n");
	printf("E: Envelop Grasp (all fingers)\n");
	printf("A: Gravity Compensation\n\n");

	printf("O: Servos OFF (any grasp cmd turns them back on)\n");
	printf("Q: Quit this program\n");

	printf("--------------------------------------------------\n\n");
}

int GetCANChannelIndex(const TCHAR* cname)
{
	if (!cname) return 0;

	if (!_tcsicmp(cname, _T("0")) || !_tcsicmp(cname, _T("PCAN_NONEBUS")) || !_tcsicmp(cname, _T("NONEBUS")))
		return 0;
	else if (!_tcsicmp(cname, _T("1")) || !_tcsicmp(cname, _T("PCAN_ISABUS1")) || !_tcsicmp(cname, _T("ISABUS1")))
		return 1;
	else if (!_tcsicmp(cname, _T("2")) || !_tcsicmp(cname, _T("PCAN_ISABUS2")) || !_tcsicmp(cname, _T("ISABUS2")))
		return 2;
	else if (!_tcsicmp(cname, _T("3")) || !_tcsicmp(cname, _T("PCAN_ISABUS3")) || !_tcsicmp(cname, _T("ISABUS3")))
		return 3;
	else if (!_tcsicmp(cname, _T("4")) || !_tcsicmp(cname, _T("PCAN_ISABUS4")) || !_tcsicmp(cname, _T("ISABUS4")))
		return 4;
	else if (!_tcsicmp(cname, _T("5")) || !_tcsicmp(cname, _T("PCAN_ISABUS5")) || !_tcsicmp(cname, _T("ISABUS5")))
		return 5;
	else if (!_tcsicmp(cname, _T("7")) || !_tcsicmp(cname, _T("PCAN_ISABUS6")) || !_tcsicmp(cname, _T("ISABUS6")))
		return 6;
	else if (!_tcsicmp(cname, _T("8")) || !_tcsicmp(cname, _T("PCAN_ISABUS7")) || !_tcsicmp(cname, _T("ISABUS7")))
		return 7;
	else if (!_tcsicmp(cname, _T("8")) || !_tcsicmp(cname, _T("PCAN_ISABUS8")) || !_tcsicmp(cname, _T("ISABUS8")))
		return 8;
	else if (!_tcsicmp(cname, _T("9")) || !_tcsicmp(cname, _T("PCAN_DNGBUS1")) || !_tcsicmp(cname, _T("DNGBUS1")))
		return 9;
	else if (!_tcsicmp(cname, _T("10")) || !_tcsicmp(cname, _T("PCAN_PCIBUS1")) || !_tcsicmp(cname, _T("PCIBUS1")))
		return 10;
	else if (!_tcsicmp(cname, _T("11")) || !_tcsicmp(cname, _T("PCAN_PCIBUS2")) || !_tcsicmp(cname, _T("PCIBUS2")))
		return 11;
	else if (!_tcsicmp(cname, _T("12")) || !_tcsicmp(cname, _T("PCAN_PCIBUS3")) || !_tcsicmp(cname, _T("PCIBUS3")))
		return 12;
	else if (!_tcsicmp(cname, _T("13")) || !_tcsicmp(cname, _T("PCAN_PCIBUS4")) || !_tcsicmp(cname, _T("PCIBUS4")))
		return 13;
	else if (!_tcsicmp(cname, _T("14")) || !_tcsicmp(cname, _T("PCAN_PCIBUS5")) || !_tcsicmp(cname, _T("PCIBUS5")))
		return 14;
	else if (!_tcsicmp(cname, _T("15")) || !_tcsicmp(cname, _T("PCAN_PCIBUS6")) || !_tcsicmp(cname, _T("PCIBUS6")))
		return 15;
	else if (!_tcsicmp(cname, _T("16")) || !_tcsicmp(cname, _T("PCAN_PCIBUS7")) || !_tcsicmp(cname, _T("PCIBUS7")))
		return 16;
	else if (!_tcsicmp(cname, _T("17")) || !_tcsicmp(cname, _T("PCAN_PCIBUS8")) || !_tcsicmp(cname, _T("PCIBUS8")))
		return 17;
	else if (!_tcsicmp(cname, _T("18")) || !_tcsicmp(cname, _T("PCAN_USBBUS1")) || !_tcsicmp(cname, _T("USBBUS1")))
		return 18;
	else if (!_tcsicmp(cname, _T("19")) || !_tcsicmp(cname, _T("PCAN_USBBUS2")) || !_tcsicmp(cname, _T("USBBUS2")))
		return 19;
	else if (!_tcsicmp(cname, _T("20")) || !_tcsicmp(cname, _T("PCAN_USBBUS3")) || !_tcsicmp(cname, _T("USBBUS3")))
		return 20;
	else if (!_tcsicmp(cname, _T("21")) || !_tcsicmp(cname, _T("PCAN_USBBUS4")) || !_tcsicmp(cname, _T("USBBUS4")))
		return 21;
	else if (!_tcsicmp(cname, _T("22")) || !_tcsicmp(cname, _T("PCAN_USBBUS5")) || !_tcsicmp(cname, _T("USBBUS5")))
		return 22;
	else if (!_tcsicmp(cname, _T("23")) || !_tcsicmp(cname, _T("PCAN_USBBUS6")) || !_tcsicmp(cname, _T("USBBUS6")))
		return 23;
	else if (!_tcsicmp(cname, _T("24")) || !_tcsicmp(cname, _T("PCAN_USBBUS7")) || !_tcsicmp(cname, _T("USBBUS7")))
		return 24;
	else if (!_tcsicmp(cname, _T("25")) || !_tcsicmp(cname, _T("PCAN_USBBUS8")) || !_tcsicmp(cname, _T("USBBUS8")))
		return 25;
	else if (!_tcsicmp(cname, _T("26")) || !_tcsicmp(cname, _T("PCAN_PCCBUS1")) || !_tcsicmp(cname, _T("PCCBUS1")))
		return 26;
	else if (!_tcsicmp(cname, _T("27")) || !_tcsicmp(cname, _T("PCAN_PCCBUS2")) || !_tcsicmp(cname, _T("PCCBUS2")))
		return 271;
	else
		return 0;
}

bool CreateBHandAlgorithm()
{
	pBHand = bhCreateRightHand();
	if (!pBHand) return false;
	pBHand->SetTimeInterval(delT);
	return true;
}

void DestroyBHandAlgorithm()
{
	if (pBHand)
	{
		delete pBHand;
		pBHand = NULL;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	PrintInstruction();

	memset(&vars, 0, sizeof(vars));
	memset(q, 0, sizeof(q));
	memset(tau_des, 0, sizeof(tau_des));
	memset(cur_des, 0, sizeof(cur_des));
	curTime = 0.0;

	pSHM = getrPanelManipulatorCmdMemory();
	
	if (CreateBHandAlgorithm() && OpenCAN())
		MainLoop();

	CloseCAN();
	DestroyBHandAlgorithm();
	closerPanelManipulatorCmdMemory();

	return 0;
}
