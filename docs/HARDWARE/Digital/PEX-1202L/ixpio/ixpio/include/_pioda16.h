/* Declarations for PIO-DA16/DA8/DA4.

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

   v 0.0.0 26 Nov 2002 by Reed Lai
     create, blah blah... */

#ifndef _PIODA16_H
#define _PIODA16_H

#define MODULE_NAME "pioda16"

#define IO_RANGE       0x100
#define BOARD_IO_ENABLE    0x11
	/* Note: The bit-0 enables board I/O
	 *       The bit-4 sets the bus writing cycle for 5 clocks, which
	 *       ensures the DAC data latching.
	 *       Read the data-sheet of the PCI control chip of this board
	 *       for details. */
#define BOARD_IO_DISABLE   0

#define INT_MASK_SHIFT_STOP_BIT  0x04

/* offset of registers */
#define RESET_CONTROL_REG           0x00
#define AUX_CONTROL_REG             0x02
#define AUX_DATA_REG                0x03
#define INT_MASK_CONTROL_REG        0x05
#define AUX_PIN_STATUS_REG          0x07
#define INT_POLARITY_CONTROL_REG    0x2a

#define _8254_COUNTER_0             0xc0
#define _8254_COUNTER_1             0xc4
#define _8254_COUNTER_2             0xc8
#define _8254_CONTROL_WORD          0xcc

#define READ_CARD_ID		    0xd4

#define DA_0_CHIP_SELECT            0xe0
#define DA_1_CHIP_SELECT            0xe4
#define DA_2_CHIP_SELECT            0xe8
#define DA_3_CHIP_SELECT            0xec

#define DIGITAL_INPUT_LOW_BYTE      0xe0
#define DIGITAL_INPUT_HIGH_BYTE     0xe4

#define DA_LOW_BYTE                 0xf0
#define DA_HIGH_BYTE                0xf4

#define DIGITAL_OUTPUT_LOW_BYTE     0xf8
#define DIGITAL_OUTPUT_HIGH_BYTE    0xfc

#define RCR     RESET_CONTROL_REG
#define ACR     AUX_CONTROL_REG
#define ADR     AUX_DATA_REG
#define IMCR    INT_MASK_CONTROL_REG
#define APSR    AUX_PIN_STATUS_REG
#define IPCR    INT_POLARITY_CONTROL_REG

#define _8254C0 _8254_COUNTER_0
#define _8254C1 _8254_COUNTER_1
#define _8254C2 _8254_COUNTER_2
#define _8254CW _8254_CONTROL_WORD

#define CID	READ_CARD_ID

#define DA0CS   DA_0_CHIP_SELECT
#define DA1CS   DA_1_CHIP_SELECT
#define DA2CS   DA_2_CHIP_SELECT
#define DA3CS   DA_3_CHIP_SELECT

#define DIL     DIGITAL_INPUT_LOW_BYTE
#define DIH     DIGITAL_INPUT_HIGH_BYTE

#define DAL     DA_LOW_BYTE
#define DAH     DA_HIGH_BYTE

#define DOL     DIGITAL_OUTPUT_LOW_BYTE
#define DOH     DIGITAL_OUTPUT_HIGH_BYTE

/* masks of registers */
#define RCR_MASK      0x11
#define ACR_MASK      0xff
#define ADR_MASK      0xff
#define IMCR_MASK     0x03
#define IMCR_INT_MASK 0x03
#define APSR_MASK     0xff
#define APSR_INT_MASK 0x03
#define IPCR_MASK     0x03
#define IPCR_INT_MASK 0x03

#define _8254C0_MASK  0xff
#define _8254C1_MASK  0xff
#define _8254C2_MASK  0xff
#define _8254CW_MASK  0xff

#define CID_MASK      0x0f

#define DA0CS_MASK    0xff
#define DA1CS_MASK    0xff
#define DA2CS_MASK    0xff
#define DA3CS_MASK    0xff

#define DAL_MASK      0xff
#define DAH_MASK      0xff

#define DIL_MASK      0xff
#define DIH_MASK      0xff
#define DOL_MASK      0xff
#define DOH_MASK      0xff

#endif							/* _PIODA16_H */
