/* Example of I/O port for PIO-D24 with PIO/PISO Static Libary.

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

   This example uses CN1 to examine the digital I/O.

   v 0.0.0 7 Jul 2007 by Golden Wang
     create */

#include "piodio.h"

int main()
{
	int fd;
	char *dev_file;
	ixpio_reg_t reg;

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

	/* set Port 1 as DO */
        PIODA_PortDirCfs(fd, 1, DIGITAL_OUTPUT);
        WORD do_data = 1;
        WORD di_data;
        WORD di_data2;

        while(getchar() != 27)
        {
	  /* output data from port1 by using libary DO function */
          if (PIODA_Digital_Output(fd, 1, do_data)) return FAILURE;
	  
	  /* input data from port0 by using libary DI function */
          if (PIODA_Digital_Input(fd, 0, &di_data)) return FAILURE;

	  /* input data from port2 by using libary DI function */
          if (PIODA_Digital_Input(fd, 2, &di_data2)) return FAILURE;

          printf("Port1 output = 0x%x. Port0 input = 0x%x. Port2 input = 0x%x.\n", do_data,di_data, di_data2);
          (do_data == 8) ? (do_data = 1) : (do_data <<= 1);
        }

	/* close device */
        PIODA_Close(fd);
	
        puts("End of program");

	return SUCCESS;
}
