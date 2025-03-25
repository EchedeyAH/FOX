/* Example of Pattern match for PCI-D96/128SU

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

   v 0.0.0  24 Jun 2020 by Winson Chen
     Set PA as DO port and PB as DI port, 
     Set PB compare trigger mode without interrupt.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpci.h"

int fd_dev;
ixpci_reg_t reg;

int SetPortB_DI_CompareTriggerMode(unsigned int value)
{

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

	//Set PortB Compare trigger value
	reg.id = IXPCI_PB_CVS;
        reg.value = value;	//PortB compare value
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

	//Set PortB as Compare trigger
	reg.id = IXPCI_PPM_CS;
	reg.value = reg.value | 0x2;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG.");
		close(fd_dev);
		return FAILURE;
	}
}

int main()
{
	int tmp, PB = 1;
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

	dwcompareVal = 0x1;
	printf("Compare value is 0x%x\n\n",dwcompareVal);
	if(SetPortB_DI_CompareTriggerMode(dwcompareVal))
		return FAILURE;

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

		reg.id = IXPCI_DIOPA;   //PA output value
	        reg.value = 0;
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
		dwClearVal = dwValue & 0x3f;	//Get compare/change of status only

		dwStatus = (dwValue >> 9) & 0x1;	//right shift 9, get 0x30 PB_C_F status.

		//Clear PortB compare trigger status
		reg.id = IXPCI_PPM_CS;
		reg.value = dwValue&(~PB);	//PA:0, PB:1, PC:2, PD:3
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
			puts("Failure of ioctl command IXPCI_WRITE_REG.");
			close(fd_dev);
			return FAILURE;
		}

		reg.id = IXPCI_PPM_CS;
		reg.value = dwClearVal;
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
			puts("Failure of ioctl command IXPCI_WRITE_REG.");
			close(fd_dev);
			return FAILURE;
		}
	
		if(SetPortB_DI_CompareTriggerMode(dwcompareVal))
			return FAILURE;

		if(dwStatus == 1)
			printf("Compare Trigger OK.\n");
		else
			printf("\nCompare Trigger Fail. Trigger Soure Value is 0x%x\n",dwcompareVal);

	}

	close(fd_dev);
        puts("\nEnd of program.");
	return SUCCESS;
}
