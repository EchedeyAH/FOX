/* Example of Card ID for PCI-2602U

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

   v 0.0.0 27 Oct 2023 by Winson Chen
     Set ao value. */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpci.h"

int main()
{
	int fd_dev, i;
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

	//Set AO configuration
	reg.id = IXPCI_AO_CFG;	//AO
	reg.value = 0x9;	//channel 0 bipolar +-10V
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
	}

        //Read AO configuration
        reg.id = IXPCI_AO_CFG;
	if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_READ_REG.");
                close(fd_dev);
                return FAILURE;
        }

	printf("AO configuration is 0x%x\n",reg.value);

	//Set AO 10V
	reg.id = IXPCI_AO0;
	reg.value = 0xffff;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
	}

	printf("AO Hex value 0x%x\n",reg.value);

	close(fd_dev);
	return SUCCESS;
}
