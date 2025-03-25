/* Example of I/O port for PIO-D48 with PIO/PISO Static Libary.

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

   v 0.0.0  12 Aug 2019 by Winson Chen
     Test DIO by static lib */


#include "piodio.h"

#define DO_enable 0
#define DI_enable 1

int main(void)
{
	int fd, r;
	char *dev_file;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = PIODA_Open(dev_file);

	if (fd < 0)
	{
		printf("Can't open device\n");
		return FAILURE;
	}

	/* init device */
	if (PIODA_DriverInit(fd)) return FAILURE;

	/* set Port0 (CN1 PA) as DO */
	r = PIODA_PortDirCfs(fd, PIOD48_P0, DO_enable);
	if(r)
	{
		printf("Port defined error\n");
		return FAILURE;
	}

	/* set port2 (CN1 PC) as DI */
	r = PIODA_PortDirCfs(fd, PIOD48_P2, DI_enable);
        if(r)
        {
                printf("Port defined error\n");
                return FAILURE;
        }


	WORD do_data = 1;
	WORD di_data;

	printf("Set port0 as DO and port2 as DI\n");
	printf("Enter to continue, Esc to exit.\n");
	while(getchar() != 27)
	{
		/* output data from port1 by using libary DO function */
		if (PIODA_Digital_Output(fd, PIOD48_P0, do_data)) return FAILURE;

		/* input data from port0 by using libary DI function */
		if (PIODA_Digital_Input(fd, PIOD48_P2, &di_data)) return FAILURE;

		printf("Port2 input = 0x%x. Port0 output = 0x%x.\n", di_data, do_data);
		(do_data == 0x80) ? (do_data = 1) : (do_data <<= 1);
	}

	/* close device */
	PIODA_Close(fd);
	return SUCCESS;
}
