/* Example of Digital I/O configure for PCI-D96/128SU

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

   v 0.0.0  5 May 2020 by Winson Chen
     Set PA as DO port and PB as DI port, then test DIOa
     port's direction
     0 -> DO
     1 -> DI
 */

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
	ixpci_reg_t reg;
	char *dev_file;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	//Set PA as DO port
	reg.id = IXPCI_DIOPA_CR;
	reg.value = 0xf;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG.");
		close(fd_dev);
		return FAILURE;
	}
	printf("Set PortA DO port.\n");

	//PA output value
	reg.id = IXPCI_DIOPA;
	reg.value = 0xfffffff1;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG.");
		close(fd_dev);
		return FAILURE;
	}
	printf("Write PortA DO value 0x%x\n",reg.value);

	//PB read input value
	reg.id = IXPCI_DIOPB;
	if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_READ_REG.");
		close(fd_dev);
		return FAILURE;
	}
	printf("Read PortB DI value 0x%x\n",reg.value);

	close(fd_dev);
	return SUCCESS;
}
