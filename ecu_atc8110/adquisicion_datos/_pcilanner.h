/* Declarations for PCI Lanner OEM I/O Card

   Author: Golden Wang

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

/* File level history (record changes for this file here.
 
   *** Do not include this file in your code. ***

   v 0.0.0 12 May 2010 by Golden Wang
     create, blah blah... */

#ifndef _PCILANNER_H
#define _PCILANNER_H

int _write_reg(ixpci_reg_t * reg, unsigned long int base[]);
int _read_reg(ixpci_reg_t * reg, unsigned long int base[]);

#define MODULE_NAME "ixpcilanner"

/* offset of registers */
/* bar 0 (NA) */
#define DI_PORT                 0x00
#define DO_PORT                 0x01
#define COUNTER0_REGISTER_1     0x02
#define COUNTER0_REGISTER_2     0x03
#define COUNTER0_REGISTER_3     0x04
#define COUNTER0_REGISTER_4     0x05
#define COUNTER1_REGISTER_1     0x06
#define COUNTER1_REGISTER_2     0x07
#define COUNTER1_REGISTER_3     0x08
#define COUNTER1_REGISTER_4     0x09
#define AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER 0x0a
#define AD_READY_OR_BUSY_STATUS 0x0a
#define AD_POLLING_REGISTER     0x0b
#define AD_LOW_BYTE             0x0c
#define AD_HIGH_BYTE            0x0d
#define EEPROM_REGISTER         0x0e

/* abbreviation of registers */
/* bar 0 */
#define _DI       DI_PORT
#define _DO       DO_PORT
#define _C0R1     COUNTER0_REGISTER_1
#define _C0R2     COUNTER0_REGISTER_2
#define _C0R3     COUNTER0_REGISTER_3
#define _C0R4     COUNTER0_REGISTER_4
#define _C1R1     COUNTER1_REGISTER_1
#define _C1R2     COUNTER1_REGISTER_2
#define _C1R3     COUNTER1_REGISTER_3
#define _C1R4     COUNTER1_REGISTER_4
#define _ADGCR    AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER
#define _ADS      AD_READY_OR_BUSY_STATUS
#define _ADPR	  AD_POLLING_REGISTER
#define _ADL      AD_LOW_BYTE
#define _ADH      AD_HIGH_BYTE
#define _EEP	  EEPROM_REGISTER

/* mask of registers (8-bit operation) */
#define _C0R_MASK          0xff
#define _C1R_MASK          0xff
#define _ADGCR_MASK        0x0f
#define _EEP_WRITE_MASK    0x07
#define _EEP_READ_MASK     0x07
#define _ADS_MASK          0x80
#define _ADPR_MASK         0xff
#define _DI_MASK           0xff
#define _DO_MASK           0xff
#define _ADL_MASK          0xff
#define _ADH_MASK          0x0f

#endif							/* _PCILANNER_H */
