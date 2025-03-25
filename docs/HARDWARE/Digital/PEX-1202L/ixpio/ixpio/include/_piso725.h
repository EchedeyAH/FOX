/* Declarations for PISO-725.

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
     INT_MASK for IMCR, IPCR, and APSR.

   v 0.1.0  7 Nov 2002 by Reed Lai
     Corrects the values of IMCR_MASK and IPCR_MASK.

     BOARD_DIO_ENABLE/DISABLE

   v 0.0.0  4 Jun 2002 by Reed Lai
     create, blah blah... */

#ifndef _PISO725_H
#define _PISO725_H

#define MODULE_NAME "piso725"

#define IO_RANGE       0x100
#define BOARD_DIO_ENABLE   0x1
#define BOARD_DIO_DISABLE  0

#define INT_MASK_SHIFT_STOP_BIT  0x100

/* offset of registers */
#define RESET_CONTROL_REG           0x00
#define AUX_CONTROL_REG             0x02
#define AUX_DATA_REG                0x03
#define INT_MASK_CONTROL_REG        0x05
#define AUX_PIN_STATUS_REG          0x07
#define INT_POLARITY_CONTROL_REG    0x2a

#define DO0_TO_DO7                  0xc0
#define DI0_TO_DI7                  0xc4

#define RCR     RESET_CONTROL_REG
#define ACR     AUX_CONTROL_REG
#define ADR     AUX_DATA_REG
#define IMCR    INT_MASK_CONTROL_REG
#define APSR    AUX_PIN_STATUS_REG
#define IPCR    INT_POLARITY_CONTROL_REG

#define DO      DO0_TO_DO7
#define DI      DI0_TO_DI7

/* mask of registers */
#define RCR_MASK        0x01
#define ACR_MASK        0xff
#define ADR_MASK        0xff
#define IMCR_MASK       0xff
#define IMCR_INT_MASK   0xff
#define APSR_MASK       0xff
#define APSR_INT_MASK   0xff
#define IPCR_MASK       0xff
#define IPCR_INT_MASK   0xff

#define DO_MASK     0xff
#define DI_MASK     0xff

#endif							/* _PISO725_H */
