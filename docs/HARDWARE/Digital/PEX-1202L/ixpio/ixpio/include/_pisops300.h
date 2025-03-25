/* PISO-PS300 items

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

#ifndef _PISOPS300_H
#define _PISOPS300_H

#define MODULE_NAME "ixpisops300"

#define IO_RANGE  	   0x100
#define BOARD_DIO_ENABLE   0x1
#define BOARD_DIO_DISABLE  0

#define INT_MASK_SHIFT_STOP_BIT  0x04

/* offset of registers */
#define FIFO1_REGISTER		    0xc0
#define FIFO2_REGISTER		    0xc0
#define RSTFIFO1_REGISTER  	    0xc4
#define MSC_REGISTER		    0xc4
#define DI			    0xc8
#define DO			    0xc8

/* for tiger's register */
#define TJ_CNTL			    0x00  //set reset and others
#define TJ_AUXC			    0x02  //AUX pin control register 1:output
#define TJ_AUXD			    0x03  //AUX pin data register
#define TJ_MASK			    0x05  //interrupt mask
#define TJ_AUX			    0x07  //AUX pin status
#define TJ_POLARITY		    0x2a  //AUX pin polarity

#define FIFO1	FIFO1_REGISTER
#define FIFO2	FIFO2_REGISTER
#define RR	RSTFIFO1_REGISTER
#define MR	MSC_REGISTER

/* mask of registers */
#define FIFO1_MASK       0xff
#define FIFO2_MASK       0xff	
#define DO_MASK	       0xff
#define DI_MASK	       0xff
#define MR_MASK	       0xff
#define RR_MASK	       0xff

#endif							/* _PISOPS300_H */
