/* Example of EEPROM Read for Lanner OEM I/O

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

   v 0.0.0 4 Jun 2010 by Golden Wang
     create, blah blah... */

#include "pcidio.h"

int main()
{
	int fd, i;
        char *dev_file;
        WORD write_value, read_value;

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

	/* Read EEPROM Data */
	for(i = 0; i < 64; i++)
	{
		PCI_LANNER_EEPROM_ReadWord(fd, i, &read_value);
		printf("Get Block : %d = Value : 0x%04x\n", i, read_value);
		read_value = 0;
	}

        /* close device */
        PCIDA_Close(fd);

        puts("End of program");

        return SUCCESS;
}
