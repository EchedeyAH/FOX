/* Declarations for PIO-D96.

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

   v 0.0.1 18 Dec 2002 by Reed Lai
     INT_MASK for IPCR, IMCR, and APSR.

   v 0.0.0 26 Nov 2002 by Reed Lai
     create, blah blah... */

#ifndef _PIOD96_H
#define _PIOD96_H

#define MODULE_NAME "piod96"

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

#define PORT_0                      0xc0
#define PORT_1                      0xc4
#define PORT_2                      0xc8
#define PORT_CONF_A                 0xcc

#define PORT_3                      0xd0
#define PORT_4                      0xd4
#define PORT_5                      0xd8
#define PORT_CONF_B                 0xdc

#define PORT_6                      0xe0
#define PORT_7                      0xe4
#define PORT_8                      0xe8
#define PORT_CONF_C                 0xec

#define PORT_9                      0xf0
#define PORT_10                     0xf4
#define PORT_11                     0xf8
#define PORT_CONF_D                 0xfc

#define RCR     RESET_CONTROL_REG
#define ACR     AUX_CONTROL_REG
#define ADR     AUX_DATA_REG
#define IMCR    INT_MASK_CONTROL_REG
#define APSR    AUX_PIN_STATUS_REG
#define IPCR    INT_POLARITY_CONTROL_REG

#define P0      PORT_0
#define P1      PORT_1
#define P2      PORT_2
#define PCA     PORT_CONF_A

#define P3      PORT_3
#define P4      PORT_4
#define P5      PORT_5
#define PCB     PORT_CONF_B

#define P6      PORT_6
#define P7      PORT_7
#define P8      PORT_8
#define PCC     PORT_CONF_C

#define P9      PORT_9
#define P10     PORT_10
#define P11     PORT_11
#define PCD     PORT_CONF_D

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

#define P0_MASK     0xff
#define P1_MASK     0xff
#define P2_MASK     0xff
#define PCA_MASK    0x07

#define P3_MASK     0xff
#define P4_MASK     0xff
#define P5_MASK     0xff
#define PCB_MASK    0x07

#define P6_MASK     0xff
#define P7_MASK     0xff
#define P8_MASK     0xff
#define PCC_MASK    0x07

#define P9_MASK     0xff
#define P10_MASK    0xff
#define P11_MASK    0xff
#define PCD_MASK    0x07

#endif							/* _PIOD96_H */
