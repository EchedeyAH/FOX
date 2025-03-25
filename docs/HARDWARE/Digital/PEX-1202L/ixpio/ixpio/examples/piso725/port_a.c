/* Example of Digital I/O for PISO-725 with PIO/PISO Static Libary.

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

   v 0.0.0 25 September 2006 by Reed Lai
     create */

#include <stdio.h>
#include "piodio.h"

int main()
{
	int fd;
	char *dev_file;

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
        WORD do_data = 1;
        WORD di_data;

	while (getchar() == 10) {

                if (PIODA_Digital_Output(fd, 0, do_data)) return FAILURE;
		usleep(5000); /* 5ms for relay operating time */

                if (PIODA_Digital_Input(fd, 0, &di_data)) return FAILURE;
		printf("Relay Output = 0x%2x\tDI =0x%2x\tEsc to exit.\n", do_data,di_data);
		(do_data < 0x80) ? (do_data <<= 1) : (do_data = 1);
	}

	PIODA_Close(fd);
	return SUCCESS;
}
