/* Declarations for PIO-D64.

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

   v 0.0.0  8 Jul 2003 by Reed Lai
     Acknowledged, proceed to dock 2... */

/* *INDENT-OFF* */

#ifndef _PIOD64_H
#define _PIOD64_H

#define MODULE_NAME "piod64"

#define IO_RANGE       0xff
#define BOARD_DIO_ENABLE   0x1
#define BOARD_DIO_DISABLE  0

#define INT_MASK_SHIFT_STOP_BIT  0x08  // three channels available

/* offset of registers */
#define RESET_CONTROL_REG         0x00
#define AUX_CONTROL_REG           0x02
#define AUX_DATA_REG              0x03
#define INT_MASK_CONTROL_REG      0x05
#define AUX_PIN_STATUS_REG        0x07
#define INT_POLARITY_CONTROL_REG  0x2a

#define DIO0_TO_DIO7              0xc0
#define DIO8_TO_DIO15             0xc4
#define DIO16_TO_DIO23            0xc8
#define DIO24_TO_DIO31            0xcc

#define _8254_1_COUNTER_0         0xd0
#define _8254_1_COUNTER_1         0xd4
#define _8254_1_COUNTER_2         0xd8
#define _8254_1_CONTROL_WORD      0xdc

#define _8254_2_COUNTER_0         0xe0
#define _8254_2_COUNTER_1         0xe4
#define _8254_2_COUNTER_2         0xe8
#define _8254_2_CONTROL_WORD      0xec

#define RCR     RESET_CONTROL_REG
#define ACR     AUX_CONTROL_REG
#define ADR     AUX_DATA_REG
#define IMCR    INT_MASK_CONTROL_REG
#define APSR    AUX_PIN_STATUS_REG
#define IPCR    INT_POLARITY_CONTROL_REG

#define DIO_A   DIO0_TO_DIO7
#define DIO_B   DIO8_TO_DIO15
#define DIO_C   DIO16_TO_DIO23
#define DIO_D   DIO24_TO_DIO31

#define _82541C0  _8254_1_COUNTER_0
#define _82541C1  _8254_1_COUNTER_1
#define _82541C2  _8254_1_COUNTER_2
#define _82541CW  _8254_1_CONTROL_WORD

#define _82542C0  _8254_2_COUNTER_0
#define _82542C1  _8254_2_COUNTER_1
#define _82542C2  _8254_2_COUNTER_2
#define _82542CW  _8254_2_CONTROL_WORD

/* mask of registers */
#define RCR_MASK      0xff
#define ACR_MASK      0xff
#define ADR_MASK      0xff
#define IMCR_MASK     0x0f
#define IMCR_INT_MASK 0x0f
#define APSR_MASK     0xff
#define APSR_INT_MASK 0x0f
#define IPCR_MASK     0x0f
#define IPCR_INT_MASK 0x0f

#define DIO_A_MASK    0xff
#define DIO_B_MASK    0xff
#define DIO_C_MASK    0xff
#define DIO_D_MASK    0xff

#define _82541C0_MASK 0xff
#define _82541C1_MASK 0xff
#define _82541C2_MASK 0xff
#define _82541CW_MASK 0xff

#define _82542C0_MASK 0xff
#define _82542C1_MASK 0xff
#define _82542C2_MASK 0xff
#define _82542CW_MASK 0xff

#endif							/* _PIOD64_H */
