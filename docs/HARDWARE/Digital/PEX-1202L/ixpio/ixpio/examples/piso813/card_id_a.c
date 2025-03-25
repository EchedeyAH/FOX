/* Example of Card ID for PISO-813U Static library.

   Author: Winson Chen

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

   v 0.0.0  18 Oct 2019 by Winson Chen
   Notice, The Card ID function is only supported by the PISO-813U (Ver. 1.0 or above)
*/

#include "piodio.h"

int main()
{
	int fd;
	char *dev_file;
	byte id;

	dev_file = "/dev/ixpio1";

	/* Open device file */
	fd = PIODA_Open(dev_file);

	if (fd < 0)
	{
		printf("Can't open device\n");
		return FAILURE;
	}

	if (PIODA_DriverInit(fd)) return FAILURE;
        /* DIO */

	PIODA_CARD_ID(fd, &id);
	printf("CARD ID is %d\n",id);

	PIODA_Close(fd);
        return SUCCESS; 
}
