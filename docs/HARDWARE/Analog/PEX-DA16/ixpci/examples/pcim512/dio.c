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

	/* DIO */
	rdi.id = IXPCI_DI;
	rdo.id = IXPCI_DO;

	if (ioctl(fd_dev, IXPCI_READ_REG, &rdi))
	{  /* DI */
        	puts("Failure of ioctl command IXPCI_RRITE_REG: IXPCI_DI.");
                return FAILURE;
        }

        printf("DI = 0x%x\n", rdi.value);

	rdo.value = 0xffff;

	puts("ESC and Enter to go out. Enter to continue.");
	while (getchar() != 27) {
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &rdo)) {  /* DO */
			puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_DO.");
			close(fd_dev);
			return FAILURE;
		}
		printf("DO = 0x%x\t", rdo.value);
		if (ioctl(fd_dev, IXPCI_READ_REG, &rdi)) {  /* DI */
			puts("Failure of ioctl command IXPCI_RRITE_REG: IXPCI_DI.");
			return FAILURE;
		}
		printf("DI = 0x%x\n", rdi.value);

		if (rdo.value == 0xffff)
			rdo.value = 0x5555;
		else
			rdo.value = 0xffff;
	}

	close(fd_dev);
	return SUCCESS;
}
