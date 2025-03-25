/* Example of Digital I/O for PCI-P16R16/P16C16/P16POR16

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

   v 0.0.0 16 May 2002 by Reed Lai
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
	unsigned int din, dout;
	char *dev_file;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	dout = 1;
	puts("ESC and Enter to go out. Enter to continue.");
	while (getchar() != 27) {
		if (ioctl(fd_dev, IXPCI_IOCTL_DO, &dout)) {  /* DO */
			puts("Failure of ioctl command IXPCI_DO.");
			close(fd_dev);
			return FAILURE;
		}
		printf("DO = 0x%04x\t", dout);
		usleep(1500);
		if (ioctl(fd_dev, IXPCI_IOCTL_DI, &din)) {  /* DI */
			puts("Failure of ioctl command IXPCI_DI.");
			return FAILURE;
		}
		printf("DI = 0x%04x\n", din);
		dout == 0x8000 ? dout = 1 : (dout <<= 1);
	}

	close(fd_dev);
	return SUCCESS;
}
