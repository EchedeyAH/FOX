/* Example of PortC Pattern function for PCI-D96/128SU

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

   v 0.0.0  8 Jul 2020 by Winson Chen
     Set PC as DO port and PB as DI port, 
     Enable PC pattern output

     This demo still testing, not finish.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpci.h"

int kbhit(void)
{
	struct timeval tv; fd_set read_fd;

	/* Do not wait at all, not even a microsecond */
	tv.tv_sec=0;
	tv.tv_usec=0;

	/* Must be done first to initialize read_fd */
	FD_ZERO(&read_fd);

	/* Makes select() ask if input is ready: 0 is the file descriptor for stdin */
	FD_SET(0, &read_fd);

	/* The first parameter is the number of the largest file descriptor to check + 1. */
	if (select(1, &read_fd, NULL /*No writes*/, NULL /*No exceptions*/, &tv) == -1)
	return 0; /* An error occured */

	/* read_fd now holds a bit map of files that are readable.
	* We test the entry for the standard input (file 0). */
	if (FD_ISSET(0, &read_fd)) /* Character pending on stdin */
	return 1;

	/* no characters were pending */
	return 0;
}

int main()
{
	char *dev_file;
	int fd_dev, index, min = 0, max = 512, offset;
	ixpci_reg_t reg;
	unsigned int dwDOVal[512];
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	//Set Digital Output Pattern
	for(index = 0; index < 512; index++)
	{
		dwDOVal[index] = index & 0xff;
		//printf(" 0x%x ",dwDOVal[index]);
	}

	/* Start DO pattern */
	//Write DO Pattern
	offset = 0;
	for(index = min; index < max; index++)
	{
		reg.id = IXPCI_PDS;
		reg.value = dwDOVal[index];
		reg.id_offset = offset * 4;
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
			puts("Failure of ioctl command IXPCI_WRITE_REG.");
			close(fd_dev);
			return FAILURE;
        	}
		offset++;
	}

	//Check DO Pattern
	offset = 0;
	for(index = min; index < max; index++)
	{
		reg.id = IXPCI_PDS;
		reg.id_offset = offset * 4;
	        if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
        	        puts("Failure of ioctl command IXPCI_READ_REG.");
                	close(fd_dev);
	                return FAILURE;
        	}
		offset++;
		//printf("reg.value is 0x%x dwDOVal[%d] value is 0x%x\n",reg.value, index, dwDOVal[index]);

		if(reg.value !=  dwDOVal[index])
		{
			printf("Check DO Pattern fail!\n");
			return FAILURE;
		}

	}

	//Set PC as DO port
	reg.id = IXPCI_DIOPC_CR;
        reg.value = 0xf;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }

	//Set PC Pattern CLK
	reg.id = IXPCI_PPC;
        reg.value = 100;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }
	
	//Set pattern number	
	reg.id = IXPCI_PPN;
        reg.value = max - 1;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }

	//Pattern Start
	reg.id = IXPCI_PPO;
        reg.value = 1;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }

	for(;;)
	{
		if(kbhit())
		{
			break;
		}
	}
	

	/* Stop DO pattern */
	//Set PC as DI port
	reg.id = IXPCI_DIOPC_CR;
        reg.value = 0xf;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }

	//Set PC Pattern CLK to 0
	reg.id = IXPCI_PPC;
        reg.value = 0;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }
	
	//Set pattern number to 0
	reg.id = IXPCI_PPN;
        reg.value = 0;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }

	//Pattern Stop
	reg.id = IXPCI_PPO;
        reg.value = 0;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }

	close(fd_dev);
        puts("\nEnd of program.");
	return SUCCESS;
}
