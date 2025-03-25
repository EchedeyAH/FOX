/* Example of Analog ouput for PCI-1800 with PCI Static Libary.

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

/* File level history (record changes for this file here.)

   This example uses CN1 to examine the digital I/O.

   v 0.0.0 1 May 2008 by Golden Wang
     create */


#include "pcidio.h"
int main()
{
        int fd;
        char *dev_file;
	float voltages = 10; //Analog output 5V
	float fAdVals[10];

        dev_file = "/dev/ixpci1";

        /* open device file */
        fd = PCIDA_Open(dev_file);

        if (fd < 0)
        {
          printf("Can't open device\n");
          return FAILURE;
        }

        /* init device */
        if (PCIDA_DriverInit(fd)) return FAILURE;

        while(getchar() != 27)
        {
          /* output 5V from Analog channel 0 by using libary AO function */
          if (PCI_180X_DaF_Output(fd, 0, voltages)) return FAILURE;

          printf("Analog output = %dV.\n", (WORD)voltages);
        }

	/* close device */
        PCIDA_Close(fd);

        puts("End of program");

        return SUCCESS;
}

