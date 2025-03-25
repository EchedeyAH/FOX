/* Example of analog input with software trigger for PIO-821.

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

   Analog input range -5V ~ +5V, giain x1

   v 0.0.0  24 Feb 2004 by Emmy Tsai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpio.h"

int main()
{
	int fd, channel, gain;
	char *dev_file;
	ixpio_reg_t adcr, ai, admcr, adtcr;
	float av;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	gain = 0;	/* gain x 1 */
	channel = 0;	/* channel 0 */
	adcr.id = IXPIO_ADGCR;
	adcr.value = (channel << 2) + gain; 
	if (ioctl(fd, IXPIO_WRITE_REG, &adcr)) {
		puts("Failure of writing A/D Gain Control Register.");
		close(fd);
		return FAILURE;
	}
	usleep(23); /* Gains control settling time */

	admcr.id = IXPIO_ADMCR;
	admcr.value = 0xf1;	/* select trigger mode = Software-trigger */
	if (ioctl(fd, IXPIO_WRITE_REG, &admcr)) {
		puts("Failure of writing A/D Mode Control Register.");
		close(fd);
		return FAILURE;
	}
	
	adtcr.id = IXPIO_ADTCR;
	adtcr.value = 0x01;	/* A/D software trigger */
	while (getchar() != 27) {
		if (ioctl(fd, IXPIO_WRITE_REG, &adtcr)) {
			puts("Failure of writing A/D Software Trigger Control Register.");
			close(fd);
			return FAILURE;
		}
		
		ai.id = IXPIO_ADS; /* read AD status */
		if (ioctl(fd, IXPIO_READ_REG, &ai)) {
			puts("A/D data not ready.");
			break;
		}

		ai.id = IXPIO_AD;
		if (ioctl(fd, IXPIO_READ_REG, &ai)) {
			puts("Failure of read A/D buffer.");
			close(fd);
			return FAILURE;
		}
		av = 10 * (float)ai.value / 0xfff - 5;
		printf("ADC read = 0x%04x ==> %fV\tESC exit\n", ai.value, av);
	}

	admcr.value = 0xf0;	/* close mode */
	if ( ioctl(fd, IXPIO_WRITE_REG, &admcr)) {
		puts("Failure of writing A/D Mode Control Register.");
		close(fd);
		return FAILURE;
	}
	
	close(fd);
	return SUCCESS;
}
