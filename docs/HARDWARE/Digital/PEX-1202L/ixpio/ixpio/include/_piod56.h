/* Declarations for PIO-D24/56.

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
     INT_MASK for IPCR, IMCR, and APSR.

   v 0.1.0  7 Nov 2002 by Reed Lai
     BOARD_DIO_ENABLE/DISABLE

   v 0.0.0 31 Oct 2002 by Reed Lai
     steal from _piod48.h, blah blah... */

#ifndef _PIOD56_H
#define _PIOD56_H

#define MODULE_NAME "piod56"

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

#define PORT0           0xc0
#define PORT1           0xc4
#define PORT2           0xc8
#define PORTCONF        0xcc

#define CON1_LOW_BYTE   0xd0
#define CON1_HIGH_BYTE  0xd4
#define CON2_LOW_BYTE   0xd0
#define CON2_HIGH_BYTE  0xd4

#define RCR     RESET_CONTROL_REG
#define ACR     AUX_CONTROL_REG
#define ADR     AUX_DATA_REG
#define IMCR    INT_MASK_CONTROL_REG
#define APSR    AUX_PIN_STATUS_REG
#define IPCR    INT_POLARITY_CONTROL_REG

#define CON1L  CON1_LOW_BYTE
#define CON1H  CON1_HIGH_BYTE
#define CON2L  CON2_LOW_BYTE
#define CON2H  CON2_HIGH_BYTE

/* mask of registers */
#define RCR_MASK       0x01
#define ACR_MASK       0xff
#define ADR_MASK       0xff
#define IMCR_MASK      0x0f
#define IMCR_INT_MASK  0x0f
#define APSR_MASK      0xff
#define APSR_INT_MASK  0x0f
#define IPCR_MASK      0x0f
#define IPCR_INT_MASK  0x0f

#define PORT0_MASK     0xff
#define PORT1_MASK     0xff
#define PORT2_MASK     0xff
#define PORTCONF_MASK  0xff

#define CON1L_MASK  0xff
#define CON1H_MASK  0xff
#define CON2L_MASK  0xff
#define CON2H_MASK  0xff

#endif							/* _PIOD56_H */
