/* Declarations for PCI-M512 new version(id:12345676)

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

#ifndef _PCIM512_H
#define _PCIM512_H

int _write_reg(ixpci_reg_t * , void __iomem * []);
int _read_reg(ixpci_reg_t * , void __iomem * []);

#define MODULE_NAME "ixpcim512"

#define INT_MASK_SHIFT_STOP_BIT  0x10

/* offset of registers */
/* bar 0 (NA) */
/* bar 1 */
/* bar 2 */
#define SRAM_REGISTER	0x00
/* bar 3 */
#define DI_PORT		0x00
#define DO_PORT		0x00
/* bar 4 */

/* abbreviation of registers */
/* bar 2 */
#define _SR  SRAM_REGISTER
/* bar 3 */
#define _DI       DI_PORT
#define _DO       DO_PORT
/* bar 4 */
/* bar 5 */

/* mask of registers (16-bit operation) */
#define _SRAM_MASK      0xff
#define _DI_MASK      0xfff0
#define _DO_MASK      0xffff
#endif							/* _PCIM512_H */
