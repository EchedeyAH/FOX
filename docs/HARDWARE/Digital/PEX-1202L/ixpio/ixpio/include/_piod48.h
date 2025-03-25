/* Declarations for PIO-D48.

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

   *** Do not include this file in your program ***

   v 0.1.1 18 Dec 2002 by Reed Lai
     INT_MASK for APSR, IMCR, and IPCR.

   v 0.1.0 7 Nov 2002 by Reed Lai
     BOARD_DIO_ENABLE/DISABLE

   v 0.0.0 7 Nov 2000 by Reed Lai
     create, blah blah... */

#ifndef _PIOD48_H
#define _PIOD48_H

#define MODULE_NAME "piod48"

#define IO_RANGE       0x100
#define BOARD_DIO_ENABLE   0x1
#define BOARD_DIO_DISABLE  0

#define INT_MASK_SHIFT_STOP_BIT  0x10

/* offset of registers */
#define RESET_CONTROL_REG           0x00
#define AUX_CONTROL_REG             0x02
#define AUX_DATA_REG                0x03
#define INT_MASK_CONTROL_REG        0x05
#define AUX_PIN_STATUS_REG          0x07
#define INT_POLARITY_CONTROL_REG    0x2a

#define READ_WRITE_8255_1_PORT_A        0xc0
#define READ_WRITE_8255_1_PORT_B        0xc4
#define READ_WRITE_8255_1_PORT_C        0xc8
#define READ_WRITE_8255_1_CONTROL_WORD  0xcc

#define READ_WRITE_8255_2_PORT_A        0xd0
#define READ_WRITE_8255_2_PORT_B        0xd4
#define READ_WRITE_8255_2_PORT_C        0xd8
#define READ_WRITE_8255_2_CONTROL_WORD  0xdc

#define READ_WRITE_8254_COUNTER_0     0xe0
#define READ_WRITE_8254_COUNTER_1     0xe4
#define READ_WRITE_8254_COUNTER_2     0xe8
#define READ_WRITE_8254_CONTROL_WORD  0xec

#define READ_WRITE_CLOCK_INT_CONTROL_REG  0xf0
#define CARD_ID				  0xf4

#define RCR     RESET_CONTROL_REG
#define ACR     AUX_CONTROL_REG
#define ADR     AUX_DATA_REG
#define IMCR    INT_MASK_CONTROL_REG
#define APSR    AUX_PIN_STATUS_REG
#define IPCR    INT_POLARITY_CONTROL_REG

#define RW82551PA  READ_WRITE_8255_1_PORT_A
#define RW82551PB  READ_WRITE_8255_1_PORT_B
#define RW82551PC  READ_WRITE_8255_1_PORT_C
#define RW82551CW  READ_WRITE_8255_1_CONTROL_WORD

#define RW82552PA  READ_WRITE_8255_2_PORT_A
#define RW82552PB  READ_WRITE_8255_2_PORT_B
#define RW82552PC  READ_WRITE_8255_2_PORT_C
#define RW82552CW  READ_WRITE_8255_2_CONTROL_WORD

#define RW8254C0  READ_WRITE_8254_COUNTER_0
#define RW8254C1  READ_WRITE_8254_COUNTER_1
#define RW8254C2  READ_WRITE_8254_COUNTER_2
#define RW8254CW  READ_WRITE_8254_CONTROL_WORD

#define RWCICR    READ_WRITE_CLOCK_INT_CONTROL_REG
#define CICR      RWCICR
#define CID	  CARD_ID

/* mask of registers */
#define RCR_MASK      0x01
#define ACR_MASK      0xff
#define ADR_MASK      0xff
#define IMCR_MASK     0x0f
#define IMCR_INT_MASK 0x0f
#define APSR_MASK     0xff
#define APSR_INT_MASK 0x0f
#define IPCR_MASK     0x0f
#define IPCR_INT_MASK 0x0f

#define RW82551PA_MASK  0xff
#define RW82551PB_MASK  0xff
#define RW82551PC_MASK  0xff
#define RW82551CW_MASK  0x9b

#define RW82552PA_MASK  0xff
#define RW82552PB_MASK  0xff
#define RW82552PC_MASK  0xff
#define RW82552CW_MASK  0x9b

#define RW8254C0_MASK  0xff
#define RW8254C1_MASK  0xff
#define RW8254C2_MASK  0xff
#define RW8254CW_MASK  0xff

#define RWCICR_MASK  0x3f
#define CICR_MASK    RWCICR_MASK
#define CID_MASK       0x0f

#endif							/* _PIOD48_H */
