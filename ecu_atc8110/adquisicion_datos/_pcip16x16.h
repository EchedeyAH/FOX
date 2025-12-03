/* Items of PCI-P16R16/P16C16/P16POR16

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

   v 0.1.0  8 Jan 2003 by Reed Lai
     Removed IO_RANGE.

   v 0.0.0 16 May 2002 by Reed Lai
     create, blah blah... */

#ifndef _PCIP16X16_H
#define _PCIP16X16_H

#define MODULE_NAME "ixpcip16x16"

/* offset of registers */
/* bar 0 (NA) */
/* bar 1 (NA) */
/* bar 2 (dedicate to the 16 bit I/O) */
#define _DO_READBACK	0x0c
#define _GET_CARD_ID	0x3c
/* bar 3 (NA) */
/* bar 4 (NA) */

/* abbreviation of registers */
/* bar 2 */
#define _GCID		_GET_CARD_ID
#define _DORB		_DO_READBACK
/* bar 3 */
/* bar 4 */
/* bar 5 */

/* mask of registers (16-bit operation) */
#define _DO_MASK	0xffff
#define _DI_MASK	0xffff
#define _GCID_MASK	0xf
#define _DORB_MASK	0xff

#endif							/* _PCIP16X16_H */
