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

   v 0.0.0 27 May 2024 by Winson Chen
     Read AI value. 
     The demo is not yet completed.	*/


#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpci.h"

//0x2ec bit7 to bit9 gain code
//1	+/- 10.24V
//2	+/- 5.12V
//3	+/- 2.56V
//7	+/- 13V

int main()
{
	int fd_dev, i, count, ai_hex;
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

//line 56~115 set polling configure

	//Set AI Scan Address
	reg.id = IXPCI_AI_SA;	//0x298
	reg.value = 0x0;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
	}

	//AI Configuration Control/Status
	reg.id = IXPCI_AI_CC;	//0x2ec
	reg.value = 0x9CCD;	//channel 1, single-ended, gain code 1 
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
	}

	//Save AI Configuration Data
	reg.id = IXPCI_SAI_C;	//0x2f0
	reg.value = 0x0;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
	}

	for(i = 0; i < 4; i++)
	{
		//AI Software Trigger Control
		reg.id = IXPCI_AI_STC;	//0x294
		reg.value = 0x0;
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                	puts("Failure of ioctl command IXPCI_WRITE_REG.");
	                close(fd_dev);
        	        return FAILURE;
		}
	
		count = 0;
	wait_ad:
		reg.id = IXPCI_AI_STS;	//0x294
		if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_READ_REG.");
                close(fd_dev);
                return FAILURE;
        	}

		reg.value = reg.value & 0x10000;
		if(reg.value != 0)
		{
			count++;
			if(count > 32000)
			{
				printf("Time out\n");
				return -1;
			}
			else
				goto wait_ad;
		}
	}

//line 56~115 set polling configure


	//AI polling
	for(i = 0; i < 5; i++)
	{
		//AI Software Trigger Control
		reg.id = IXPCI_AI_STC;	//0x294
		reg.value = 0x0;
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                	puts("Failure of ioctl command IXPCI_WRITE_REG.");
	                close(fd_dev);
        	        return FAILURE;
		}
	
		count = 0;
	wait_ai:
		reg.id = IXPCI_AI_STS;	//0x294
		if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_READ_REG.");
                close(fd_dev);
                return FAILURE;
        	}
		reg.value = reg.value & 0x10000;
		if(reg.value == 0)
		{
			ai_hex = reg.value & 0xffff;
			printf("channel 1, single-ended, gain code 1\n");
			printf("Hex value is 0x%x\n", ai_hex);
		}
		else
		{
			count++;
			if(count > 32000)
			{
				printf("Time out\n");
				return -1;
			}
			else
				goto wait_ai;
		}

	usleep(100);
	}
	
	


	close(fd_dev);
	return SUCCESS;
}
