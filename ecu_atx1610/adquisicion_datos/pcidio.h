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

/* defined General ERROR ID */
#define PCIDA_NOERROR 0
#define PCIDA_MODULE_NAME_GET_ERROR 1
#define PCIDA_PARAMETER_ERROR 2
#define PCIDA_IOTCL_ERROR 3
#define PCIDA_SIGACTION_ERROR 4


/* defined PCI-180X Library ERROR ID */
#define PCIDA_DIGITAL_OUTPUT_ERROR 2
#define PCIDA_DIGITAL_INPUT_ERROR 3
#define PCIDA_ANALOG_OUTPUT_ERROR 4
#define PCIDA_ANALOG_INPUT_ERROR 5
#define PCIDA_READ_SR_ERROR 6
#define PCIDA_WRITE_CR_ERROR 7
#define PCIDA_SOFTWARE_TRIGGER_ERROR 8
#define PCIDA_CR_TIME_OUT 9
#define PCIDA_WRITE_8254CR_ERROR 10
#define PCIDA_WRITE_8254C0_ERROR 11
#define PCIDA_WRITE_8254C1_ERROR 12
#define PCIDA_WRITE_8254C2_ERROR 13
#define PCIDA_8254_DELAY_ERROR 14
#define PCIDA_SET_CHANNEL_ERROR 15
#define PCIDA_CREATE_SCAN_THREAD_ERROR 16
#define PCIDA_MAGIC_SCAN_THREAD_ERROR 17
#define PCIDA_CONFIG_CODE_ERROR 18
#define PCIDA_HIGH_ALARM 19
#define PCIDA_LOW_ALARM 20

/* defined PCI-LANNER Library ERROR ID */
#define PCIDA_LANNER_DIGITAL_OUTPUT_ERROR 2
#define PCIDA_LANNER_DIGITAL_INPUT_ERROR 3
#define PCIDA_LANNER_COUNTER_NUMBER_ERROR 4
#define PCIDA_LANNER_READ_COUNTER_ERROR 5
#define PCIDA_LANNER_CLEAR_COUNTER_ERROR 6
#define PCIDA_LANNER_SET_CHANNEL_GAIN_ERROR 7
#define PCIDA_LANNER_SET_POLLING_ERROR 8
#define PCIDA_LANNER_ANALOG_INPUT_ERROR 9
#define PCIDA_LANNER_ANALOG_CHANNEL_ERROR 10
#define PCIDA_LANNER_ANALOG_GAIN_ERROR 11
#define PCIDA_LANNER_EEPROM_ADDR_ERROR 12
#define PCIDA_LANNER_EEPROM_WRITE_ERROR 13

/* define driver and dll version */
#define IXPCI_LIBRARY_VERSION "0.3.1" 
#define IXPCI_MODULE_NAME
#define LINE_SIZE 128
#define MAX_BOARD_NUMBER 30
#define MAX_CARD_SUPPORT 4
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

/* implement the function in PIODIO.c */

#ifdef __cplusplus
extern "C" {
#endif

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

/* support PCI-1800(H/L) card */

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
float PCI_180X_ComputeRealValue(WORD, DWORD, DWORD);

WORD PCI_180X_M_FUN_1(WORD fd, WORD wDaFrequency, WORD wDaWave, float fDaAmplitude, WORD wAdClock, WORD wAdNumber, WORD wAdConfig, float fAdBuf[], float fLowAlarm, float fHighAlarm);
WORD PCI_180X_M_FUN_2(WORD fd, WORD wDaNumber, WORD wDaWave, WORD wDaBuf[], WORD wAdClock, WORD wAdNumber, WORD wAdConfig, WORD wAdBuf[]);
WORD PCI_180X_M_FUN_3(WORD fd, WORD wDaFrequency, WORD wDaWave, float fDaAmplitude, WORD wAdClock, WORD wAdNumber, WORD  wChannelStatus[], WORD wAdConfig[], float fAdBuf[], float fLowAlarm, float fHighAlarm);
WORD PCI_180X_M_FUN_4(WORD fd, WORD wType, WORD wDaFrequency, WORD wDaWave, float fDaAmplitude, WORD wAdClock, WORD wAdNumber, WORD wChannelStatus[], WORD wAdConfig[], float fAdBuf[], float fLowAlarm, float fHighAlarm);

#define PCI_120X_DaF_Output PCI_180X_DaF_Output
#define PCI_120X_Da_Output PCI_180X_Da_Output
#define PCI_120X_Digital_Output PCI_180X_Digital_Output
#define PCI_120X_Digital_Input PCI_180X_Digital_Input
#define PCI_120X_SetChannelConfig PCI_180X_SetChannelConfig
#define PCI_120X_DelayUs PCI_180X_DelayUs
#define PCI_120X_AdsPolling PCI_180X_AdsPolling
#define PCI_120X_AdsPacer PCI_180X_AdsPacer
#define PCI_120X_AddToScan PCI_180X_AddToScan
#define PCI_120X_SaveScan PCI_180X_SaveScan
#define PCI_120X_ClearScan PCI_180X_ClearScan
#define PCI_120X_StartScan PCI_180X_StartScan
#define PCI_120X_ReadScanStatus PCI_180X_ReadScanStatus
#define PCI_120X_StopMagicScan PCI_180X_StopMagicScan
#define PCI_120X_StartScanPostTrg PCI_180X_StartScanPostTrg
#define PCI_120X_StartScanPreTrg PCI_180X_StartScanPreTrg
#define PCI_120X_StartScanMiddleTrg PCI_180X_StartScanMiddleTrg
#define PCI_120X_StartScanPreTrgVerC PCI_180X_StartScanPreTrgVerC
#define PCI_120X_StartScanMiddleTrgVerC PCI_180X_StartScanMiddleTrgVerC
#define PCI_120X_ComputeRealValue PCI_180X_ComputeRealValue

#define PCI_120X_M_FUN_1 PCI_180X_M_FUN_1
#define PCI_120X_M_FUN_2 PCI_180X_M_FUN_2
#define PCI_120X_M_FUN_3 PCI_180X_M_FUN_3
#define PCI_120X_M_FUN_4 PCI_180X_M_FUN_4

/* support PCI-Lanner I/O card */

WORD PCI_LANNER_Digital_Output(WORD fd, BYTE data);
WORD PCI_LANNER_Digital_Input(WORD fd, BYTE *di_data);
WORD PCI_LANNER_Read_Count(WORD fd, WORD count, DWORD *count_value);
WORD PCI_LANNER_Clear_Count(WORD fd, WORD count);
WORD PCI_LANNER_Set_Voltate_Gain_MUX(WORD fd, WORD channel, WORD gain);
WORD PCI_LANNER_Set_Current_Gain_MUX(WORD fd, WORD channel);
WORD PCI_LANNER_ReadAI_Hex(WORD fd, WORD *hex_value);
WORD PCI_LANNER_Read_Voltage(WORD fd, WORD channel, WORD gain, float *fvalue);
WORD PCI_LANNER_Read_CalVoltage(WORD fd, WORD channel, WORD gain, float *fvalue);
WORD PCI_LANNER_Read_Current(WORD fd, WORD channel, float *fvalue);
WORD PCI_LANNER_Read_CalCurrent(WORD fd, WORD channel, float *fvalue);
WORD PCI_LANNER_EEPROM_WriteEnable(WORD fd);
WORD PCI_LANNER_EEPROM_WriteDisable(WORD fd);
WORD PCI_LANNER_EEPROM_WriteWord(WORD fd, WORD addr, WORD value);
WORD PCI_LANNER_EEPROM_ReadWord(WORD fd, WORD addr, WORD *value);
WORD PCI_LANNER_Read_Voltage_Polling(WORD fd, WORD channel, WORD gain, DWORD datacount, float *voltage);
WORD PCI_LANNER_Read_Current_Polling(WORD fd, WORD channel, DWORD datacount, float *current);
WORD PCI_LANNER_Set_Voltage_Gain_MUX(WORD fd, WORD channel, WORD gain);

#ifdef __cplusplus
}
#endif

