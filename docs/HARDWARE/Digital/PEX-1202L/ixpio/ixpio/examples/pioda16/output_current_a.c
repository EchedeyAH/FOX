/* Example of analog output for PIO-DA16/DA8/DA4 with PIO/PISO Static Libary.

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

   This example shows the analog current output by libary function.

   v 0.0.0 1  Nov by Reed Lai
     create, blah blah... */

#include "piodio.h"

int main()
{
	int fd;
	char *dev_file;
	ixpio_analog_t ao;

	dev_file = "/dev/ixpio1";

        fd = PIODA_Open(dev_file);

        if (fd < 0)
        {
          printf("Can't open device\n");
          return FAILURE;
        }

        if (PIODA_DriverInit(fd)) return FAILURE;

        unsigned int current = 0;

	puts("Press <enter> for next, ESC to exit.");
	while (getchar() != 27) {
		if (current > 22) current = 0;			
                if(PIODA_AnalogOutputCalCurrent(fd, 6, current))
                {
		  printf("output current: %d mA", current);
                  return FAILURE;
                }
		printf("output current: %d mA    <enter> next, ESC exit ", current);
                current++; 
	}

	close(fd);
	puts("\nEnd of program.");
	return SUCCESS;
}
