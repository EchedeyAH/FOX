/* Items of PCI-PP8R8

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

   v 0.0.0  9 Jan 2003 by Reed Lai
     Create */

#ifndef _PCIP8R8_H
#define _PCIP8R8_H

#define MODULE_NAME "ixpcip8r8"

/* offset of registers */
/* bar 0 (NA) */
/* bar 1 (NA) */
/* bar 2 (dedicate to the 8 bit I/O) */
#define _DO_READBACK    0x0c
#define _GET_CARD_ID    0x3c

/* bar 3 (NA) */
/* bar 4 (NA) */

/* abbreviation of registers */
/* bar 2 */
#define _DORB	_DO_READBACK
#define _GCID	_GET_CARD_ID
/* bar 3 */
/* bar 4 */
/* bar 5 */

/* mask of registers (8-bit operation) */
#define _DO_MASK        0xff
#define _DI_MASK        0xff
#define _GCID_MASK      0xf
#define _DORB_MASK      0xff

#endif
