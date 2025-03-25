/* Declarations for PISO-813.

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

   v 0.0.0 14 Nov 2002 by Reed Lai
     create, blah blah... */

#ifndef _PISO813_H
#define _PISO813_H

#define MODULE_NAME "piso813"

#define IO_RANGE       0x100
#define BOARD_DIO_ENABLE   0x1
#define BOARD_DIO_DISABLE  0

/* offset of registers */
#define RESET_CONTROL_REG               0
#define AD_LOW_BYTE_DATA_REG            0xd0
#define AD_HIGH_BYTE_DATA_REG           0xd4
#define MULTIPLEXER_CHANNEL_SELECT_REG  0xe0
#define PGA_GAIN_CODE_REG               0xe4
#define AD_TRIGGER_CONTROL_REG          0xf0
#define CARD_ID				0xfc

#define RCR     RESET_CONTROL_REG
#define ADL     AD_LOW_BYTE_DATA_REG
#define ADH     AD_HIGH_BYTE_DATA_REG
#define MCSR    MULTIPLEXER_CHANNEL_SELECT_REG
#define PGCR    PGA_GAIN_CODE_REG
#define ADTCR   AD_TRIGGER_CONTROL_REG
#define CID	CARD_ID

/* mask of registers */
#define RCR_MASK    0x01
#define ADL_MASK    0xff
#define ADH_MASK    0x1f

#define AD_STATUS_BIT  0x10

#define MCSR_MASK   0x1f
#define PGCR_MASK   0x07
#define ADTCR_MASK  0xff
#define CID_MASK    0x0f

#endif							/* _PISO813_H */
