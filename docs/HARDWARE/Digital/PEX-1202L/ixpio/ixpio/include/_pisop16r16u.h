/* PISO-P8R8U/P16R16U items

   Author: Moki Matsushima

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

   v 0.0.0 25 Nov 2002 by Reed Lai
     Create. */

#ifndef _PISOP16R16U_H
#define _PISOP16R16U_H

#define MODULE_NAME "ixpisoP16R16U"

#define IO_RANGE       0x100
#define BOARD_DIO_ENABLE   1
#define BOARD_DIO_DISABLE  0

/* registers' offsets */
#define RESET_CONTROL_REG	0x00
#define DIO0_TO_DIO7		0xc0
#define DIO8_TO_DIO15		0xc4
#define CARD_ID			0x07

#define RCR  RESET_CONTROL_REG
#define DIO_A   DIO0_TO_DIO7
#define DIO_B   DIO8_TO_DIO15
#define CID	CARD_ID

/* registers' masks */
#define RCR_MASK	0xff
#define DIO_A_MASK	0xff
#define DIO_B_MASK	0xff
#define CID_MASK	0x3c

#endif							/* _PISOP8R8_H */
