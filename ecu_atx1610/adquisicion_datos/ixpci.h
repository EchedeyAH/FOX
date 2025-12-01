/* Declarations for PCI DAQ series.

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
   v 0.17.0 22 Aug 2023 by Winson Chen
     Give support for PCI-2602U

   v 0.16.2 5  May 2020 by Winson Chen
     Give support for PCI-D96/128SU

   v 0.16.1 23 Mar 2017 by Golden Wang
     To add card ID for PCI-FC16.

   v 0.16.0 22 Sep 2015 by Winson Chen
     Give support to PCIe-8622

   v 0.15.2 26 Fev 2014 by Winson Chen
     To remove driver, library for PISO-CAN/200/400 serial cards.

   v 0.15.1 1  Nov 2013 by Winson Chen
     Give support to PCI-822.

   v 0.15.0 29 Oct 2013 by Winson Chen
     Give support to PCI-826.

   v 0.14.0 1 Apr 2011 by Golden Wang
     Give support to linux kernel 2.6.37.

   v 0.13.0 12 May 2010 by Golden Wang
     To add driver for Lanner OEM I/O Card.

   v 0.12.0 16 Aug 2009 by Golden Wang
     To add driver, library for PISO-CAN200/400 Series Cards.

   v 0.11.1 16 May 2007 by Golden Wang
     Don't include "linux/cdev.h" here.

   v 0.11.0 6 Feb 2007 by Golden Wang
     Give support to linux kernel 2.6.X

   v 0.10.0 25 Jun 2003 by Reed Lai
     Defines IXPCI_PROC_FILE.

   v 0.9.0 11 Mar 2003 by Reed Lai
     Gives support to PCI-TMC12.
     Gives sub-vendor and sub-device IDs.

   v 0.8.0  9 Jan 2003 by Reed Lai
     PCI-1002.

   v 0.7.0  9 Jan 2003 by Reed Lai
     Gives support to PCI-P8R8.

   v 0.6.0  7 Jan 2003 by Reed Lai
     Gives support to PCI-1002.
     Adds base address ranges for ixpci_devinfo.

   v 0.5.0 11 Nov 2002 by Reed Lai
     ICPDAS_LICENSE
     Removes some unused symbols.

   v 0.4.2 11 Sep 2002 by Reed Lai
     Adds symbol void *(_cardname) (int,int)

   v 0.4.1 26 Jul 2002 by Reed Lai
     Just refines some codes.

   v 0.4.0 16 May 2002 by Reed Lai
     Gives support to PCI-P16R16/P16C16/P16POR16

   v 0.3.0  1 Nov 2001 by Reed Lai
     Macros for Kernel 2.2 compatibility
	 cleanup_module()
	 init_module()

   v 0.2.0 31 Oct 2001 by Reed Lai
     Macros for Kernel 2.2 compatibility
         module_register_chrdev()
	 module_unregister_chrdev()

   v 0.1.0 25 Oct 2001 by Reed Lai
     Re-filenames to ixpci.h (from pdaq.h.)
     Changes all "pdaq" to "ixpci."

   v 0.0.0 10 Apr 2001 by Reed Lai
     Create.  */
/* *INDENT-OFF* */

#ifndef _IXPCI_H
#define _IXPCI_H

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/wait.h>
//#include <linux/ioport.h>
//#include <asm/io.h>

#define ICPDAS_LICENSE "GPL"

/* General Definition */
#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

#ifndef SA_SHIRQ
#define SA_SHIRQ IRQF_SHARED
#endif

#define IXPCI_DRIVER_VERSION "0.8.19"
#define ORGANIZATION "icpdas"
#define FAMILY "ixpci"			/* name of family */
#define DEVICE_NAME "ixpci"		/* device name used in /dev and /proc */
#define DEVICE_NAME_LEN 5
#define DEVICE_NR_DEVS 4
#define DEVICE_MAJOR 0			/* dynamic allocation of major number */
#define DEVICE_MINOR 0			

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
#define IXPCI_PROC_FILE "/proc/ixpci/ixpci"
#else
#define IXPCI_PROC_FILE "/proc/ixpci"
#endif

#define PCI_BASE_ADDRESSES_NUMBER  6
#define PBAN  PCI_BASE_ADDRESSES_NUMBER

#define CARD_NAME_LENGTH  45
#define CNL  CARD_NAME_LENGTH

#define KMSG(fmt, args...) printk(KERN_INFO FAMILY ": " fmt, ## args)

/* PCI Card's ID (vendor id).(device id) */
/*
          0x 1234 5678 1234 5678
             ---- ---- ---- ----
              |    |     |    |
      vendor id    |     |    sub-device id
	       device id     sub-vendor id
*/
#define PCI_1800	0x1234567800000000
#define PCI_1802	0x1234567800000000
#define PCI_1602	0x1234567800000000
#define PCI_1602_A	0x1234567600000000
#define PCI_1202	0x1234567200000000
#define PCI_1002	0x12341002c1a20823
#define PCI_P16C16	0x12341616c1a20823
#define PCI_P16R16	0x12341616c1a20823
#define PCI_P16POR16	0x12341616c1a20823
#define PEX_P16POR16	0x12341616c1a20823
#define PCI_P8R8	0x12340808c1a20823
#define PEX_P8POR8	0x12340808c1a20823
#define PCI_TMC12	0x10b5905021299912
#define PCI_M512	0x10b5905021290512
#define PCI_M256	0x10b5905021290256
#define PCI_M128	0x10b5905021290128
#define PCI_9050EVM	0x10b5905010b59050
#define PCI_LANNER	0x1415951114150000
#define PCI_826		0x10b5300121290826
#define PCI_822         0x10b5300121290822
#define PCIe_8622	0x10ee000735778622
#define PCIe_8620	0x10ee000735778620
#define PCI_FC16        0x10b5300100FC0016
#define PCI_D96		0xe15900033577d096
#define PCI_D128	0xe15900033577d128
#define PCI_2602U	0x10b5905435772602

#define IXPCI_VENDOR(a)		((a) >> 48)
#define IXPCI_DEVICE(a)		(((a) >> 32) & 0x0ffff)
#define IXPCI_SUBVENDOR(a)	(((a) >> 16) & 0x0ffff)
#define IXPCI_SUBDEVICE(a)	((a) & 0x0ffff)

/* The chaos of name convention from hardware manual... */
enum {
	IXPCI_8254_COUNTER_0,
	IXPCI_8254_COUNTER_1,
	IXPCI_8254_COUNTER_2,
	IXPCI_8254_CONTROL_REG,
	IXPCI_SELECT_THE_ACTIVE_8254_CHIP,
	IXPCI_GENERAL_CONTROL_REG,
	IXPCI_STATUS_REG,
	IXPCI_GET_CARD_ID,
	IXPCI_AD_SOFTWARE_TRIGGER_REG,
	IXPCI_DIGITAL_INPUT_PORT,
	IXPCI_DIGITAL_OUTPUT_PORT,
	IXPCI_DIGITAL_INPUT_OUTPUT_PORTA,
	IXPCI_DIGITAL_INPUT_OUTPUT_PORTB,
	IXPCI_DIGITAL_INPUT_OUTPUT_PORTC,
	IXPCI_DIGITAL_INPUT_OUTPUT_PORTD,
	IXPCI_DIGITAL_INPUT_OUTPUT_PORTA_CONTROL_REG,
	IXPCI_DIGITAL_INPUT_OUTPUT_PORTB_CONTROL_REG,
	IXPCI_DIGITAL_INPUT_OUTPUT_PORTC_CONTROL_REG,
	IXPCI_DIGITAL_INPUT_OUTPUT_PORTD_CONTROL_REG,
	IXPCI_DIGITAL_INPUT_OUTPUT_PORTA_TO_PORTD,
	IXPCI_DO_READBACK,
	IXPCI_SET_DIO_PORTA_PORTB_CONFIGURATION,
	IXPCI_GET_DIO_JUMPER_STATUS,
	IXPCI_ANALOG_INPUT_CHANNEL_CONTROL_REG,
	IXPCI_ANALOG_INPUT_GAIN_CONTROL_REG,
	IXPCI_ANALOG_INPUT_PORT,
	IXPCI_ANALOG_OUTPUT_PORT,
	IXPCI_READ_SET_DA_CONTROL_SETTING,
	IXPCI_ANALOG_OUTPUT_CHANNEL_0,
	IXPCI_ANALOG_OUTPUT_CHANNEL_1,
	IXPCI_ANALOG_OUTPUT_CHANNEL_2,
	IXPCI_ENABLE_DISABLE_DA_CHANNEL,
	IXPCI_PCI_INTERRUPT_CONTROL_REG,
	IXPCI_CLEAR_INTERRUPT,
	IXPCI_LAST_REG,
	IXPCI_SRAM_512,
	IXPCI_BATTERY_VOLTAGE_STATUS,
	IXPCI_AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER,
	IXPCI_AD_POLLING_REGISTER,
	IXPCI_EEPROM_REGISTER,
	IXPCI_READ_COUNTER0,
	IXPCI_CLEAR_COUNTER0,
	IXPCI_READ_COUNTER1,
	IXPCI_CLEAR_COUNTER1,
	IXPCI_READ_SET_SAMPLING_RATE,
	IXPCI_READBACK_SET_CONUNT_NUMBER_FOR_MAGIC_SCAN,
	IXPCI_AD_PACER_CONTROL_SETTING,
	IXPCI_READ_AD_PACER_CONTROL_SETTING,
	IXPCI_READ_WRITE_MAGIC_SCAN_BASE_FREQUENCY_SETTING,
	IXPCI_START_STOP_MAGIC_SCAN,
	IXPCI_READ_SET_INTERRUPT_CONTROL,
	IXPCI_READ_AI_FIFO_DATA,
	IXPCI_AI_FIFO_STATUS,
	IXPCI_CLEAR_AI_FIFO,
	IXPCI_READ_AI_DATA_LOW,
	IXPCI_READ_AI_DATA_HIGH,
	IXPCI_ANALOG_INPUT_INTERNAL_CLOCK_CONTROL_STATUS,
	IXPCI_AI_DATA_ACQUISITION_START,
	IXPCI_READ_WRITE_COUNTER0_CONTROL_STATUS,
	IXPCI_READ_WRITE_COUNTER0_PERIOD_CONTROL_STATUS,
	IXPCI_READ_WRITE_COUNTER1_CONTROL_STATUS,
	IXPCI_READ_WRITE_COUNTER1_PERIOD_CONTROL_STATUS,
	IXPCI_READ_EEP,
	IXPCI_TIMER0_CHANNEL_MODE,
	IXPCI_TIMER0_SPEED_MODE,
	IXPCI_TIMER0_SELECT_CHANNEL,
	IXPCI_TIMER0_LATCH_CHANNEL,
	IXPCI_TIMER1_CHANNEL_MODE,
	IXPCI_TIMER1_SPEED_MODE,
	IXPCI_TIMER1_SELECT_CHANNEL,
	IXPCI_TIMER1_LATCH_CHANNEL,
	IXPCI_PORTA_COMPARE_VALUE_SETTING,
	IXPCI_PORTB_COMPARE_VALUE_SETTING,
	IXPCI_PORTC_COMPARE_VALUE_SETTING,
	IXPCI_PORTD_COMPARE_VALUE_SETTING,
	IXPCI_PORT_PATTERN_MATCH_CHANGE_OF_STATE,
	IXPCI_PORTA_CLEAR_CHANGE_OF_STATUS,
	IXPCI_PORTB_CLEAR_CHANGE_OF_STATUS,
	IXPCI_PORT_PATTERN_CLK,
	IXPCI_PORT_PATTERN_NUMBER,
	IXPCI_PORT_PATTERN_OUTPUT,
	IXPCI_PATTERN_DATA_SETTING,
	IXPCI_XOR_LOGIC,
	IXPCI_LATCH_COUNTER,	//20211216 : add the "IXPCI_LATCH_COUNTER" for the cmd "IXPCI_LATCH_CNT"
	IXPCI_RESET_AI_MODE,
	IXPCI_RESET_AI_FIFO_OVERFLOW_STATUS,
	IXPCI_AI_SOFTWARE_TRIGGER_CONRTOL,
	IXPCI_AI_SOFTWARE_TRIGGER_STATUS,
	IXPCI_AI_SCAN_ADDRESS,
	IXPCI_AI_CONFIGURATION_CONTROL_STATUS,
	IXPCI_SAVE_AI_CONFIGURATION,
	IXPCI_RESET_DI_FIFO,
	IXPCI_RESET_DO_FIFO,
	IXPCI_RESET_AO0_FIFO,
	IXPCI_RESET_DI_FIFO_OVERFLOW_STATUS,
	IXPCI_DIO_PORTA_TO_PORTD_CONFIGURATION,
	IXPCI_DI_FIFO_STATUS,
	IXPCI_AO_CONFIGURATION,
};

/* read & write register switch case define */
#define IXPCI_8254C0       IXPCI_8254_COUNTER_0
#define IXPCI_8254C1       IXPCI_8254_COUNTER_1
#define IXPCI_8254C2       IXPCI_8254_COUNTER_2
#define IXPCI_8254CR       IXPCI_8254_CONTROL_REG
#define IXPCI_8254_CHIP_SELECT IXPCI_SELECT_THE_ACTIVE_8254_CHIP
#define IXPCI_8254CS       IXPCI_8254_CHIP_SELECT
#define IXPCI_CONTROL_REG  IXPCI_GENERAL_CONTROL_REG
#define IXPCI_CR           IXPCI_CONTROL_REG
#define IXPCI_SR           IXPCI_STATUS_REG
#define IXPCI_READ_C0	   IXPCI_READ_COUNTER0
#define IXPCI_CLEAR_C0	   IXPCI_CLEAR_COUNTER0
#define IXPCI_READ_C1	   IXPCI_READ_COUNTER1
#define IXPCI_CLEAR_C1	   IXPCI_CLEAR_COUNTER1
#define IXPCI_CN0CS	   IXPCI_READ_WRITE_COUNTER0_CONTROL_STATUS
#define IXPCI_CN0PCS	   IXPCI_READ_WRITE_COUNTER0_PERIOD_CONTROL_STATUS
#define IXPCI_CN1CS	   IXPCI_READ_WRITE_COUNTER1_CONTROL_STATUS
#define IXPCI_CN1PCS	   IXPCI_READ_WRITE_COUNTER1_PERIOD_CONTROL_STATUS

#define IXPCI_DI           IXPCI_DIGITAL_INPUT_PORT
#define IXPCI_DO           IXPCI_DIGITAL_OUTPUT_PORT
#define IXPCI_XOR	   IXPCI_XOR_LOGIC
#define IXPCI_LATCH_CNT	   IXPCI_LATCH_COUNTER	//20211216 : add the cmd "IXPCI_LATCH_CNT"
#define IXPCI_DIOPA        IXPCI_DIGITAL_INPUT_OUTPUT_PORTA
#define IXPCI_DIOPB        IXPCI_DIGITAL_INPUT_OUTPUT_PORTB
#define IXPCI_DIOPC        IXPCI_DIGITAL_INPUT_OUTPUT_PORTC
#define IXPCI_DIOPD        IXPCI_DIGITAL_INPUT_OUTPUT_PORTD
#define IXPCI_DIOPA_CR     IXPCI_DIGITAL_INPUT_OUTPUT_PORTA_CONTROL_REG
#define IXPCI_DIOPB_CR     IXPCI_DIGITAL_INPUT_OUTPUT_PORTB_CONTROL_REG
#define IXPCI_DIOPC_CR     IXPCI_DIGITAL_INPUT_OUTPUT_PORTC_CONTROL_REG
#define IXPCI_DIOPD_CR     IXPCI_DIGITAL_INPUT_OUTPUT_PORTD_CONTROL_REG
#define IXPCI_DIO_PABCD    IXPCI_DIGITAL_INPUT_OUTPUT_PORTA_TO_PORTD
#define IXPCI_DORB	   IXPCI_DO_READBACK
#define IXPCI_PAB_CONFIG   IXPCI_SET_DIO_PORTA_PORTB_CONFIGURATION
#define IXPCI_DIO_PABCD_C  IXPCI_DIO_PORTA_TO_PORTD_CONFIGURATION
#define IXPCI_GDIO_JS      IXPCI_GET_DIO_JUMPER_STATUS
#define IXPCI_DI_RF	   IXPCI_RESET_DI_FIFO
#define IXPCI_DO_RF	   IXPCI_RESET_DO_FIFO
#define IXPCI_DI_RFOS	   IXPCI_RESET_DI_FIFO_OVERFLOW_STATUS
#define IXPCI_DI_FS	   IXPCI_DI_FIFO_STATUS

#define IXPCI_ADST         IXPCI_AD_SOFTWARE_TRIGGER_REG
#define IXPCI_AICR         IXPCI_ANALOG_INPUT_CHANNEL_CONTROL_REG
#define IXPCI_AIGR         IXPCI_ANALOG_INPUT_GAIN_CONTROL_REG
#define IXPCI_AI           IXPCI_ANALOG_INPUT_PORT
#define IXPCI_AD           IXPCI_AI
#define IXPCI_AO	   IXPCI_ANALOG_OUTPUT_PORT
#define IXPCI_RW_DA	   IXPCI_AO
#define IXPCI_RS_DA_CS     IXPCI_READ_SET_DA_CONTROL_SETTING
#define IXPCI_AO0	   IXPCI_ANALOG_OUTPUT_CHANNEL_0
#define IXPCI_AO1          IXPCI_ANALOG_OUTPUT_CHANNEL_1
#define IXPCI_DA1          IXPCI_AO1
#define IXPCI_AO2          IXPCI_ANALOG_OUTPUT_CHANNEL_2
#define IXPCI_DA2          IXPCI_AO2
#define IXPCI_ED_DA_CH     IXPCI_ENABLE_DISABLE_DA_CHANNEL
#define IXPCI_ADGCR	   IXPCI_AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER
#define IXPCI_ADPR	   IXPCI_AD_POLLING_REGISTER
#define IXPCI_RS_SR	   IXPCI_READ_SET_SAMPLING_RATE
#define IXPCI_RS_CN_MS	   IXPCI_READBACK_SET_CONUNT_NUMBER_FOR_MAGIC_SCAN
#define IXPCI_AI_PC	   IXPCI_AD_PACER_CONTROL_SETTING
#define IXPCI_RAI_PC	   IXPCI_READ_AD_PACER_CONTROL_SETTING
#define IXPCI_RW_MS_BF_CS  IXPCI_READ_WRITE_MAGIC_SCAN_BASE_FREQUENCY_SETTING
#define IXPCI_SS_MS	   IXPCI_START_STOP_MAGIC_SCAN
#define IXPCI_AI_FD	   IXPCI_READ_AI_FIFO_DATA
#define IXPCI_AI_FS	   IXPCI_AI_FIFO_STATUS
#define IXPCI_AI_CF	   IXPCI_CLEAR_AI_FIFO
#define IXPCI_AI_L	   IXPCI_READ_AI_DATA_LOW
#define IXPCI_AI_H	   IXPCI_READ_AI_DATA_HIGH
#define IXPCI_AI_ICCS	   IXPCI_ANALOG_INPUT_INTERNAL_CLOCK_CONTROL_STATUS
#define IXPCI_AI_DAS	   IXPCI_AI_DATA_ACQUISITION_START
#define IXPCI_AI_RM	   IXPCI_RESET_AI_MODE
#define IXPCI_AI_RFOS	   IXPCI_RESET_AI_FIFO_OVERFLOW_STATUS
#define IXPCI_AI_STC	   IXPCI_AI_SOFTWARE_TRIGGER_CONRTOL
#define IXPCI_AI_STS	   IXPCI_AI_SOFTWARE_TRIGGER_STATUS
#define IXPCI_AI_SA	   IXPCI_AI_SCAN_ADDRESS
#define IXPCI_AI_CC	   IXPCI_AI_CONFIGURATION_CONTROL_STATUS
#define IXPCI_SAI_C	   IXPCI_SAVE_AI_CONFIGURATION
#define IXPCI_AO0_RF	   IXPCI_RESET_AO0_FIFO
#define IXPCI_AO_CFG	   IXPCI_AO_CONFIGURATION

#define IXPCI_T0CM         IXPCI_TIMER0_CHANNEL_MODE
#define IXPCI_T0SM         IXPCI_TIMER0_SPEED_MODE
#define IXPCI_T0SC         IXPCI_TIMER0_SELECT_CHANNEL
#define IXPCI_T0LC         IXPCI_TIMER0_LATCH_CHANNEL
#define IXPCI_T1CM         IXPCI_TIMER1_CHANNEL_MODE
#define IXPCI_T1SM         IXPCI_TIMER1_SPEED_MODE
#define IXPCI_T1SC         IXPCI_TIMER1_SELECT_CHANNEL
#define IXPCI_T1LC         IXPCI_TIMER1_LATCH_CHANNEL
#define IXPCI_PA_CVS	   IXPCI_PORTA_COMPARE_VALUE_SETTING
#define IXPCI_PB_CVS	   IXPCI_PORTB_COMPARE_VALUE_SETTING
#define IXPCI_PC_CVS	   IXPCI_PORTC_COMPARE_VALUE_SETTING
#define IXPCI_PD_CVS	   IXPCI_PORTD_COMPARE_VALUE_SETTING
#define IXPCI_PPM_CS	   IXPCI_PORT_PATTERN_MATCH_CHANGE_OF_STATE
#define IXPCI_PA_CCS	   IXPCI_PORTA_CLEAR_CHANGE_OF_STATUS
#define IXPCI_PB_CCS	   IXPCI_PORTB_CLEAR_CHANGE_OF_STATUS
#define IXPCI_PPC	   IXPCI_PORT_PATTERN_CLK
#define IXPCI_PPN	   IXPCI_PORT_PATTERN_NUMBER
#define IXPCI_PPO	   IXPCI_PORT_PATTERN_OUTPUT
#define IXPCI_PDS	   IXPCI_PATTERN_DATA_SETTING

#define IXPCI_GCID         IXPCI_GET_CARD_ID
#define IXPCI_PICR         IXPCI_PCI_INTERRUPT_CONTROL_REG
#define IXPCI_CI           IXPCI_CLEAR_INTERRUPT
#define IXPCI_SRAM512      IXPCI_SRAM_512
#define IXPCI_BATTERY_STATUS IXPCI_BATTERY_VOLTAGE_STATUS
#define IXPCI_EEP	   IXPCI_EEPROM_REGISTER
#define IXPCI_RS_IC	   IXPCI_READ_SET_INTERRUPT_CONTROL

/*IXPCI global header*/

/* IXPCI structure for signal conditions */
typedef struct ixpci_signal {
	int sid;			/* signal id */
	pid_t pid;			/* process id */
	struct task_struct *task;	/* pointer to task structure */
	int is;				/* mask for irq source 0 disable 1 enable */
	int edge;			/* active edge for each irq source 0 for
					   negative (falling) edge 1 for positive
					   (rising) edge */
	int bedge;			/* both edges, or bipolar. 0 up to the
					   setting in variable edge. 1 does action 
					   for both negative and positive
					   edges, discards setting in variable edge.*/
} ixpci_signal_t;

/* IXPCI structure for register */
typedef struct ixpci_reg {
	unsigned int id;		/* register's id */
	unsigned int value;		/* register's value for read/write */
	unsigned int sram_off;		/* sram offset for sram card */
	unsigned int id_offset;		/* register's id and count it's offset, first used in pci_d96 bar1 */
	int mode;
} ixpci_reg_t;

//typedef struct cnafifo {
//        int head;
//        int tail;
//        int status;
//        int active;
//        char free[MAX_BUFSIZE];
//        canmsg_t data[MAX_BUFSIZE];
//} canfifo_t;

/* register operation mode */
enum {
	IXPCI_RM_RAW,			/* read/write directly without data mask */
	IXPCI_RM_NORMAL,		/* read/write directly */
	IXPCI_RM_READY,			/* blocks before ready */
	IXPCI_RM_TRIGGER,		/* do software trigger before ready (blocked) */
	IXPCI_RM_LAST_MODE
};

/* IXPCI cards' definition */
struct ixpci_carddef {
	__u64 id;			/* composed sub-ids */
	unsigned int present;		/* card's present counter */
	char *module;			/* module name, if card is present then load module in this name */
	char *name;			/* card's name */
};

extern struct ixpci_carddef ixpci_card[];

/* IXPCI device information for found cards' list */
typedef struct ixpci_devinfo {
	struct ixpci_devinfo *next;	/* next device (ixpci card) */
	struct ixpci_devinfo *prev;	/* previous device */
	struct ixpci_devinfo *next_f;	/* next device in same family */
	struct ixpci_devinfo *prev_f;	/* previous device in same family */
	unsigned int cno;		/* card number */
	unsigned int no;		/* device number (minor number) */
	__u64 id;			/* card's id */
	unsigned int irq;		/* interrupt */
	unsigned long base[PBAN];	/* base I/O addresses */
	unsigned int range[PBAN];	/* ranges for each I/O address */
	void *ioaddr[PBAN];		/* mmio address */
	unsigned int open;		/* open counter */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        struct cdev *cdev;
#else
        struct file_operations *fops;   /* file operations for this device */
#endif
	char name[CNL];			/* card name information */
	struct ixpci_signal sig;	/* user signaling for interrupt */
	char AI_ch;			/* card's AI channel numbers */
	char AO_ch;			/* card's AO channel numbers */

} ixpci_devinfo_t;

/* IOCTL command IDs */
enum {
	IXPCI_IOCTL_ID_RESET,
	IXPCI_IOCTL_ID_GET_INFO,
	IXPCI_IOCTL_ID_SET_SIG,
	IXPCI_IOCTL_ID_READ_REG,
	IXPCI_IOCTL_ID_WRITE_REG,
	IXPCI_IOCTL_ID_TIME_SPAN,
	IXPCI_IOCTL_ID_DI,
	IXPCI_IOCTL_ID_DO,
	IXPCI_IOCTL_ID_IRQ_ENABLE,
	IXPCI_IOCTL_ID_IRQ_DISABLE,
	IXPCI_IOCTL_ID_XOR,
	IXPCI_IOCTL_ID_XOR_INPUT,
	IXPCI_IOCTL_ID_LAST_ITEM
};

/* IXPCI IOCTL command */
#define IXPCI_MAGIC_NUM  0x26	/* why? ascii codes 'P' + 'D' + 'A' + 'Q' */
#define IXPCI_GET_INFO   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_GET_INFO, ixpci_devinfo_t *)

#define IXPCI_SET_SIG    _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_SET_SIG, ixpci_signal_t *)

#define IXPCI_READ_REG   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_READ_REG, ixpci_reg_t *)
#define IXPCI_WRITE_REG  _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_WRITE_REG, ixpci_reg_t *)
#define IXPCI_TIME_SPAN  _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_TIME_SPAN, int)
#define IXPCI_WAIT       IXPCI_TIME_SPAN
#define IXPCI_DELAY      IXPCI_TIME_SPAN
#define IXPCI_BLOCK      IXPCI_TIME_SPAN
#define IXPCI_RESET      _IO(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_RESET)

#define IXPCI_IOCTL_DI   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_DI, void *)
#define IXPCI_IOCTL_DO   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_DO, void *)

#define IXPCI_IRQ_ENABLE  _IO(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_IRQ_ENABLE)
#define IXPCI_IRQ_DISABLE  _IO(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_IRQ_DISABLE)
#define IXPCI_IOCTL_XOR   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_XOR, void *)
#define IXPCI_IOCTL_XOR_IN   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_XOR_INPUT, void *)

/* Exported Symbols */
#ifdef __KERNEL__

void _disable_irq(ixpci_devinfo_t *);
void _enable_irq(ixpci_devinfo_t * );
int _clear_int(ixpci_devinfo_t * );
irqreturn_t irq_handler(int , void *);
void _del_dev(void);
ixpci_devinfo_t *_add_dev(unsigned int , unsigned int , __u64 , char *, struct pci_dev *);
int _find_dev(void);
ixpci_devinfo_t *_find_minor(int);
void ixpci_cleanup(void);
int ixpci_get_info(char *, char **, off_t , int );
ixpci_devinfo_t *_align_minor(int minor);

/* from ixpcitmc12.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcitmc12_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpcitmc12_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcitmc12_release(struct inode *, struct file *);
#else
void ixpcitmc12_release(struct inode *, struct file *);
#endif
int ixpcitmc12_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpcip16x16.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcip16x16_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpcip16x16_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcip16x16_release(struct inode *, struct file *);
#else
void ixpcip16x16_release(struct inode *, struct file *);
#endif
int ixpcip16x16_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpcip8r8.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcip8r8_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpcip8r8_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcip8r8_release(struct inode *, struct file *);
#else
void ixpcip8r8_release(struct inode *, struct file *);
#endif
int ixpcip8r8_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpci1800.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci1800_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpci1800_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1800_release(struct inode *, struct file *);
#else
void ixpci1800_release(struct inode *, struct file *);
#endif
int ixpci1800_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpci1202.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci1202_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpci1202_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1202_release(struct inode *, struct file *);
#else
void ixpci1202_release(struct inode *, struct file *);
#endif
int ixpci1202_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpci1002.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci1002_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpci1002_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1002_release(struct inode *, struct file *);
#else
void ixpci1002_release(struct inode *, struct file *);
#endif
int ixpci1002_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpci1602.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci1602_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpci1602_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1602_release(struct inode *, struct file *);
#else
void ixpci1602_release(struct inode *, struct file *);
#endif
int ixpci1602_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpcim512.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcim512_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpcim512_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcim512_release(struct inode *, struct file *);
#else
void ixpcim512_release(struct inode *, struct file *);
#endif
int ixpcim512_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpcilanner.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcilanner_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpcilanner_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcilanner_release(struct inode *, struct file *);
#else
void ixpcilanner_release(struct inode *, struct file *);
#endif
int ixpcilanner_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpci826.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci826_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpci826_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci826_release(struct inode *, struct file *);
#else
void ixpci826_release(struct inode *, struct file *);
#endif
int ixpci826_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpcifc16.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcifc16_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpcifc16_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcifc16_release(struct inode *, struct file *);
#else
void ixpcifc16_release(struct inode *, struct file *);
#endif
int ixpcifc16_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpci822.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci822_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpci822_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci822_release(struct inode *, struct file *);
#else
void ixpci822_release(struct inode *, struct file *);
#endif
int ixpci822_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpcie8622.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcie8622_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpcie8622_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcie8622_release(struct inode *, struct file *);
#else
void ixpcie8622_release(struct inode *, struct file *);
#endif
int ixpcie8622_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
/* from ixpcid96.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcid96_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpcid96_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcid96_release(struct inode *, struct file *);
#else
void ixpcid96_release(struct inode *, struct file *);
#endif
int ixpcid96_open(struct inode *, struct file *);

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
/* from ixpci2602u.o */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci2602_ioctl(struct file *, unsigned int, unsigned long);
#else
int ixpci2602_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci2602_release(struct inode *, struct file *);
#else
void ixpci2602_release(struct inode *, struct file *);
#endif
int ixpci2602_open(struct inode *, struct file *);

//----------------------------------------------------------------------------
//
/* from ixpci.o */
void *(_cardname) (__u64, int);
void *(_pci_cardname) (__u64);
void (ixpci_copy_devinfo) (ixpci_devinfo_t *, ixpci_devinfo_t *);
extern ixpci_devinfo_t *ixpci_dev;
extern int ixpci_major;
//extern int ixpci_total_boards;

/* from _proc.o */
int ixpci_proc_init(void);
void ixpci_proc_exit(void);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#define devfs_register_chrdev(a,b,c) module_register_chrdev(a,b,c)
#define devfs_unregister_chrdev(a,b) module_unregister_chrdev(a,b)
#define ixpci_init(a) init_module(a)
#define ixpci_cleanup(a) cleanup_module(a)
#endif

#endif							/* __KERNEL__ */

#endif							/* _IXPCI_H */
/* *INDENT-ON* */
