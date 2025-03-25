/* Example of DO Readback function for PEX-P8POR8i/P16POR16i

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

   v 0.0.0 23 April 2018 by Winson Chen
     Read DO value back. 
     Note that the DO Readback function is only supported by PEX-P8POR8i/P16POR16i Series cards (Version 1.0 or above).*/

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

	dout = 1;
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
