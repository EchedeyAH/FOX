/* Declarations for PIO-D144.

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

   v 0.1.2  9 Jul 2003 by Reed Lai
     Uses _8DR instead of RW8DR.

   v 0.1.1 18 Dec 2002 by Reed Lai
     INT_MASK for APSR, IMCR, and IPCR.

   v 0.1.0  7 Nov 2002 by Reed Lai
     BOARD_DIO_ENABLE/DISABLE

   v 0.0.0 26 Sep 2000 by Reed Lai
     create, blah blah... */

#ifndef _PIOD144_H
#define _PIOD144_H

#define MODULE_NAME "piod144"

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
#define _8BIT_DATA_REG              0xc0
#define ACTIVE_IO_PORT_CONTROL_REG  0xc4
#define IO_SELECTION_CONTROL_REG_A  0xc8
#define IO_SELECTION_CONTROL_REG_B  0xcc
#define IO_SELECTION_CONTROL_REG_C  0xd0

#define RCR     RESET_CONTROL_REG
#define ACR     AUX_CONTROL_REG
#define ADR     AUX_DATA_REG
#define IMCR    INT_MASK_CONTROL_REG
#define APSR    AUX_PIN_STATUS_REG
#define IPCR    INT_POLARITY_CONTROL_REG
#define _8DR    _8BIT_DATA_REG
#define AIOPCR  ACTIVE_IO_PORT_CONTROL_REG
#define IOSCRA  IO_SELECTION_CONTROL_REG_A
#define IOSCRB  IO_SELECTION_CONTROL_REG_B
#define IOSCRC  IO_SELECTION_CONTROL_REG_C

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

#define _8DR_MASK   0xff
#define AIOPCR_MASK 0xff
#define IOSCRA_MASK 0x3f
#define IOSCRB_MASK 0x3f
#define IOSCRC_MASK 0x3f

#endif							/* _PIOD144_H */
