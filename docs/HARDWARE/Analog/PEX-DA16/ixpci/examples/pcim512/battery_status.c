/* Example of Digital I/O for PCI-1202

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

   v 0.1.0 25 Oct 2001 by Reed Lai
     Change all "ixpci" to "ixpci."

   v 0.0.0 14 May 2001 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpci.h"


int main()
{
	int fd_dev;
	ixpci_reg_t rdi, rdo;
	char *dev_file;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);

	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* set ioctl command */
	rdi.id = IXPCI_BATTERY_STATUS;

	if (ioctl(fd_dev, IXPCI_READ_REG, &rdi))
	{  /* DI */
        	puts("Failure of ioctl command IXPCI_RRITE_REG: IXPCI_BATTERY_STATUS.");
                return FAILURE;
        }

        printf("BATTERY STATUS = 0x%x\n", rdi.value);

	/* check Battery 1 status */
	if ( (rdi.value & 0x3) == 0 )
	{
		printf("BT1 > 2.3V : LED1 OFF, LED2 OFF\n");
	}
	else if ( (rdi.value & 0x3) == 0x1 )
        {
                printf("2.1V < BT1 < 2.3V : LED1 ON, LED2 OFF\n");
        }
	else if ( (rdi.value & 0x3) == 0x3 )
        {
                printf("BT1 < 2.1V : LED1 ON, LED2 ON\n");
        }

	/* check Battery 2 status */
        if ( (rdi.value & 0xc) == 0 )
        {
                printf("BT2 > 2.3V : LED3 OFF, LED4 OFF\n");
        }
        else if ( (rdi.value & 0xc) == 0x4 )
        {
                printf("2.1V < BT2 < 2.3V : LED3 ON, LED4 OFF\n");
        }
        else if ( (rdi.value & 0xc) == 0xc )
        {
                printf("BT2 < 2.1V : LED3 ON, LED4 ON\n");
        }

	close(fd_dev);
	return SUCCESS;
}
