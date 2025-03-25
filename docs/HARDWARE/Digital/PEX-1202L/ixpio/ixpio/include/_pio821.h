/* Declarations for PIO-821.

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


#ifndef _PIO821_H
#define _PIO821_H

#define MODULE_NAME "pio821"

#define IO_RANGE       0x100
#define BOARD_IO_ENABLE    0x11
	/* Note: The bit-0 enables board I/O
	 *       The bit-4 sets the bus writing cycle for 5 clocks, which
	 *       ensures the DAC data latching.
	 *       Read the data-sheet of the PCI control chip of this board
	 *       for details. */
#define BOARD_IO_DISABLE   0

#define INT_MASK_SHIFT_STOP_BIT  0x08

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

#define DA_LOW_BYTE                 0xd0
#define DA_HIGH_BYTE                0xd4
#define AD_LOW_BYTE		    0xd0
#define AD_HIGH_BYTE		    0xd4

#define DIGITAL_INPUT_LOW_BYTE      0xd8
#define DIGITAL_INPUT_HIGH_BYTE     0xdc
#define DIGITAL_OUTPUT_LOW_BYTE     0xd8
#define DIGITAL_OUTPUT_HIGH_BYTE    0xdc

#define AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER	0xe0
#define AD_MODE_CONTROL_REGISTER    0xe4
#define AD_TRIGGER_CONTROL_REGISTER 0xe8
#define AD_STATUS		    0xec

#define RCR     RESET_CONTROL_REG
#define ACR     AUX_CONTROL_REG
#define ADR     AUX_DATA_REG
#define IMCR    INT_MASK_CONTROL_REG
#define ASR    AUX_PIN_STATUS_REG
#define IPCR    INT_POLARITY_CONTROL_REG

#define _8254C0 _8254_COUNTER_0
#define _8254C1 _8254_COUNTER_1
#define _8254C2 _8254_COUNTER_2
#define _8254CW _8254_CONTROL_WORD

#define DAL     DA_LOW_BYTE
#define DAH     DA_HIGH_BYTE
#define ADL	AD_LOW_BYTE
#define ADH	AD_HIGH_BYTE

#define AD

#define DIL     DIGITAL_INPUT_LOW_BYTE
#define DIH     DIGITAL_INPUT_HIGH_BYTE
#define DOL     DIGITAL_OUTPUT_LOW_BYTE
#define DOH     DIGITAL_OUTPUT_HIGH_BYTE

#define DI

#define ADGCR	AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER
#define ADMCR	AD_MODE_CONTROL_REGISTER
#define ADTCR	AD_TRIGGER_CONTROL_REGISTER
#define ADS	AD_STATUS


/* masks of registers */
#define RCR_MASK      0xff
#define ACR_MASK      0xff
#define ADR_MASK      0xff
#define IMCR_MASK     0x0f
#define IMCR_INT_MASK 0x0f
#define ASR_MASK      0xff
#define ASR_INT_MASK  0x0f
#define IPCR_MASK     0x0f
#define IPCR_INT_MASK 0x0f

#define _8254C0_MASK  0xff
#define _8254C1_MASK  0xff
#define _8254C2_MASK  0xff
#define _8254CW_MASK  0xff

#define DAL_MASK      0xff
#define DAH_MASK      0x0f
#define ADL_MASK      0xff
#define ADH_MASK      0x0f

#define DIL_MASK      0xff
#define DIH_MASK      0xff
#define DOL_MASK      0xff
#define DOH_MASK      0xff

#define ADGCR_MASK    0x3f
#define ADMCR_MASK    0xff
#define ADTCR_MASK    0xff
#define ADS_MASK      0x01

#endif							/* _PIO821_H */
