#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "ixpio.h"

/* defined ERROR ID */
#define PIODA_NOERROR 0
#define PIODA_MODULE_NAME_GET_ERROR 1
#define PIODA_DEVICE_DIO_INIT_ERROR 2
#define PIODA_ACTIVE_PORT_ERROR 3
#define PIODA_PORT_DEFINED_ERROR 4
#define PIODA_DIGITAL_OUTPUT_ERROR 5
#define PIODA_DIGITAL_INPUT_ERROR 6
#define PIODA_INT_SOURCE_DEFINED_ERROR 7
#define PIODA_CONFIGURE_INTERRUPT_ERROR 8
#define PIODA_ACTIVEMODE_DEFINED_ERROR 9
#define PIODA_ADD_SIGNAL_ERROR 10
#define PIODA_AUX_CONTROL_ERROR 11
#define PIODA_AUX_DATA_ERROR 12
#define PIODA_READ_EEPROM_ERROR 13
#define PIODA_EEPROM_DATA_ERROR 14
#define PIODA_OUTPUT_VOLTAGE_ERROR 15
#define PIODA_OUTPUT_CALVOLTAGE_ERROR 16
#define PIODA_OUTPUT_CURRENT_ERROR 17
#define PIODA_OUTPUT_CALCURRENT_ERROR 18
#define PIODA_ANALOG_INPUT_VOLTAGE_ERROR 19
#define PIODA_SET_GAIN_ERROR 20
#define PIODA_SET_CHANNEL_ERROR 21
#define PIODA_LIBRARY_PARAMETER_ERROR 22
#define PIODA_CARD_ID_ERROR 23
#define PIODA_MODULE_NOT_SUPPORT 24

/* define driver and dll version */
#define IXPIO_DRIVER_VERSION "0.23.7"
#define IXPIO_LIBRARY_VERSION "0.3.3"
#define IXPIO_MODULE_NAME
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

/* define interrupt source */
#define P2C0 0x01
#define P2C1 0x02
#define P2C2 0x04
#define P2C3 0x08
#define ALL_INT 0x0f

/* defined PIO-D144, PIO_D168 interrupt source */
#define PIOD144_168_P2C0 P2C0	/* Pin 29 */
#define PIOD144_168_P2C1 P2C1	/* Pin 28 */
#define PIOD144_168_P2C2 P2C2	/* Pin 27 */
#define PIOD144_168_P2C3 P2C3	/* Pin 26 */
#define PIOD144_168_ALL_INT ALL_INT

/*  defined PIO-DA16 interrupt source */
#define PIODA16_INT0 0x01	/* enable INT 0 */
#define PIODA16_INT1 0x02       /* enable INT 1 */
#define PIODA16_ALL_INT 0x03    /* enable INT 0, INT 1 */

/*  defined PISO-725 interrupt source */
#define PISO725_INT0 0x01       /* enable INT 0 */
#define PISO725_INT1 0x02       /* enable INT 1 */
#define PISO725_INT2 0x04       /* enable INT 2 */
#define PISO725_INT3 0x08       /* enable INT 3 */
#define PISO725_INT4 0x10       /* enable INT 4 */
#define PISO725_INT5 0x20       /* enable INT 5 */
#define PISO725_INT6 0x40       /* enable INT 6 */
#define PISO725_INT7 0x80       /* enable INT 7 */
#define PISO725_ALL_INT 0xff    /* enable INT 0 ~ 7 */

/*  defined PISO-730/A interrupt source */
#define PISO730_INT0 0x01       /* enable INT 0 */
#define PISO730_INT1 0x02       /* enable INT 1 */
#define PISO730_ALL_INT 0x3     /* enable INT 0, INT 1 */

/* defined PIO-D96 interrupt source */
#define PIOD96_P2C0 P2C0         /* enable P2C0 interrupt */
#define PIOD96_P2C1 P2C1         /* enable P5C0 interrupt */
#define PIOD96_P5C0 PIOD96_P2C1  /* enable P5C0 interrupt */
#define PIOD96_P2C2 P2C2         /* enable P8C0 interrupt */
#define PIOD96_P8C0 PIOD96_P2C2  /* enable P8C0 interrupt */
#define PIOD96_P2C3 P2C3         /* enable P11C0 interrupt */
#define PIOD96_P11C0 PIOD96_P2C3 /* enable P11C0 interrupt */
#define PIOD96_ALL_INT ALL_INT   /* enable P2C0, P5C0, P8C0,P11C0 interrupt */

/* defined PIO-D56/D24 interrupt source */
#define PIOD56_24_P2C0 P2C0         /* enable P2C0 interrupt */
#define PIOD56_24_P2C1 P2C1         /* enable P2C1 interrupt */
#define PIOD56_24_P2C2 P2C2         /* enable P2C2 interrupt */
#define PIOD56_24_P2C3 P2C3         /* enable P2C3 interrupt */
#define PIOD56_24_ALL_INT ALL_INT   /* enable P2C0, P2C1, P2C2,P2C3 interrupt */

/* defined PIO-D48 interrupt source */	/* address = base + 5*/
#define PIOD48_INT0 P2C0	/* enable INT0 interrupt */
#define PIOD48_INT1 P2C1	/* enable INT1 interrupt */
#define PIOD48_INT2 P2C2	/* enable INT2 interrupt */
#define PIOD48_INT3 P2C3	/* enable INT3 interrupt */
#define PIOD48_ALL_INT ALL_INT	/* enable INt0, INT1, INT2, INT3 interrupt */

/* defined pisop64 di range */
#define PISOP64_DIA IXPIO_DI_A /* DI 0  ~ DI 7  */  
#define PISOP64_DIB IXPIO_DI_B /* DI 8  ~ DI 15 */
#define PISOP64_DIC IXPIO_DI_C /* DI 16 ~ DI 23 */
#define PISOP64_DID IXPIO_DI_D /* DI 24 ~ DI 31 */
#define PISOP64_DIE IXPIO_DI_E /* DI 32 ~ DI 39 */
#define PISOP64_DIF IXPIO_DI_F /* DI 40 ~ DI 47 */
#define PISOP64_DIG IXPIO_DI_G /* DI 48 ~ DI 55 */
#define PISOP64_DIH IXPIO_DI_H /* DI 56 ~ DI 63 */

/* defined pisoa64 do range */
#define PISOA64_DOA IXPIO_DO_A /* DI 0  ~ DI 7  */
#define PISOA64_DOB IXPIO_DO_B /* DI 8  ~ DI 15 */
#define PISOA64_DOC IXPIO_DO_C /* DI 16 ~ DI 23 */
#define PISOA64_DOD IXPIO_DO_D /* DI 24 ~ DI 31 */
#define PISOA64_DOE IXPIO_DO_E /* DI 32 ~ DI 39 */
#define PISOA64_DOF IXPIO_DO_F /* DI 40 ~ DI 47 */
#define PISOA64_DOG IXPIO_DO_G /* DI 48 ~ DI 55 */
#define PISOA64_DOH IXPIO_DO_H /* DI 56 ~ DI 63 */

/* defined pisoc64 do range */
#define PISOC64_DOA IXPIO_DO_A /* DI 0  ~ DI 7  */
#define PISOC64_DOB IXPIO_DO_B /* DI 8  ~ DI 15 */
#define PISOC64_DOC IXPIO_DO_C /* DI 16 ~ DI 23 */
#define PISOC64_DOD IXPIO_DO_D /* DI 24 ~ DI 31 */
#define PISOC64_DOE IXPIO_DO_E /* DI 32 ~ DI 39 */
#define PISOC64_DOF IXPIO_DO_F /* DI 40 ~ DI 47 */
#define PISOC64_DOG IXPIO_DO_G /* DI 48 ~ DI 55 */
#define PISOC64_DOH IXPIO_DO_H /* DI 56 ~ DI 63 */

/* defined pisop32c32 dio range */
#define PISOP32C32_DIOA IXPIO_DIO_A /* DIO 0  ~ DIO 7 */
#define PISOP32C32_DIOB IXPIO_DIO_B /* DIO 8  ~ DIO 15 */
#define PISOP32C32_DIOC IXPIO_DIO_C /* DIO 16 ~ DIO 23 */
#define PISOP32C32_DIOD IXPIO_DIO_D /* DIO 24 ~ DIO 31 */
#define PISOP32C32_DIO_ALL IXPIO_DIO /* DIO 0 ~ DIO 31 */

/* defined pisop32a32 dio range */
#define PISOP32A32_DIOA IXPIO_DIO_A /* DIO 0  ~ DIO 7 */
#define PISOP32A32_DIOB IXPIO_DIO_B /* DIO 8  ~ DIO 15 */
#define PISOP32A32_DIOC IXPIO_DIO_C /* DIO 16 ~ DIO 23 */
#define PISOP32A32_DIOD IXPIO_DIO_D /* DIO 24 ~ DIO 31 */
#define PISOP32A32_DIO_ALL IXPIO_DIO  /* DIO 0 ~ DIO 31 */

/* Defined piod48_pexd48 port number */
#define PIOD48_P0	IXPIO_PORT_0	/* CN1 PA, port0 */
#define PIOD48_P1	IXPIO_PORT_1	/* CN1 PB, port1 */
#define PIOD48_P2	IXPIO_PORT_2	/* CN1 PC, port2 */
#define PIOD48_P2_HB	IXPIO_PORT_2_HB	/* CN1 PC high nibble, port 2 */
#define PIOD48_P2_LB	IXPIO_PORT_2_LB	/* CN1 PC low nibble, port 2 */

#define PIOD48_P3	IXPIO_PORT_3	/* CN2 PA, port3 */
#define PIOD48_P4	IXPIO_PORT_4	/* CN2 PB, port4 */
#define PIOD48_P5	IXPIO_PORT_5	/* CN2 PC, port5 */
#define PIOD48_P5_HB	IXPIO_PORT_5_HB	/* CN2 PC high nibble, port 5 */
#define PIOD48_P5_LB	IXPIO_PORT_5_LB	/* CN2 PC low nibble, port 5 */

/* Defined piso/pex-p16r16 pex-p8r8 port number */
#define PISOP16R16_DIOA	IXPIO_DIO_A /* DIO 0 ~ DIO 7 */
#define PISOP16R16_DIOB	IXPIO_DIO_B /* DIO 8 ~ DIO 15 */ //PISO_PEX-P16R16 only
#define PISOP16R16_DIO_ALL IXPIO_DIO /* DIO 0 ~ DIO 15 */

/* Defined piso-730/A port number */
#define PIS730_IDIOA	IXPIO_IDIO_L	/* IDIO 0 ~ IDIO 7 */
#define PIS730_IDIOB	IXPIO_IDIO_H	/* IDIO 8 ~ IDIO 15 */
#define PIS730_IDIO	IXPIO_IDIO	/* IDIO 0 ~ IDIO 15 */
#define PIS730_DIOA	IXPIO_DIO_A	/* DIO 0 ~ DIO 7 */
#define PIS730_DIOB	IXPIO_DIO_B	/* DIO 8 ~ DIO 15 */
#define	PIS730_DIO	IXPIO_DIO	/* DIO 0 ~ DIO 15 */

typedef unsigned short WORD;
typedef void (*HANDLE)(int);
typedef unsigned char byte;
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

typedef struct dev_control {
	port_config_t pconf[MAX_PORT_LEVEL];
        WORD (*deviceinit)(WORD, WORD);
        WORD (*portdircfs)(WORD, WORD, boolean);
        WORD (*doutput)(WORD, WORD);
        WORD (*doutput_port)(WORD, WORD, byte);
        WORD (*dinput)(WORD, WORD *);
        WORD (*dinput_port)(WORD, WORD, WORD *);
        WORD (*int_source_init)(WORD, WORD, WORD);
        //WORD (*avoutput_cal)(WORD fd, WORD channel, float fvalue);
        //WORD (*acoutput_cal)(WORD fd, WORD channel, float fvalue);
        //WORD (*avoutput_uncal)(WORD fd, WORD channel, float fvalue);
        //WORD (*acoutput_uncal)(WORD fd, WORD channel, float fvalue);
        WORD (*ainput)(WORD fd, WORD channel, WORD gain, float *fvalue);
	WORD (*C8254W)(WORD, WORD, WORD, WORD, WORD);
	WORD (*C8254_0)(WORD, WORD, WORD);
	WORD (*C8254_1)(WORD, WORD, WORD);
	WORD (*C8254_2)(WORD, WORD, WORD);
	WORD (*card_id)(WORD, byte *);
	int fd;
	char device_module[32];
}dev_control_t;


#ifdef __cplusplus
extern "C" {
#endif

/* implement the function in PIODIO.c */
int PIODA_Open(char *);
WORD PIODA_Close(WORD);
WORD PIODA_DriverInit(WORD);
char* PIODA_GetDriverVersion(void);
char* PIODA_GetLibraryVersion(void);
WORD PIODA_DeviceInit(WORD);
WORD PIODA_PortDirCfs(WORD, WORD, boolean);
WORD PIODA_Digital_Output(WORD, WORD, WORD);
WORD PIODA_Digital_Input(WORD, WORD, WORD *);
WORD PIODA_IntInstall(WORD, HANDLE, WORD, WORD, WORD);
WORD PIODA_IntRemove(WORD, WORD);
WORD PIODA_OutputByte(WORD, ixpio_reg_t *);
WORD PIODA_InputByte(WORD, ixpio_reg_t *);
WORD PIODA_OutputAnalog(WORD, ixpio_analog_t *);
WORD PIODA_8254CW(WORD, WORD, WORD, WORD, WORD);
WORD PIODA_8254C0(WORD, WORD, WORD);
WORD PIODA_8254C1(WORD, WORD, WORD);
WORD PIODA_8254C2(WORD, WORD, WORD);
WORD PIODA_CARD_ID(WORD, byte *);

/* for A/D Card */
WORD PIODA_SetChannelConfig(WORD fd, WORD channel, WORD Config);
WORD PIODA_AnalogOutputHex(WORD fd, WORD channel, WORD value);
WORD PIODA_AnalogInputHex(WORD fd, WORD channel, WORD *value);
WORD PIODA_AnalogOutputVoltage(WORD fd, WORD channel, float fvalue);
WORD PIODA_AnalogOutputCalVoltage(WORD fd, WORD channel, float fvalue);
WORD PIODA_AnalogOutputCurrent(WORD fd, WORD channel, float fvalue);
WORD PIODA_AnalogOutputCalCurrent(WORD fd, WORD channel, float fvalue);
WORD PIODA_AnalogInputVoltage(WORD fd, WORD channel, WORD gain, float *fvalue);

WORD PIOD48_INT01_CONTROL(WORD fd, WORD Int, WORD option);
WORD PIOD48_8254Clk(WORD fd, WORD boolean);

/* implement under function in pio_device.c */

/* support PIOD144,D168 card */
WORD PIOD144_168_DeviceInit(WORD, WORD);		 
WORD PIOD144_168_PortDirCfs(WORD, WORD, boolean);		 
WORD PIOD144_168_Digital_Output(WORD, WORD,byte);		 
WORD PIOD144_168_Digital_Input(WORD, WORD, WORD *);		 
WORD PIOD144_168_Int_Source_Init(WORD, WORD, WORD);          

/* support PIO-D56,D24 card */
WORD PIOD56_24_PortDirCfs(WORD, WORD, boolean);
WORD PIOD56_24_Digital_Output(WORD, WORD, byte);
WORD PIOD56_24_Digital_Input(WORD, WORD, WORD *);
WORD PIOD56_24_Int_Source_Init(WORD, WORD, WORD);

/* support PIO-D96 card */
WORD PIOD96_PortDirCfs(WORD, WORD, boolean);
WORD PIOD96_Digital_Output(WORD, WORD, byte);
WORD PIOD96_Digital_Input(WORD, WORD, WORD *);
WORD PIOD96_Int_Source_Init(WORD, WORD, WORD);

/* support PISOP8R8 card */
WORD PISOP8R8_Digital_Output(WORD, WORD);
WORD PISOP8R8_Digital_Input(WORD, WORD *);

/* support PISO725 card */
WORD PISO725_Digital_Output(WORD, byte);
WORD PISO725_Digital_Input(WORD, WORD *);
WORD PISO725_Int_Source_Init(WORD, WORD, WORD);

/* support PISOA64/C64, PISOP64, PISO-P32A32/P32C32 card */
WORD PISOA64_C64_Digital_Output(WORD, WORD, byte);
WORD PISOP64_Digital_Input(WORD, WORD, WORD *);
WORD PISOP32A32_C32_Digital_Output(WORD, WORD, byte);
WORD PISOP32A32_C32_Digital_Input(WORD, WORD, WORD *);

/* support PIODA16 card */
WORD PIODA16_Digital_Output(WORD, WORD);
WORD PIODA16_Digital_Input(WORD, WORD *);
WORD PIODA16_Int_Source_Init(WORD, WORD, WORD);          

/* support PISO813 card */
WORD PISO813_Analog_Input_Voltage(WORD, WORD, WORD, float *);
WORD PISO813_Card_ID(WORD, byte *);

/* support PISO730, PISO-730A card */
WORD PISO730_Digital_Output(WORD, WORD, byte);
WORD PISO730_Digital_Input(WORD, WORD , WORD *);
WORD PISO730_Int_Source_Init(WORD, WORD , WORD);


#ifdef __cplusplus
}
#endif 
