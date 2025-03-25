/* Example of Card ID for PCI-P16R16/P16C16/P16POR16, PEX-P16POR16

   Author: Winson Chen

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

   v 0.0.0 24 April 2018 by Winson Chen
     Read DO value back. */

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
	unsigned int dorb, dout;
	char *dev_file;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if(fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	dout = 3;
	if(ioctl(fd_dev, IXPCI_IOCTL_DO, &dout)) {  /* Set DO value */
		puts("Failure of ioctl command IXPCI_DO.");
		close(fd_dev);
		return FAILURE;
	}

	if(ioctl(fd_dev, IXPCI_DORB, &dorb)) {  /* Read DO value back */
		puts("Failure of ioctl command IXPCI_DORB.");
		return FAILURE;
	}

	printf("Do value is %u\n",dorb);

	close(fd_dev);
	return SUCCESS;
}
