/* Example of Digital I/O for Lanner OEM I/O

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

   v 0.0.0 12 May 2010 by Golden Wang
     create, blah blah... */

#include "pcidio.h"

int main()
{
	int fd;
        char *dev_file;
        BYTE do_data = 1;
        BYTE di_data;

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

	puts("ESC and Enter to go out. Enter to continue.");

	while(getchar() != 27)
        {
		/* output data from port1 by using libary DO function */
		if (PCI_LANNER_Digital_Output(fd, do_data))
			return FAILURE;

		/* input data from port0 by using libary DI function */
		if (PCI_LANNER_Digital_Input(fd, &di_data)) 
			return FAILURE;

		printf("Digital output = 0x%x. Digital input = 0x%x.\n", do_data,di_data);
		(do_data == 0x80) ? (do_data = 1) : (do_data <<= 1);
        }

        /* close device */
	if (PCI_LANNER_Digital_Output(fd, 0))
		return FAILURE;
	
        PCIDA_Close(fd);

        puts("End of program");

        return SUCCESS;
}
