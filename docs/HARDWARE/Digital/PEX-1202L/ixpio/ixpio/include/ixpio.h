/* Declarations for PIO series.

   Author: Reed Lai

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* File level history (record changes for this file here.)

   Use vi or gvim to edit this file and set tabstop=4.

   v 0.14.0  6 Aug 2003 by Reed Lai
     Complex signaling condition.
     Fixed the SUCCESS and FAILURE definitions.

   v 0.13.0  2 Jul 2003 by Reed Lai
     Gives support to PIO-D64.
     Removed all RW and READ_WRITE words.
     Implements the individual ware pattern control - on, off, status,...

   v 0.12.0 20 Jun 2003 by Reed Lai
     Defines IXPIO_PROC_FILE

   v 0.11.1 18 Dec 2002 by Reed Lai
     Abandons the ixpio_devinfo member -- ipcr.

   v 0.11.0  2 Dec 2002 by Reed Lai
     Gives support to PIO-DA16/DA8/DA4.

   v 0.10.0 26 Nov 2002 by Reed Lai
     Gives support to PIO-D96.

   v 0.9.0 25 Nov 2002 by Reed Lai
     Gives support to PISO-P8R8/P8SSR8AC/P8SSR8DC.

   v 0.8.0 14 Nov 2002 by Reed Lai
     Gives support to PISO-813.

   v 0.7.0  7 Nov 2002 by Reed Lai
     ICPDAS_LICENSE

   v 0.6.0 31 Oct 2002 by Reed Lai
     Gives support to PIO-D24/D56.

   v 0.5.0 25 Jul 2002 by Reed Lai
     Gives supoort to Linux kernel 2.4.

   v 0.4.0 25 Jun 2002 by Reed Lai
     Gives support to PISO-730A.

   v 0.3.0  4 Jun 2002 by Reed Lai
     Gives support to PISO-725.

   v 0.2.0  3 May 2002 by Reed Lai
     Gives support to PISO-P32C32.

   v 0.1.0 29 Apr 2002 by Reed Lai
     Gives support to PISO-730.
     Renames PIO to IXPIO.

   v 0.0.1 5 Apr 2000 by Reed Lai
     Modified some structures.

   v 0.00 27 Mar 2000 by Reed Lai
     create, blah blah... */

/* *INDENT-OFF* */

#ifndef _IXPIO_H
#define _IXPIO_H

#include <linux/pci.h>
//#include <linux/sched.h>
#include <linux/version.h>
#include <linux/types.h>

#include "_flags.h"

#define ICPDAS_LICENSE "GPL"

/* General Definition */
#ifdef SUCCESS
#undef SUCCESS
#endif
#define SUCCESS 0

#ifdef FAILURE
#undef FAILURE
#endif
#define FAILURE -1

#define ADD_PRESENT 1
#define BOARD_IRQ_DISABLE 0

#ifndef SA_SHIRQ
#define SA_SHIRQ IRQF_SHARED
#endif

/* PIO Device */
#define ORGANIZATION "icpdas"
#define FAMILY "ixpio"			/* name of family */
#define DEVICE_NAME "ixpio"		/* device entry name */
#define DEVICE_NAME_LEN 5
#define DEVICE_NR_DEVS 4                
#define DEVICE_MAJOR 0                  /* dynamic allocation of major number */
#define DEVICE_MINOR 0                  /* set first minor number */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
#define IXPIO_PROC_FILE "/proc/ixpio/ixpio"
#else
#define IXPIO_PROC_FILE "/proc/ixpio"
#endif

#define CARD_NAME_LENGTH 32
#define CNL CARD_NAME_LENGTH

#define KMSG(fmt, args...) printk(KERN_INFO FAMILY ": " fmt, ## args)

/* PIO PCI IDs */
#define IXPIO_VENDOR_ID PCI_VENDOR_ID_TIGERJET	/* not reliable! */
#define IXPIO_DEVICE_ID 0x0002	/* not reliable! */
#define IXPIO_DEVICE_ID2 0x0001 /* not reliable! */
#define IXPIO_SUB_AUX_ID_OFFSET 7
#define IXPIO_SUB_AUX_ID_RANGE  8
#define IXPIO_SUB_AUX_ID_MASK   0xf0

/*********************************
 * IxPIO PCI composed sub ID     *
 *                               *
 *           0x 0000 00 00       *
 *              ---- -- --       *
 *                |  |  |        *
 *    sub-vendor id  |  |        *
 *                   |  |        *
 *       sub-device id  |        *
 *                      |        *
 *             sub-aux id        *
 *********************************/
#define PIO_D168A      0x00800150	/* Tiger 100 */
#define PIO_D168       0x98800150	/* Tiger 320 */
#define PIO_D168_T3A   0x98800150	/* Tiger 320 */
#define PIO_D168_T3B   0xd8800150	/* Tiger 320 */
#define PIO_D144       0x00800100
#define PIO_D144_T3A   0x5C800100
#define PIO_D144_T3B   0x1C800100
#define PIO_D96        0x00800110
#define PIO_D96_T3A    0x58800110
#define PIO_D96_T3B    0x18800110
#define PIO_D64A       0x00800120	/* Tiger 100 */
#define PIO_D64	       0x40800120	/* Tiger 320 */
#define PIO_D64_T3A    0x40800120	/* Tiger 320 */
#define PIO_D64_T3B    0x00800120	/* Tiger 320 */
#define PIO_D56        0x00800140	/* conflict */
#define PIO_D56_T3A    0xC0800140	/* conflict */
#define PIO_D56_T3B    0x80800140	/* conflict */
#define PIO_D48        0x00800130
#define PIO_D48_T3A    0x00800130
#define PIO_D48_T3B    0x40800130
#define PIO_D24        0x00800140	/* conflict */
#define PIO_D24_T3A    0xC0800140	/* 320 A */
#define PIO_D24_T3B    0x80800140	/* 320 B */

#define PIO_823        0x00800300
#define PIO_821        0x40ff0310
#define PIO_821_T3A    0x40ff0300

#define PIO_DA16       0x00800400	/* conflict */
#define PIO_DA16_T3A   0x41800000	/* conflict */
#define PIO_DA16_T3B   0x01800000	/* conflict */
#define PIO_DA8        0x00800400	/* conflict */
#define PIO_DA8_T3A    0x41800000	/* conflict */
#define PIO_DA8_T3B    0x01800000	/* conflict */
#define PIO_DA4        0x00800400	/* conflict */
#define PIO_DA4_T3A    0x41800000	/* conflict */
#define PIO_DA4_T3B    0x01800000	/* conflict */

#define PISO_C64       0x00800800
#define PISO_C64_T3A   0x02800000
#define PISO_C64_T3B   0x42800000
#define PISO_P64       0x00800810
#define PISO_P64_T3A   0x02800010
#define PISO_P64_T3B   0x42800010
#define PISO_A64       0x00800850
#define PISO_A64_T3A   0x82800050
#define PISO_A64_T3B   0xC2800050
#define PISO_P32C32    0x00800820
#define PISO_P32C32_T3A    0x02800020
#define PISO_P32C32_T3B    0x42800020
#define PISO_P32A32    0x00800870
#define PISO_P32A32_T3A    0x82800070
#define PISO_P32A32_T3B    0xC2800070
#define PISO_P8R8      0x00800830	/* conflict */
#define PISO_P8R8_T3A  0x42000030	/* conflict */
#define PISO_P8R8_T3B  0x02000030	/* conflict */
#define PISO_P8R8_T3C  0x4A800000	/* conflict */
#define PISO_P8SSR8AC  0x00800830	/* conflict */
#define PISO_P8SSR8AC_T3A  0x42000030	/* conflict */
#define PISO_P8SSR8AC_T3B  0x02000030	/* conflict */
#define PISO_P8SSR8DC  0x00800830	/* conflict */
#define PISO_P8SSR8DC_T3A  0x42000030	/* conflict */
#define PISO_P8SSR8DC_T3B  0x02000030	/* conflict */
#define PISO_730       0x00800840
#define PISO_730_T3A   0xC2FF0040
#define PISO_730_T3B   0x82FF0040
#define PISO_730_T3C   0xCA800000
#define PISO_730A      0x00800880
#define PISO_730A_T3A  0x62FF0080
#define PISO_730A_T3B  0x22FF0080
#define PISO_813       0x00800a00
#define PISO_813_T3A   0x42800200
#define PISO_813_T3B   0x02800200
#define PISO_DA2       0x00800b00
#define PISO_DA2_T3A   0x42800300
#define PISO_DA2_T3B   0x02800300
#define PISO_725       0x00800c00
#define PISO_725_T3A   0x03800000
#define PISO_725_T3B   0x43800000
#define PISO_725_T3C   0xc3800000
#define PISO_PS300     0x00810410
#define PISO_PS300_T3A 0x41810010
#define PISO_PS300_T3B 0x01810010
#define PISO_ENC300    0x00810510	/* conflict */
#define PISO_ENC600    0x00810510	/* conflict */
#define PISO_P16R16U   0x18000000

/* id of registers */
enum {
	IXPIO_RESET_CONTROL_REGISTER,
	IXPIO_AUX_CONTROL_REGISTER,
	IXPIO_AUX_DATA_REGISTER,
	IXPIO_INT_MASK_CONTROL_REGISTER,
	IXPIO_AUX_PIN_STATUS_REGISTER,
	IXPIO_INTERRUPT_POLARITY_CONTROL_REGISTER,

	IXPIO_8BIT_DATA_REGISTER,
	IXPIO_ACTIVE_IO_PORT_CONTROL_REGISTER,
	IXPIO_IO_SECECT_CONTROL_REGISTER,
	IXPIO_IO_SECECT_CONTROL_REGISTER_A,
	IXPIO_IO_SECECT_CONTROL_REGISTER_B,
	IXPIO_IO_SECECT_CONTROL_REGISTER_C,
	IXPIO_IO_SECECT_CONTROL_REGISTER_D,

	IXPIO_8255_PORT_A,
	IXPIO_8255_PORT_B,
	IXPIO_8255_PORT_C,
	IXPIO_8255_CONTROL_WORD,

	IXPIO_8255_2_PORT_A,
	IXPIO_8255_2_PORT_B,
	IXPIO_8255_2_PORT_C,
	IXPIO_8255_2_CONTROL_WORD,

	IXPIO_8254_COUNTER_0,
	IXPIO_8254_COUNTER_1,
	IXPIO_8254_COUNTER_2,
	IXPIO_8254_CONTROL_WORD,

	IXPIO_8254_2_COUNTER_0,
	IXPIO_8254_2_COUNTER_1,
	IXPIO_8254_2_COUNTER_2,
	IXPIO_8254_2_CONTROL_WORD,

	IXPIO_CLOCK_INT_CONTROL_REGISTER,

	IXPIO_IDIO0_TO_IDIO7,
	IXPIO_IDIO8_TO_IDIO15,
	IXPIO_IDIO,

	IXPIO_DIO0_TO_DIO7,
	IXPIO_DIO8_TO_DIO15,
	IXPIO_DIO16_TO_DIO23,
	IXPIO_DIO24_TO_DIO31,
	IXPIO_DIO,

	IXPIO_DI0_TO_DI7,
	IXPIO_DI8_TO_DI15,
	IXPIO_DI16_TO_DI23,
	IXPIO_DI24_TO_DI31,
	IXPIO_DI32_TO_DI39,
	IXPIO_DI40_TO_DI47,
	IXPIO_DI48_TO_DI55,
	IXPIO_DI56_TO_DI63,
	IXPIO_DI,

	IXPIO_DO0_TO_DO7,
	IXPIO_DO8_TO_DO15,
	IXPIO_DO16_TO_DO23,
	IXPIO_DO24_TO_DO31,
	IXPIO_DO32_TO_DO39,
	IXPIO_DO40_TO_DO47,
	IXPIO_DO48_TO_DO55,
	IXPIO_DO56_TO_DO63,
	IXPIO_DO,

	IXPIO_PORT_0,
	IXPIO_PORT_1,
	IXPIO_PORT_2,
	IXPIO_PORT_CONFIGURATION,
	IXPIO_PORT_CONF_A,

	IXPIO_PORT_2_HB,
	IXPIO_PORT_2_LB,

	IXPIO_PORT_3,
	IXPIO_PORT_4,
	IXPIO_PORT_5,
	IXPIO_PORT_CONF_B,

	IXPIO_PORT_5_HB,
	IXPIO_PORT_5_LB,

	IXPIO_PORT_6,
	IXPIO_PORT_7,
	IXPIO_PORT_8,
	IXPIO_PORT_CONF_C,

	IXPIO_PORT_9,
	IXPIO_PORT_10,
	IXPIO_PORT_11,
	IXPIO_PORT_CONF_D,

	IXPIO_CON1_LOW_BYTE,
	IXPIO_CON1_HIGH_BYTE,
	IXPIO_CON2_LOW_BYTE,
	IXPIO_CON2_HIGH_BYTE,

	IXPIO_AD_LOW_BYTE_DATA_REGISTER,
	IXPIO_AD_HIGH_BYTE_DATA_REGISTER,
	IXPIO_MULTIPLEXER_CHANNEL_SELECT_REGISTER,
	IXPIO_PGA_GAIN_CODE_REGISTER,
	IXPIO_AD_TRIGGER_CONTROL_REGISTER,
	IXPIO_AD,

	IXPIO_DA_0_CHIP_SELECT,
	IXPIO_DA_1_CHIP_SELECT,
	IXPIO_DA_2_CHIP_SELECT,
	IXPIO_DA_3_CHIP_SELECT,

	IXPIO_DA_LOW_BYTE,
	IXPIO_DA_HIGH_BYTE,
	IXPIO_DA1_LOW_BYTE,
	IXPIO_DA1_HIGH_BYTE,
	IXPIO_DA2_LOW_BYTE,
	IXPIO_DA2_HIGH_BYTE,
	IXPIO_DA,

	IXPIO_JUMPER_STATUS,
	IXPIO_READ_CARD_ID,

	IXPIO_FIFO1_REGISTER,
	IXPIO_FIFO2_REGISTER,
	IXPIO_RSTFIFO1_REGISTER,
	IXPIO_MSC_REGISTER,

	IXPIO_TJ_CNTL,
	IXPIO_TJ_AUXC,
	IXPIO_TJ_AUXD,
	IXPIO_TJ_MASK,
	IXPIO_TJ_AUX_STATUS,
	IXPIO_TJ_POLARITY,

	IXPIO_X1_AXIS_CONTROL_REGISTER,
	IXPIO_X2_AXIS_CONTROL_REGISTER,
	IXPIO_X3_AXIS_CONTROL_REGISTER,
	IXPIO_X4_AXIS_CONTROL_REGISTER,
	IXPIO_X5_AXIS_CONTROL_REGISTER,
	IXPIO_X6_AXIS_CONTROL_REGISTER,
	
	IXPIO_X1_AXIS_DIGITAL_INPUT_REGISTER,
	IXPIO_X2_AXIS_DIGITAL_INPUT_REGISTER,
	IXPIO_X3_AXIS_DIGITAL_INPUT_REGISTER,
	IXPIO_X4_AXIS_DIGITAL_INPUT_REGISTER,
	IXPIO_X5_AXIS_DIGITAL_INPUT_REGISTER,
	IXPIO_X6_AXIS_DIGITAL_INPUT_REGISTER,

	IXPIO_AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER,
	IXPIO_AD_MODE_CONTROL_REGISTER,
	IXPIO_AD_STATUS,
	IXPIO_LAST_OBJECT
};

#define IXPIO_RCR     IXPIO_RESET_CONTROL_REGISTER
#define IXPIO_ACR     IXPIO_AUX_CONTROL_REGISTER
#define IXPIO_ADR     IXPIO_AUX_DATA_REGISTER
#define IXPIO_IMCR    IXPIO_INT_MASK_CONTROL_REGISTER
#define IXPIO_APSR    IXPIO_AUX_PIN_STATUS_REGISTER
#define IXPIO_ASR     IXPIO_APSR
#define IXPIO_IPCR    IXPIO_INTERRUPT_POLARITY_CONTROL_REGISTER

#define IXPIO_8DR   IXPIO_8BIT_DATA_REGISTER
#define IXPIO_AIOPCR  IXPIO_ACTIVE_IO_PORT_CONTROL_REGISTER
#define IXPIO_IOSCR   IXPIO_IO_SECECT_CONTROL_REGISTER
#define IXPIO_IOSCRA  IXPIO_IO_SECECT_CONTROL_REGISTER_A
#define IXPIO_IOSCRB  IXPIO_IO_SECECT_CONTROL_REGISTER_B
#define IXPIO_IOSCRC  IXPIO_IO_SECECT_CONTROL_REGISTER_C
#define IXPIO_IOSCRD  IXPIO_IO_SECECT_CONTROL_REGISTER_D

#define IXPIO_8255PA   IXPIO_8255_PORT_A
#define IXPIO_8255PB   IXPIO_8255_PORT_B
#define IXPIO_8255PC   IXPIO_8255_PORT_C
#define IXPIO_8255CW   IXPIO_8255_CONTROL_WORD

#define IXPIO_82551PA  IXPIO_8255PA
#define IXPIO_82551PB  IXPIO_8255PB
#define IXPIO_82551PC  IXPIO_8255PC
#define IXPIO_82551CW  IXPIO_8255CW

#define IXPIO_82552PA  IXPIO_8255_2_PORT_A
#define IXPIO_82552PB  IXPIO_8255_2_PORT_B
#define IXPIO_82552PC  IXPIO_8255_2_PORT_C
#define IXPIO_82552CW  IXPIO_8255_2_CONTROL_WORD

#define IXPIO_8254C0  IXPIO_8254_COUNTER_0
#define IXPIO_8254C1  IXPIO_8254_COUNTER_1
#define IXPIO_8254C2  IXPIO_8254_COUNTER_2
#define IXPIO_8254CW  IXPIO_8254_CONTROL_WORD

#define IXPIO_82541C0 IXPIO_8254C0
#define IXPIO_82541C1 IXPIO_8254C1
#define IXPIO_82541C2 IXPIO_8254C2
#define IXPIO_82541CW IXPIO_8254CW

#define IXPIO_82542C0 IXPIO_8254_2_COUNTER_0
#define IXPIO_82542C1 IXPIO_8254_2_COUNTER_1
#define IXPIO_82542C2 IXPIO_8254_2_COUNTER_2
#define IXPIO_82542CW IXPIO_8254_2_CONTROL_WORD
#define IXPIO_8254C3  IXPIO_82542C0
#define IXPIO_8254C4  IXPIO_82542C1
#define IXPIO_8254C5  IXPIO_82542C2
#define IXPIO_8254CW2 IXPIO_82542CW

#define IXPIO_CICR  IXPIO_CLOCK_INT_CONTROL_REGISTER

#define IXPIO_IDIO_L  IXPIO_IDIO0_TO_IDIO7
#define IXPIO_IDIO_H  IXPIO_IDIO8_TO_IDIO15

#define IXPIO_DIO_A   IXPIO_DIO0_TO_DIO7
#define IXPIO_DIO_B   IXPIO_DIO8_TO_DIO15
#define IXPIO_DIO_C   IXPIO_DIO16_TO_DIO23
#define IXPIO_DIO_D   IXPIO_DIO24_TO_DIO31

#define IXPIO_DI_A   IXPIO_DI0_TO_DI7
#define IXPIO_DI_B   IXPIO_DI8_TO_DI15
#define IXPIO_DI_C   IXPIO_DI16_TO_DI23
#define IXPIO_DI_D   IXPIO_DI24_TO_DI31
#define IXPIO_DI_E   IXPIO_DI32_TO_DI39
#define IXPIO_DI_F   IXPIO_DI40_TO_DI47
#define IXPIO_DI_G   IXPIO_DI48_TO_DI55
#define IXPIO_DI_H   IXPIO_DI56_TO_DI63

#define IXPIO_DO_A   IXPIO_DO0_TO_DO7
#define IXPIO_DO_B   IXPIO_DO8_TO_DO15
#define IXPIO_DO_C   IXPIO_DO16_TO_DO23
#define IXPIO_DO_D   IXPIO_DO24_TO_DO31
#define IXPIO_DO_E   IXPIO_DO32_TO_DO39
#define IXPIO_DO_F   IXPIO_DO40_TO_DO47
#define IXPIO_DO_G   IXPIO_DO48_TO_DO55
#define IXPIO_DO_H   IXPIO_DO56_TO_DO63

#define IXPIO_DIO_L   IXPIO_DIO_A
#define IXPIO_DIO_H   IXPIO_DIO_B
#define IXPIO_DL      IXPIO_DIO_L
#define IXPIO_DH      IXPIO_DIO_H

#define IXPIO_P0      IXPIO_PORT_0
#define IXPIO_P1      IXPIO_PORT_1
#define IXPIO_P2      IXPIO_PORT_2
#define IXPIO_PC      IXPIO_PORT_CONFIGURATION
#define IXPIO_PCA     IXPIO_PORT_CONF_A

#define IXPIO_P3      IXPIO_PORT_3
#define IXPIO_P4      IXPIO_PORT_4
#define IXPIO_P5      IXPIO_PORT_5
#define IXPIO_PCB     IXPIO_PORT_CONF_B

#define IXPIO_P6      IXPIO_PORT_6
#define IXPIO_P7      IXPIO_PORT_7
#define IXPIO_P8      IXPIO_PORT_8
#define IXPIO_PCC     IXPIO_PORT_CONF_C

#define IXPIO_P9      IXPIO_PORT_9
#define IXPIO_P10     IXPIO_PORT_10
#define IXPIO_P11     IXPIO_PORT_11
#define IXPIO_PCD     IXPIO_PORT_CONF_D

#define IXPIO_CON1L   IXPIO_CON1_LOW_BYTE
#define IXPIO_CON1H   IXPIO_CON1_HIGH_BYTE
#define IXPIO_CON2L   IXPIO_CON2_LOW_BYTE
#define IXPIO_CON2H   IXPIO_CON2_HIGH_BYTE

#define IXPIO_AI      IXPIO_AD
#define IXPIO_ADL     IXPIO_AD_LOW_BYTE_DATA_REGISTER
#define IXPIO_AIL     IXPIO_ADL
#define IXPIO_ADH     IXPIO_AD_HIGH_BYTE_DATA_REGISTER
#define IXPIO_AIH     IXPIO_ADH
#define IXPIO_MCSR    IXPIO_MULTIPLEXER_CHANNEL_SELECT_REGISTER
#define IXPIO_PGCR    IXPIO_PGA_GAIN_CODE_REGISTER
#define IXPIO_ADGCR   IXPIO_AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER
#define IXPIO_ADMCR   IXPIO_AD_MODE_CONTROL_REGISTER
#define IXPIO_ADTCR   IXPIO_AD_TRIGGER_CONTROL_REGISTER
#define IXPIO_ADS     IXPIO_AD_STATUS

#define IXPIO_DA0CS   IXPIO_DA_0_CHIP_SELECT
#define IXPIO_DA1CS   IXPIO_DA_1_CHIP_SELECT
#define IXPIO_DA2CS   IXPIO_DA_2_CHIP_SELECT
#define IXPIO_DA3CS   IXPIO_DA_3_CHIP_SELECT

#define IXPIO_DAL     IXPIO_DA_LOW_BYTE
#define IXPIO_AOL     IXPIO_DAL
#define IXPIO_DAH     IXPIO_DA_HIGH_BYTE
#define IXPIO_AOH     IXPIO_DAH
#define IXPIO_DA1_L   IXPIO_DA1_LOW_BYTE
#define IXPIO_DA1_H   IXPIO_DA1_HIGH_BYTE
#define IXPIO_DA2_L   IXPIO_DA2_LOW_BYTE
#define IXPIO_DA2_H   IXPIO_DA2_HIGH_BYTE

#define IXPIO_JS      IXPIO_JUMPER_STATUS
#define IXPIO_CID     IXPIO_READ_CARD_ID

#define IXPIO_FIFO1   IXPIO_FIFO1_REGISTER
#define IXPIO_FIFO2   IXPIO_FIFO2_REGISTER
#define IXPIO_RR      IXPIO_RSTFIFO1_REGISTER
#define IXPIO_MR      IXPIO_MSC_REGISTER

#define IXPIO_XR1     IXPIO_X1_AXIS_CONTROL_REGISTER
#define IXPIO_XR2     IXPIO_X2_AXIS_CONTROL_REGISTER
#define IXPIO_XR3     IXPIO_X3_AXIS_CONTROL_REGISTER
#define IXPIO_XR4     IXPIO_X4_AXIS_CONTROL_REGISTER
#define IXPIO_XR5     IXPIO_X5_AXIS_CONTROL_REGISTER
#define IXPIO_XR6     IXPIO_X6_AXIS_CONTROL_REGISTER
	
#define IXPIO_DI1     IXPIO_X1_AXIS_DIGITAL_INPUT_REGISTER
#define IXPIO_DI2     IXPIO_X2_AXIS_DIGITAL_INPUT_REGISTER
#define IXPIO_DI3     IXPIO_X3_AXIS_DIGITAL_INPUT_REGISTER
#define IXPIO_DI4     IXPIO_X4_AXIS_DIGITAL_INPUT_REGISTER
#define IXPIO_DI5     IXPIO_X5_AXIS_DIGITAL_INPUT_REGISTER
#define IXPIO_DI6     IXPIO_X6_AXIS_DIGITAL_INPUT_REGISTER

typedef union data {
	__u8 u8;
	__u16 u16;
	__u32 u32;
	__u64 u64;
	void *ptr;
} data_t;

/* PIO signal */
typedef struct ixpio_signal {
	struct ixpio_signal *prev;
	struct ixpio_signal *next;
	int id;						/* control id */
	int sid;					/* signal id */
	pid_t pid;					/* process id */
	struct task_struct *task;			/* pointer to task structure */
	int is;						/* mask for irq source 0 disable 1 enable */
	int edge;					/* active edge for each irq source 0 for
								   negative (falling) edge 1 for positive
								   (rising) edge */
	int bedge;					/* both edges, or bipolar. 0 up to the
								   setting in variable edge. 1 does action 
								   for both negative and positive
								   edges, discards setting in variable
								   edge.  */
	/* FIXME - the edge and bedge should be merged */
} ixpio_signal_t;

/* PIO register */
typedef struct ixpio_reg {
	unsigned int id;			/* register's id */
	unsigned int value;			/* register's value for read/write */
} ixpio_reg_t;

enum {
	IXPIO_FLAG_ANALOG_PAT_START
};
/* PIO analog */
typedef struct ixpio_analog {
	struct ixpio_analog *prev;
	struct ixpio_analog *next;
	unsigned int id;			/* controlling id */
	ixpio_signal_t sig;			/* signal condiction */
	unsigned int channel;
	ixpio_flags_t flags;
	unsigned int data_size;
	unsigned int data_cur;
	data_t data;
} ixpio_analog_t;

/* PIO digital */
typedef struct ixpio_digital {
	ixpio_flags_t flags;
	data_t data;
} ixpio_digital_t;


/* PIO cards' definition */
struct ixpio_carddef {
	unsigned int id;		/* = csid, composed sub=IDs */
	unsigned int present;		/* card's present counter */
	char *module;			/* module name, if card is present then
						   load module in this name */
	char *name;			/* card's name */
};

extern struct ixpio_carddef ixpio_card[];

/* PIO device information */
enum {
	IXPIO_FLAG_DATA_START,
	IXPIO_FLAG_KEEP_ALIVE
};
typedef struct ixpio_devinfo {
	struct ixpio_devinfo *next;	/* next device (pio card) */
	struct ixpio_devinfo *prev;	/* previous device */
	struct ixpio_devinfo *next_f;	/* next device in same family */
	struct ixpio_devinfo *prev_f;	/* previous device in same family */
	int no;					/* device number (minor number) */
	unsigned int csid;			/* composed sub-IDs */
	unsigned int irq;			/* IRQ */
	unsigned long base;			/* base I/O address */
	unsigned int open;			/* open counter */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        struct cdev *cdev;
#else
	struct file_operations *fops;	/* file operations for this device */
#endif
	char name[CNL];			/* card name information */
	ixpio_signal_t *sig;		/* user signal setting for interrupt */
	unsigned int is;		/* last interrupt source */
	unsigned int is_edge;		/* edge of last interrupt source 0 for
						 * negative edge 1 for positive edge */
	unsigned int hid;		/* hardware address selection (motion card) */
	ixpio_flags_t flags;
	data_t data;
	int tmp;
} ixpio_devinfo_t;

/* IDs of ioctl commands */
enum {
	IXPIO_IOCTL_ID_GET_INFO,
	IXPIO_IOCTL_ID_SIG,
	IXPIO_IOCTL_ID_SIG_ADD,
	IXPIO_IOCTL_ID_SIG_DEL,
	IXPIO_IOCTL_ID_SIG_DEL_ALL,
	IXPIO_IOCTL_ID_REG_READ,
	IXPIO_IOCTL_ID_REG_WRITE,
	IXPIO_IOCTL_ID_DI,
	IXPIO_IOCTL_ID_DO,
	IXPIO_IOCTL_ID_AI,
	IXPIO_IOCTL_ID_AO,
	IXPIO_IOCTL_ID_AO_PAT,
	IXPIO_IOCTL_ID_AO_PAT_ADD,
	IXPIO_IOCTL_ID_AO_PAT_START,
	IXPIO_IOCTL_ID_AO_PAT_PAUSE,
	IXPIO_IOCTL_ID_AO_PAT_STOP,
	IXPIO_IOCTL_ID_AO_PAT_DEL,
	IXPIO_IOCTL_ID_AO_PAT_STATUS,
	IXPIO_IOCTL_ID_AO_PAT_RETRIEVE,
	IXPIO_IOCTL_ID_DATA_START,
	IXPIO_IOCTL_ID_DATA_STOP,
	IXPIO_IOCTL_ID_DATA_CLEAR,
	IXPIO_IOCTL_ID_KEEP_ALIVE,
	IXPIO_IOCTL_ID_NO_KEEP_ALIVE,
	IXPIO_IOCTL_ID_LAST
};

/* PIO IOCTL command */
#define IXPIO_MAGIC_NUM  0xE8	/* why? ascii codes 'P' + 'I' + 'O' */
#define IXPIO_GET_INFO   _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_GET_INFO, ixpio_devinfo_t *)

#define IXPIO_SIG          _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_SIG, ixpio_signal_t *)
#define IXPIO_SET_SIG      IXPIO_SIG  /* backward compatible */
#define IXPIO_SIG_ADD      _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_SIG_ADD, ixpio_signal_t *)
#define IXPIO_SIG_DEL      _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_SIG_DEL, unsigned int)
#define IXPIO_SIG_DEL_ALL  _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_SIG_DEL_ALL, ixpio_signal_t *)

#define IXPIO_REG_READ   _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_REG_READ, ixpio_reg_t *)
#define IXPIO_READ_REG   IXPIO_REG_READ  /* backward compatible */
#define IXPIO_REG_WRITE  _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_REG_WRITE, ixpio_reg_t *)
#define IXPIO_WRITE_REG  IXPIO_REG_WRITE  /* backward compatible */

#define IXPIO_DIGITAL_IN  _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_DI, ixpio_digital_t *)
#define IXPIO_DIGITAL_OUT _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_DO, ixpio_digital_t *)
#define IXPIO_ANALOG_IN   _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AI, ixpio_analog_t *)
#define IXPIO_ANALOG_OUT  _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AO, ixpio_analog_t *)
#define IXPIO_ANALOG_OUT_PAT           _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AO_PAT, ixpio_analog_t *)
#define IXPIO_ANALOG_OUT_PAT_ADD       _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AO_PAT_ADD, ixpio_analog_t *)
#define IXPIO_ANALOG_OUT_PAT_START     _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AO_PAT_START, unsigned int)
#define IXPIO_ANALOG_OUT_PAT_PAUSE     _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AO_PAT_PAUSE, unsigned int)
#define IXPIO_ANALOG_OUT_PAT_STOP      _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AO_PAT_STOP, unsigned int)
#define IXPIO_ANALOG_OUT_PAT_DEL       _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AO_PAT_DEL, unsigned int)
#define IXPIO_ANALOG_OUT_PAT_CLEAR     IXPIO_ANALOG_OUT_PAT_DEL  /* backward compatible */
#define IXPIO_ANALOG_OUT_PAT_STATUS    _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AO_PAT_STATUS, ixpio_analog_t *)
#define IXPIO_ANALOG_OUT_PAT_RETRIEVE  _IOR(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_AO_PAT_RETRIEVE, ixpio_analog_t *)
#define IXPIO_DATA_START   _IO(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_DATA_START)
#define IXPIO_DATA_STOP    _IO(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_DATA_STOP)
#define IXPIO_DATA_CLEAR   _IO(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_DATA_CLEAR)

#define IXPIO_KEEP_ALIVE      _IO(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_KEEP_ALIVE)
#define IXPIO_NO_KEEP_ALIVE   _IO(IXPIO_MAGIC_NUM, IXPIO_IOCTL_ID_NO_KEEP_ALIVE)

/* this macro creates composed sub-ids */
#define IXPIO_CSID(a,b,c) ( ((a)<<16) + ((b)<<8) + ((c)&0xf0) )

/* Exported Symbols */
#ifdef __KERNEL__

void *_cardname(unsigned id, int add_present);
void _del_dev(void);
ixpio_devinfo_t *_add_dev(unsigned int no, unsigned int csid, char *name, struct pci_dev *sdev);
ixpio_devinfo_t *_find_minor(int minor);
void ixpio_cleanup(void);
int ixpio_get_info(char *buf, char **start, off_t offset, int buf_size);
struct task_struct *find_task_by_pid(pid_t nr, struct pid_namespace *ns);


/* from ixpisop8r8.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisop8r8_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisop8r8_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisop8r8_release(struct inode *, struct file *);
#else
void ixpisop8r8_release(struct inode *, struct file *);
#endif

int ixpisop8r8_open(struct inode *, struct file *);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisop16r16u_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisop16r16u_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisop16r16u_release(struct inode *, struct file *);
#else
void ixpisop16r16u_release(struct inode *, struct file *);
#endif

int ixpisop16r16u_open(struct inode *, struct file *);

/* from ixpiso813.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiso813_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiso813_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiso813_release(struct inode *, struct file *);
#else
void ixpiso813_release(struct inode *, struct file *);
#endif

int ixpiso813_open(struct inode *, struct file *);

/* from ixpiso725.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiso725_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiso725_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiso725_release(struct inode *, struct file *);
#else
void ixpiso725_release(struct inode *, struct file *);
#endif

int ixpiso725_open(struct inode *, struct file *);

/* from ixpisop32c32.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisop32c32_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisop32c32_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisop32c32_release(struct inode *, struct file *);
#else
void ixpisop32c32_release(struct inode *, struct file *);
#endif
int ixpisop32c32_open(struct inode *, struct file *);

/* from ixpisop32a32.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisop32a32_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisop32a32_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisop32a32_release(struct inode *, struct file *);
#else
void ixpisop32a32_release(struct inode *, struct file *);
#endif

int ixpisop32a32_open(struct inode *, struct file *);

/* from ixpiso730a.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiso730a_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiso730a_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiso730a_release(struct inode *, struct file *);
#else
void ixpiso730a_release(struct inode *, struct file *);
#endif

int ixpiso730a_open(struct inode *, struct file *);

/* from ixpiso730.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiso730_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiso730_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiso730_release(struct inode *, struct file *);
#else
void ixpiso730_release(struct inode *, struct file *);
#endif

int ixpiso730_open(struct inode *, struct file *);

/* from ixpisoda2.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisoda2_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisoda2_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisoda2_release(struct inode *, struct file *);
#else
void ixpisoda2_release(struct inode *, struct file *);
#endif

int ixpisoda2_open(struct inode *, struct file *);

/* from ixpisop64.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisop64_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisop64_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisop64_release(struct inode *, struct file *);
#else
void ixpisop64_release(struct inode *, struct file *);
#endif

int ixpisop64_open(struct inode *, struct file *);

/* from ixpisoc64.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisoc64_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisoc64_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisoc64_release(struct inode *, struct file *);
#else
void ixpisoc64_release(struct inode *, struct file *);
#endif

int ixpisoc64_open(struct inode *, struct file *);

/* from ixpisoa64.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisoa64_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisoa64_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisoa64_release(struct inode *, struct file *);
#else
void ixpisoa64_release(struct inode *, struct file *);
#endif

int ixpisoa64_open(struct inode *, struct file *);

/* from ixpioda16.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpioda16_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpioda16_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpioda16_release(struct inode *, struct file *);
#else
void ixpioda16_release(struct inode *, struct file *);
#endif

int ixpioda16_open(struct inode *, struct file *);

/* from ixpisoenc600.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisoenc600_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisoenc600_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisoenc600_release(struct inode *, struct file *);
#else
void ixpisoenc600_release(struct inode *, struct file *);
#endif

int ixpisoenc600_open(struct inode *, struct file *);

/* from ixpiod96.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiod96_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiod96_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiod96_release(struct inode *, struct file *);
#else
void ixpiod96_release(struct inode *, struct file *);
#endif

int ixpiod96_open(struct inode *, struct file *);

/* from ixpiod64a.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiod64a_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiod64a_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiod64a_release(struct inode *, struct file *);
#else
void ixpiod64a_release(struct inode *, struct file *);
#endif

int ixpiod64a_open(struct inode *, struct file *);

/* from ixpiod64.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiod64_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiod64_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiod64_release(struct inode *, struct file *);
#else
void ixpiod64_release(struct inode *, struct file *);
#endif

int ixpiod64_open(struct inode *, struct file *);

/* from ixpiod56.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiod56_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiod56_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiod56_release(struct inode *, struct file *);
#else
void ixpiod56_release(struct inode *, struct file *);
#endif

int ixpiod56_open(struct inode *, struct file *);

/* from ixpiod48.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiod48_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiod48_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiod48_release(struct inode *, struct file *);
#else
void ixpiod48_release(struct inode *, struct file *);
#endif

int ixpiod48_open(struct inode *, struct file *);

/* from ixpiod144.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiod144_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
#else
int ixpiod144_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiod144_release(struct inode *, struct file *);
#else
void ixpiod144_release(struct inode *, struct file *);
#endif

int ixpiod144_open(struct inode *, struct file *);

/* from ixpiod168.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiod168_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiod168_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiod168_release(struct inode *, struct file *);
#else
void ixpiod168_release(struct inode *, struct file *);
#endif

int ixpiod168_open(struct inode *, struct file *);

/* from ixpio821.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpio821_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpio821_ioctl(struct inode *, struct file *, unsigned int,	unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpio821_release(struct inode *, struct file *);
#else
void ixpio821_release(struct inode *, struct file *);
#endif

int ixpio821_open(struct inode *, struct file *);

/* from ixpiod168a.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiod168a_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpiod168a_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiod168a_release(struct inode *, struct file *);
#else
void ixpiod168a_release(struct inode *, struct file *);
#endif
int ixpiod168a_open(struct inode *, struct file *);

/* from ixpisops300.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisops300_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpisops300_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisops300_release(struct inode *, struct file *);
#else
void ixpisops300_release(struct inode *, struct file *);
#endif
int ixpisops300_open(struct inode *, struct file *);

/* from ixpio.o */
void *(_pio_cardname) (int);
void (ixpio_copy_devinfo) (ixpio_devinfo_t *, ixpio_devinfo_t *);
extern ixpio_devinfo_t *ixpio_dev;
extern int ixpio_major;

/* from _proc.o */
int ixpio_proc_init(void);
void ixpio_proc_exit(void);

/* from _signal.c */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
void ixpio_signaling(unsigned int, ixpio_devinfo_t *);
#else
__inline__ void ixpio_signaling(unsigned int, ixpio_devinfo_t *);
#endif

void ixpio_clear_signals(ixpio_devinfo_t *);
int ixpio_del_signal(unsigned int, ixpio_devinfo_t *);
int ixpio_add_signal(ixpio_signal_t *, ixpio_devinfo_t *);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#define devfs_register_chrdev(a,b,c) module_register_chrdev(a,b,c)
#define devfs_unregister_chrdev(a,b) module_unregister_chrdev(a,b)
#define ixpio_init(a) init_module(a)
#define ixpio_cleanup(a) cleanup_module(a)
#endif

#endif							/* __KERNEL__ */

#endif							/* _IXPIO_H */
