#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include "ixpci.h"

#define PCI_NoError 0
#define PCI_DriverOpenError 1
#define PCI_DriverNoOpen 2
#define PCI_GetDriverVersionError 3
#define PCI_InstallIrqError 4
#define PCI_ClearIntCountError 5
#define PCI_GetIntCountError 6
#define PCI_RegisterApcError 7
#define PCI_RemoveIrqError 8
#define PCI_FindBoardError 9
#define PCI_ExceedBoardNumber 10
#define PCI_ResetError 11
#define PCI_IrqMaskError 12
#define PCI_ActiveModeError 13
#define PCI_GetActiveFlagError 14
#define PCI_ActiveFlagEndOfQueue 15
#define PCI_BoardIsNotOpen 16
#define PCI_BoardOpenError 17
#define PCI_BoardNoIsZero 18
#define PCI_BoardNoExceedFindBoards 19
#define PCI_InputParameterError 20
#define PCI_IntInitialStateError 21
#define PCI_IntInitialValueError 22
#define PCI_TimeOut 23
#define PCI_WriteXORLogicError 24
#define PCI_ReadXORLogicError 24

/* define driver and dll version */
#define IXPCI_LIBRARY_VERSION 100 	//100(in hexadecimal format) denotes Version 1.00
#define IXPCI_MODULE_NAME
#define LINE_SIZE 128
#define MAX_BOARD_NUMBER 16
#define MAX_CARD_SUPPORT 16
#define MAX_PORT_LEVEL 4
#define LEVEL_A 0
#define LEVEL_B 1
#define LEVEL_C 2
#define LEVEL_D 3

/* define DO,DI for library User */
#define DIGITAL_OUTPUT 0
#define DIGITAL_INPUT  1
#define COUNTER0 0
#define COUNTER1 1

//define trigger mode
#define SOFTWARE_TRIGGER	1
#define PACER_TRIGGER		2
#define EXTERNAL_TRIGGER	3
#define SIGNAL_BASE		60

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef void (*HANDLE)(int);
typedef unsigned char BYTE;
typedef int boolean;

struct port_list {
	WORD ports[6];
};

/* record port config that can be configured by software */
typedef struct port_config {
        WORD init;
	WORD id;
	WORD status;
}port_config_t;

/* implement the function in PIODAQ.c */
typedef struct pci_daq {
	int idx;	//dwBoardNo
        int fd;
        char module_name[32];
        char dev_node[32];
        ixpci_signal_t sig;
        struct sigaction act, act_old;
}pci_daq_t;

#ifdef __cplusplus
extern "C" {
#endif

DWORD PCIDA_DriverInit(WORD *wBoards);
DWORD PCIDA_DetectBoards(char *module_name);
DWORD PCIDA_DetectBoardNo(char *module_name);
DWORD PCIDA_OpenBoard(DWORD dwBoardNo, DWORD dwIntEnable);
DWORD PCIDA_CloseBoard(DWORD dwBoardNo);
DWORD PCIDA_GetDllVersion(void);
DWORD PCIDA_WriteCounter(char *module_name, DWORD dwBoardNo, BYTE Counter, BYTE Mode, DWORD Data);
DWORD PCIDA_ReadCounter(char *module_name, DWORD dwBoardNo, BYTE Counter, DWORD *Data);
DWORD PCIDA_IntLatchCounter(char *module_name, DWORD dwBoardNo, BYTE Counter); //20211216 : To latch CNT in driver's ISR
DWORD PCIDA_EnableInt(DWORD dwBoardNo);
DWORD PCIDA_DisableInt(DWORD dwBoardNo);
DWORD PCIDA_WriteByte(DWORD dwBoardNo, DWORD dwOffset, BYTE Data);
DWORD PCIDA_ReadByte(DWORD dwBoardNo, DWORD dwOffset, BYTE *Data);
DWORD PCIDA_WriteWord(DWORD dwBoardNo, DWORD dwOffset, WORD Data);
DWORD PCIDA_ReadWord(DWORD dwBoardNo, DWORD dwOffset, WORD *Data);
DWORD PCIDA_InstallCallBackFunc(DWORD dwBoardNo, DWORD dwIntType, HANDLE user_function);
DWORD PCIDA_RemoveAllCallBackFunc(DWORD dwBoardNo);
DWORD PCIDA_ResetDevice(DWORD dwBoardNo);
DWORD PCIDA_WriteXORLogic(char *module_name, DWORD dwBoardNo, DWORD Data);
DWORD PCIDA_ReadXORLogic(char *module_name, DWORD dwBoardNo);

/*
WORD PCIDA_IntInstall(WORD, HANDLE, WORD);
WORD PCIDA_IRQEnable(WORD);
WORD PCIDA_IntRemove(WORD);
WORD PCIDA_8254Control(WORD, WORD);
WORD PCIDA_8254C0(WORD, BYTE, BYTE);
WORD PCIDA_8254C1(WORD, BYTE, BYTE);
WORD PCIDA_8254C2(WORD, BYTE, BYTE);
WORD PCIDA_OutputByte(WORD, DWORD, DWORD);
DWORD PCIDA_InputByte(WORD, DWORD);
WORD PCIDA_WriteDigitalOutput(WORD, WORD);
WORD PCIDA_ReadDigitalInput(WORD, WORD*);
WORD PCIDA_ResetDevice(WORD);
*/

/* support PCI-TMC12 card */
#define PTMC12_GetDllVersion() PCIDA_GetDllVersion()
#define PTMC12_DetectBoards() PCIDA_DetectBoards("PCI-TMC12")
#define PTMC12_DetectBoardNo() PCIDA_DetectBoardNo("PCI-TMC12")
#define PTMC12_WriteCounter(dwBoardNo, Counter, Mode, Data) PCIDA_WriteCounter("PCI-TMC12", dwBoardNo, Counter, Mode, Data)
#define PTMC12_ReadCounter(dwBoardNo, Counter, Data) PCIDA_ReadCounter("PCI-TMC12", dwBoardNo, Counter, Data)
#define PTMC12_WriteXORLogic(dwBoardNo, Data) PCIDA_WriteXORLogic("PCI-TMC12", dwBoardNo, Data)
#define PTMC12_ReadXORLogic(dwBoardNo) PCIDA_ReadXORLogic("PCI-TMC12", dwBoardNo)
#define PTMC12_IntLatchCounter(dwBoardNo, Counter) PCIDA_IntLatchCounter("PCI-TMC12", dwBoardNo, Counter)

#define PTMC12_DriverInit(wBoards) PCIDA_DriverInit(wBoards)
#define PTMC12_OpenBoard(dwBoardNo, dwIntEnable) PCIDA_OpenBoard(dwBoardNo, dwIntEnable)
#define PTMC12_CloseBoard(dwBoardNo) PCIDA_CloseBoard(dwBoardNo)
#define PTMC12_EnableInt(dwBoardNo) PCIDA_EnableInt(dwBoardNo)
#define PTMC12_DisableInt(dwBoardNo) PCIDA_DisableInt(dwBoardNo)
#define PTMC12_InstallCallBackFunc(dwBoardNo, dwIntType, user_function) PCIDA_InstallCallBackFunc(dwBoardNo, dwIntType, user_function);
#define PTMC12_RemoveAllCallBackFunc(dwBoardNo) PCIDA_RemoveAllCallBackFunc(dwBoardNo)
#define PTMC12_WriteByte(dwBoardNo, dwOffset, Data) PCIDA_WriteByte(dwBoardNo, dwOffset, Data)
#define PTMC12_ReadByte(dwBoardNo, dwOffset, Data) PCIDA_ReadByte(dwBoardNo, dwOffset, Data)
#define PTMC12_WriteWord(dwBoardNo, dwOffset, Data) PCIDA_WriteWord(dwBoardNo, dwOffset, Data)
#define PTMC12_ReadWord(dwBoardNo, dwOffset, Data) PCIDA_ReadWord(dwBoardNo, dwOffset, Data)
#define PTMC12_ResetDevice(dwBoardNo) PCIDA_ResetDevice(dwBoardNo)

#ifdef __cplusplus
}
#endif

#if 0
20210819 : mark first
int PCIDA_Open(char *dev_file);
WORD PCIDA_Close(WORD fd);
WORD PCIDA_DriverInit(WORD fd);
char* PCIDA_GetDriverVersion(void);
char* PCIDA_GetLibraryVersion(void);
WORD PCIDA_SetGain(WORD, BYTE);
WORD PCIDA_SetTriggerType(WORD, WORD);
WORD PCIDA_ReadAI(WORD, BYTE, WORD*);
WORD PCIDA_SetAIChannel(WORD ,BYTE);
WORD PCIDA_IntInstall(WORD, HANDLE, WORD);
WORD PCIDA_IRQEnable(WORD);
WORD PCIDA_IntRemove(WORD);
WORD PCIDA_8254Control(WORD, WORD);
WORD PCIDA_8254C0(WORD, BYTE, BYTE);
WORD PCIDA_8254C1(WORD, BYTE, BYTE);
WORD PCIDA_8254C2(WORD, BYTE, BYTE);
WORD PCIDA_OutputByte(WORD, DWORD, DWORD);
DWORD PCIDA_InputByte(WORD, DWORD);
WORD PCIDA_WriteDigitalOutput(WORD, WORD);
WORD PCIDA_ReadDigitalInput(WORD, WORD*);
WORD PCIDA_ResetDevice(WORD);
#endif

/* support PCI-1800(H/L) card */
#if 0
20210819 : mark first
WORD PCI_180X_DaF_Output(WORD, WORD, float);
WORD PCI_180X_Da_Output(WORD, WORD, float);
WORD PCI_180X_Digital_Output(WORD, DWORD);
WORD PCI_180X_Digital_Input(WORD, DWORD *); 
WORD PCI_180X_SetChannelConfig(WORD, WORD, WORD);
WORD PCI_180X_DelayUs(WORD, int);
WORD PCI_180X_AdsPolling(WORD, float [], WORD);
WORD PCI_1602_AdsPolling(WORD, float [], WORD);
WORD PCI_180X_AdsPacer(WORD ,float [], WORD, WORD);
WORD PCI_180X_AddToScan(WORD, WORD, WORD, WORD, WORD, WORD, WORD);
WORD PCI_180X_SaveScan(WORD , WORD, WORD []);
WORD PCI_180X_ClearScan(WORD);
WORD PCI_180X_StartScan(WORD, WORD, WORD, int);
void PCI_180X_ReadScanStatus(WORD *, DWORD *, DWORD *);
WORD PCI_180X_StopMagicScan(WORD fd);
WORD PCI_180X_StartScanPostTrg(WORD, WORD, DWORD, int);
WORD PCI_180X_StartScanPreTrg(WORD, WORD, DWORD, int);
WORD PCI_180X_StartScanMiddleTrg(WORD, WORD, DWORD, DWORD, int);
WORD PCI_180X_StartScanPreTrgVerC(WORD, WORD, DWORD, int);
WORD PCI_180X_StartScanMiddleTrgVerC(WORD, WORD, DWORD, DWORD, int);

WORD PCI_180X_M_FUN_1(WORD fd, WORD wDaFrequency, WORD wDaWave, float fDaAmplitude, WORD wAdClock, WORD wAdNumber, WORD wAdConfig, float fAdBuf[], float fLowAlarm, float fHighAlarm);
WORD PCI_180X_M_FUN_2(WORD fd, WORD wDaNumber, WORD wDaWave, WORD wDaBuf[], WORD wAdClock, WORD wAdNumber, WORD wAdConfig, WORD wAdBuf[]);
WORD PCI_180X_M_FUN_3(WORD fd, WORD wDaFrequency, WORD wDaWave, float fDaAmplitude, WORD wAdClock, WORD wAdNumber, WORD  wChannelStatus[], WORD wAdConfig[], float fAdBuf[], float fLowAlarm, float fHighAlarm);
WORD PCI_180X_M_FUN_4(WORD fd, WORD wType, WORD wDaFrequency, WORD wDaWave, float fDaAmplitude, WORD wAdClock, WORD wAdNumber, WORD wChannelStatus[], WORD wAdConfig[], float fAdBuf[], float fLowAlarm, float fHighAlarm);
#endif
