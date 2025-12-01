/* Declarations for PCI-1602 new version(id:12345676)

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

/* File level history (record changes for this file here.
 
   *** Do not include this file in your code. ***

   v 0.0.0 18 Apr 2001 by Reed Lai
     create, blah blah... */

#ifndef _PCI1602_H
#define _PCI1602_H

int _write_reg(ixpci_reg_t * reg, unsigned long int base[]);
int _read_reg(ixpci_reg_t * reg, unsigned long int base[]);
int _time_span(int span, unsigned long base[]);
int _reset_dev(ixpci_devinfo_t * dp);

#define MODULE_NAME "ixpci1602"

#define INT_MASK_SHIFT_STOP_BIT  0x10

/* offset of registers */
/* bar 0 (NA) */
/* bar 1 */
#define _8254_TIMER_1           0x00
#define _8254_TIMER_2           0x04
#define _8254_TIMER_3           0x08
#define _8254_CONTROL_REGISTER  0x0c
/* bar 2 */
#define CONTROL_REGISTER        0x00
#define STATUS_REGISTER         0x00
#define AD_SOFTWARE_TRIGGER     0x04
/* bar 3 */
#define DI_PORT                 0x00
#define DO_PORT                 0x00
/* bar 4 */
#define AD_DATA_PORT            0x00
#define DA_CHANNEL_1            0x00
#define DA_CHANNEL_2            0x04

/* abbreviation of registers */
/* bar 2 */
#define _8254C0  _8254_TIMER_1	/* take care the digit!! */
#define _8254C1  _8254_TIMER_2
#define _8254C2  _8254_TIMER_3
#define _8254CR  _8254_CONTROL_REGISTER
/* bar 3 */
#define _CR       CONTROL_REGISTER
#define _SR       STATUS_REGISTER
#define _ADST     AD_SOFTWARE_TRIGGER
/* bar 4 */
#define _DI       DI_PORT
#define _DO       DO_PORT
/* bar 5 */
#define _AD       AD_DATA_PORT
#define _DA1      DA_CHANNEL_1
#define _DA2      DA_CHANNEL_2

/* mask of registers (16-bit operation) */
#define _8254C0_MASK  0xffff
#define _8254C1_MASK  0xffff
#define _8254C2_MASK  0xffff
#define _8254CR_MASK  0xffff
#define _CR_MASK      0xbfdf
#define _SR_MASK      0x00ff
#define _ADST_MASK    0xffff
#define _DI_MASK      0xffff
#define _DO_MASK      0xffff
#define _AD_MASK      0xffff
#define _DA1_MASK     0xffff
#define _DA2_MASK     0xffff

#endif							/* _PCI1602_H */
