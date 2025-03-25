/* Example of Analog I/O for PCIe-8622

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

   v 0.0.0 25 Sep 2015 by Winson Chen
     create, blah blah... */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpci.h"

void usage();

int main(int argc, char **argv)
{
	int fd_dev, ch, gain, AOH;
	ixpci_reg_t ao;
	char *dev_file;
	unsigned int timeout = 0;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	if(argc == 4)
	{
		ch = atoi(argv[1]);
		gain = atoi(argv[2]);
		AOH = atoi(argv[3]);
	}
	else
	{
		usage();
		return 0;
	}

	ao.id = IXPCI_AO;
	ao.value = 0x80000;  //Output Range Select Register

	if(ch)  //Select DAC channel
		ao.value |= 0x20000;

	switch(gain)  //Analog output range;
	{
	case 1:
		ao.value = ao.value | 0x0;  //0~5V
		break;
	case 2:
		ao.value = ao.value | 0x1;  //0~10V
		break;
	case 3:
		ao.value = ao.value | 0x3;  //-5~+5V
		break;
	case 4:
		ao.value = ao.value | 0x4;  //-10~+10V
		break;
	default:
		printf("Wrong AO output range mode.\n");
		return FAILURE;
	}

	if (ioctl(fd_dev, IXPCI_WRITE_REG, &ao)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AO.");
		return FAILURE;
	}
	usleep(1);

	while(1)
	{
		/* Read AO command status */
		if (ioctl(fd_dev, IXPCI_READ_REG, &ao)){
                	puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_AO.");
        	}

		if((ao.value & 0x1000000) == 0)
			break;
		timeout++;

		if(timeout > 1000000)
			return FAILURE;	//timeout
	}

	//ao.id = IXPCI_AO;
	ao.value = 0x190005;  //Control register

	if (ioctl(fd_dev, IXPCI_WRITE_REG, &ao)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AO.");
		return FAILURE;
	}
	usleep(1);

	//ao.id = IXPCI_AO;
	ao.value = 0x10000f;  //Power control register

	if (ioctl(fd_dev, IXPCI_WRITE_REG, &ao)){
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AO.");
	}
	usleep(1);

	//ao.id = IXPCI_AO;
	ao.value = AOH;  //Analog output value

	if(ch)  //Select DAC channel
		ao.value = ao.value | 0x20000;

	if (ioctl(fd_dev, IXPCI_WRITE_REG, &ao)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AO.");
        }

	printf("OK\n");

	close(fd_dev);
	return SUCCESS;
}

void usage()
{
	printf("Usage: Set AO channel, output range and AO Hex value\n");
	printf("       Channels: 2 (0 and 1)\n");
	printf("       Output range mode:1. 0~5V\n");
	printf("			 2. 0~10V\n");
	printf("			 3. -5V~+5V\n");
	printf("			 4. -10V~+10V\n");
	printf("       AO Hex value: 0~65535\n");
	printf("Example: Set channel 0, output range 0~5V, AO Hex value 65535\n");
	printf("	 #./aoh 0 1 65535\n");
}
