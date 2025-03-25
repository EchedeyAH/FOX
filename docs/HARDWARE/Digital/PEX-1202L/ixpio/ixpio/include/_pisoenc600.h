/* PISO-ENC600 items

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

#ifndef _PISOENC600_H
#define _PISOENC600_H

#define MODULE_NAME "ixpisoenc600"

#define IO_RANGE  	   0xff
#define BOARD_DIO_ENABLE   0x1
#define BOARD_DIO_DISABLE  0

/* offset of registers */
#define X1_AXIS_CONTROL_REGISTER	0xc0
#define X2_AXIS_CONTROL_REGISTER	0xc8
#define X3_AXIS_CONTROL_REGISTER	0xd0
#define X4_AXIS_CONTROL_REGISTER	0xd8
#define X5_AXIS_CONTROL_REGISTER	0xe0
#define X6_AXIS_CONTROL_REGISTER	0xe8

#define X1_AXIS_DIGITAL_INPUT_REGISTER	0xc4
#define X2_AXIS_DIGITAL_INPUT_REGISTER	0xcc
#define X3_AXIS_DIGITAL_INPUT_REGISTER	0xd4
#define X4_AXIS_DIGITAL_INPUT_REGISTER	0xdc
#define X5_AXIS_DIGITAL_INPUT_REGISTER	0xe4
#define X6_AXIS_DIGITAL_INPUT_REGISTER	0xec

#define DO				0xc4

#define XR1	X1_AXIS_CONTROL_REGISTER		
#define XR2	X2_AXIS_CONTROL_REGISTER
#define XR3	X3_AXIS_CONTROL_REGISTER
#define XR4	X4_AXIS_CONTROL_REGISTER
#define XR5	X5_AXIS_CONTROL_REGISTER
#define XR6	X6_AXIS_CONTROL_REGISTER

#define DI1	X1_AXIS_DIGITAL_INPUT_REGISTER
#define DI2	X2_AXIS_DIGITAL_INPUT_REGISTER
#define DI3	X3_AXIS_DIGITAL_INPUT_REGISTER
#define DI4	X4_AXIS_DIGITAL_INPUT_REGISTER
#define DI5	X5_AXIS_DIGITAL_INPUT_REGISTER
#define DI6	X6_AXIS_DIGITAL_INPUT_REGISTER

/* for tiger's register */
#define TJ_CNTL				0x00  //set reset and others
#define TJ_AUXC			    	0x02  //AUX pin control register 1:output
#define TJ_AUXD			    	0x03  //AUX pin data register
#define TJ_MASK			    	0x05  //interrupt mask
#define TJ_AUX			    	0x07  //AUX pin status
#define TJ_POLARITY		    	0x2a  //AUX pin polarity

/* mask of registers */
#define XR1_MASK			0xff
#define XR2_MASK			0xff
#define XR3_MASK			0xff
#define XR4_MASK			0xff
#define XR5_MASK			0xff
#define XR6_MASK			0xff

#define DI1_MASK			0xff
#define DI2_MASK			0xff
#define DI3_MASK			0xff
#define DI4_MASK			0xff
#define DI5_MASK			0xff
#define DI6_MASK			0xff

#define DO_MASK				0xff
#endif							/* _PISOENC600_H */
