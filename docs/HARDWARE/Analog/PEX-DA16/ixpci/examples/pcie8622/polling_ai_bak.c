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

   v 0.0.0 1 Oct 2015 by Winson Chen
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpci.h"

int main(int argc, char **argv)
{
	int fd_dev, i, ai_range, AI_channels = 16, count, times = 10;
	ixpci_reg_t ar,aw;
	char *dev_file;
	unsigned int timeout = 0;
	float ai_value[AI_channels];
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* Clear FIFO 0x04 */
	aw.id = IXPCI_AI_CF;
	aw.value = 0;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &aw)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_CF.");
        }
	usleep(1);

	/* AI Scan Mode Control/Status 0x0c  */
	aw.id = IXPCI_AIGR;
	aw.value = 0x80;  //0x0: +5V~-5V  0x80: +10V~-10V

	if (ioctl(fd_dev, IXPCI_WRITE_REG, &aw)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AIGR.");
        }
	usleep(1);

	switch(aw.value & 0x80)
	{
		case 0:
			ai_range = 5.00;  //+5V~-5V
			break;
		case 0x80:
			ai_range = 10.00;  //+10V~-10V
			break;
	}

	printf("Analog Input Range :+%dV~-%dV\n",ai_range,ai_range);

	for(count = 0; count < times; count++)
	{
		/* Clear FIFO  0x04 */
		aw.id = IXPCI_AI_CF;
		aw.value = 0;
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &aw)){
			puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_CF.");
		}
		usleep(1);

		/* Software trigger 0x10 */
		aw.id = IXPCI_ADST;
		aw.value = 0x0;
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &aw)){
			puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_ADST.");
		}
		usleep(1);

		/* Read ADC status 0x0c */
		aw.id = IXPCI_AIGR;
	       	for(;;)
		{
			if (ioctl(fd_dev, IXPCI_READ_REG, &aw)){
				puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_AIGR.");
			}

			aw.value = aw.value & 0x200;
			if(!aw.value)  //ready
				break;

			if(timeout > 100000)
				break;

			timeout++;
		}

		for(i = 0; i < AI_channels; i++)
		{
			if(i % 16 <= 7 )
			{
				/* Read AI data 0x10 */
				ar.id = IXPCI_AI_L;
				if (ioctl(fd_dev, IXPCI_READ_REG, &ar)){
        		        	puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_AI.");
		        	}
			}
			else
			{
				/* Read AI data 0x14 */
				ar.id = IXPCI_AI_H;
        	                if (ioctl(fd_dev, IXPCI_READ_REG, &ar)){
                	                puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_AI.");
                        	}
			}
			if((float)ar.value / 32768.0 >= 1)  //minus
				ai_value[i] = ((float)ar.value / 32768 * ai_range - 2 * ai_range)*1.01;
			else  //plus
				ai_value[i] = ((float)ar.value / 32768 * ai_range) * 1.01;
			printf("%d. %.3f  ",i,ai_value[i]);
			if(i % 4 == 3)
				printf("\n");
		}
		printf("\n");
	}
	close(fd_dev);
	return SUCCESS;
}
