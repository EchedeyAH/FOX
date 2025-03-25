/* Flag inline functions

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

   v 0.0.0 19 Nov 2002 by Reed Lai
     create, separate from ixpio.h... */

#ifndef _FLAGS_INLINE_H
#define _FLAGS_INLINE_H

#include "_flags.h"

/* flag operation */
inline ixpio_flags_t get_flag_mask(const int pos)
{
	ixpio_flags_t mask = 1;
	if (!pos)
		return 1;
	return (mask << pos);		/* XXX - little-endian */
}

inline void on_flag(ixpio_flags_t * obj, const int pos)
{
	*obj |= get_flag_mask(pos);
}

inline void off_flag(ixpio_flags_t * obj, const int pos)
{
	*obj &= ~get_flag_mask(pos);
}

inline int get_flag(const ixpio_flags_t * obj, const int pos)
{
	ixpio_flags_t result = *obj;
	result &= get_flag_mask(pos);
	return result ? 1 : 0;
}

#endif							/* _FLAGS_INLINE_H */
