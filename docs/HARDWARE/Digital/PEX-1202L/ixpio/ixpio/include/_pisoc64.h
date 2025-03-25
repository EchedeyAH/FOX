/* PISO-C64 items

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
     BOARD_DIO_ENABLE/DISABLE

   v 0.0.0  3 May 2002 by Reed Lai
     Create. */

#ifndef _PISOC64_H
#define _PISOC64_H

#define MODULE_NAME "ixpisoC64"

#define IO_RANGE       0x100
#define BOARD_DIO_ENABLE   0x1
#define BOARD_DIO_DISABLE  0

#define INT_MASK_SHIFT_STOP_BIT  1

/* registers' offsets */
#define RESET_CONTROL_REG           0x00
#define AUX_CONTROL_REG             0x02
#define AUX_DATA_REG                0x03
#define INT_MASK_CONTROL_REG        0x05
#define AUX_PIN_STATUS_REG          0x07
#define INT_POLARITY_CONTROL_REG    0x2a

#define DO0_TO_DO7                0xc0
#define DO8_TO_DO15               0xc4
#define DO16_TO_DO23              0xc8
#define DO24_TO_DO31              0xcc
#define DO32_TO_DO39              0xd0
#define DO40_TO_DO47              0xd4
#define DO48_TO_DO55              0xd8
#define DO56_TO_DO63              0xdc

#define RCR     RESET_CONTROL_REG
#define ACR     AUX_CONTROL_REG
#define ADR     AUX_DATA_REG
#define IMCR    INT_MASK_CONTROL_REG
#define APSR    AUX_PIN_STATUS_REG
#define ASR     APSR
#define IPCR    INT_POLARITY_CONTROL_REG

#define DO_A   DO0_TO_DO7
#define DO_B   DO8_TO_DO15
#define DO_C   DO16_TO_DO23
#define DO_D   DO24_TO_DO31
#define DO_E   DO32_TO_DO39
#define DO_F   DO40_TO_DO47
#define DO_G   DO48_TO_DO55
#define DO_H   DO56_TO_DO63

/* registers' masks */
#define RCR_MASK       0x01
#define ACR_MASK       0xff
#define ADR_MASK       0xff
#define IMCR_MASK      0
#define IMCR_INT_MASK  0
#define APSR_MASK      0xff
#define APSR_INT_MASK  0

#define DO_A_MASK   0xff
#define DO_B_MASK   0xff
#define DO_C_MASK   0xff
#define DO_D_MASK   0xff
#define DO_E_MASK   0xff
#define DO_F_MASK   0xff
#define DO_G_MASK   0xff
#define DO_H_MASK   0xff

#endif							/* _PISOC64_H */
