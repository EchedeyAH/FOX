/* Example of Digital I/O for PCIe-8622

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

   v 0.0.0 24 Sep 2015 by Winson Chen
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
	int fd_dev, i, DI[12] ={0}, th = 2048;
	ixpci_reg_t dio;
	char *dev_file;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	dio.id = IXPCI_DO;
	dio.value = 5;  //DO0 & DO2 ON

	if (ioctl(fd_dev, IXPCI_WRITE_REG, &dio)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_DO.");
		return FAILURE;
	}

	dio.id = IXPCI_DI;
	if (ioctl(fd_dev, IXPCI_READ_REG, &dio)){
		puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_DI.");
	}

	printf("DO readback = 0x%x\n",dio.value & 0x0fff);

	dio.value = (dio.value & 0x0fff0000)>>16;

	for(i = 11; i >= 0; i--)
	{
		if(dio.value / th)
		{
			DI[i] = 1;
			dio.value -= th;
		}
		th /= 2;

		if(DI[i])
			printf("DI %d ON\n",i);
		else
			printf("DI %d OFF\n",i);
	}

	close(fd_dev);
	return SUCCESS;
}
