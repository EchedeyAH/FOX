/* Example of Change of status for PCI-D96/128SU

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

   v 0.0.0  6 Jul 2020 by Winson Chen
     Set PA as DO port and PB as DI port, 
     Set PB change of status mode without interrupt.
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
	int fd_dev, tmp;
	ixpci_reg_t reg;
	char *dev_file;
	unsigned int dwValue, dwDOVal, dwClearVal, dwcompareVal, dwStatus;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	printf("Set PortA as DO mode, PortB as DI mode(Pattern match) demo.\n");

	//Set PA to DO Mode
	reg.id = IXPCI_DIOPA_CR;
        reg.value = 0xf;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }

	//Get CompareTrigger/Change Status Readback.
	reg.id = IXPCI_PPM_CS;
	if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_READ_REG.");
		close(fd_dev);
		return FAILURE;
	}

	//Close PortB Compare Trigger/Change Status
	reg.id = IXPCI_PPM_CS;
	reg.value = reg.value & 0x1d;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG.");
		close(fd_dev);
		return FAILURE;
	}

	//Get CompareTrigger/Change Status Readback.
	reg.id = IXPCI_PPM_CS;
	if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_READ_REG.");
		close(fd_dev);
		return FAILURE;
	}

	//Set PortB as Change of status
	reg.id = IXPCI_PPM_CS;
	reg.value = reg.value | 0x20;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG.");
		close(fd_dev);
		return FAILURE;
	}
	
	/* wait for exit */
        puts("Press Esc key to exit");
        while (getchar() != 27)
	{
		printf("Please Set DO Value:");
		tmp = scanf("%x",&dwDOVal);

		reg.id = IXPCI_DIOPA;   //PA output value
	        reg.value = dwDOVal;
        	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                	puts("Failure of ioctl command IXPCI_WRITE_REG.");
	                close(fd_dev);
        	        return FAILURE;
	        }

		//Get CompareTrigger/Change Status Readback.
		reg.id = IXPCI_PPM_CS;
		if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
			puts("Failure of ioctl command IXPCI_READ_REG.");
			close(fd_dev);
			return FAILURE;
		}
		dwValue = reg.value;

		dwStatus = (dwValue >> 13) & 0x1;	//right shift 13, get 0x30 PB_S_F status.

		//Clear PortB change of status
		reg.id = IXPCI_PB_CCS;
		reg.value = 0;
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
			puts("Failure of ioctl command IXPCI_WRITE_REG.");
			close(fd_dev);
			return FAILURE;
		}
	
		if(dwStatus == 1)
			printf("Change of status Trigger OK.\n");
		else
			printf("Change of status Trigger Fail.\n");
	}

	close(fd_dev);
        puts("\nEnd of program.");
	return SUCCESS;
}
