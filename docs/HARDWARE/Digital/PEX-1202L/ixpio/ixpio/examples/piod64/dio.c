/* Example of Digital I/O for PIO-D64.

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

   This example shows the digital output and input by basic register
   read/write.  You can loop back the DO to DI, connect the CN1 with CN2
   and CN3 with CN4 by two 20-pin cables to examine the digital I/O.

   v 0.0.0 10 Jul 2003 by Reed Lai
     Need something destroyed?... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpio.h"

int main()
{
	int fd;
	char *dev_file;
	ixpio_reg_t dina, dinb, dinc, dind;
	ixpio_reg_t douta, doutb, doutc, doutd;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	dina.id = douta.id = IXPIO_DIO_A;
	dinb.id = doutb.id = IXPIO_DIO_B;
	dinc.id = doutc.id = IXPIO_DIO_C;
	dind.id = doutd.id = IXPIO_DIO_D;
	douta.value = 1;
	doutb.value = doutc.value = doutd.value = 0;

	puts("Press <enter> for next, ESC to exit.");
	while (getchar() != 27) {
					
		if (ioctl(fd, IXPIO_WRITE_REG, &douta) ||
			ioctl(fd, IXPIO_WRITE_REG, &doutb) ||
			ioctl(fd, IXPIO_WRITE_REG, &doutc) ||
			ioctl(fd, IXPIO_WRITE_REG, &doutd)) {
			close(fd);
			puts("Failure of Digital output.");
		}
		if (ioctl(fd, IXPIO_READ_REG, &dina) ||
			ioctl(fd, IXPIO_READ_REG, &dinb) ||
			ioctl(fd, IXPIO_READ_REG, &dinc) ||
			ioctl(fd, IXPIO_READ_REG, &dind)) {
			close(fd);
			puts("Failure of Digital Input.");
		}
		printf("Digital Output: 0x %02x %02x %02x %02x  Input: 0x %02x %02x %02x %02x  <Enter> next, ESC exit ", doutd.value, doutc.value, doutb.value, douta.value, dind.value, dinc.value, dinb.value, dina.value);

		/* Rolling the bit */
		if (douta.value) {
			if (douta.value == 0x80) {
				douta.value = 0;
				doutb.value = 1;
			} else douta.value <<= 1;
		} else if (doutb.value) {
			if (doutb.value == 0x80) {
				doutb.value = 0;
				doutc.value = 1;
			} else doutb.value <<= 1;
		} else if (doutc.value) {
			if (doutc.value == 0x80) {
				doutc.value = 0;
				doutd.value = 1;
			} else doutc.value <<= 1;
		} else if (doutd.value) {
			if (doutd.value == 0x80) {
				doutd.value = 0;
				douta.value = 1;
			} else doutd.value <<= 1;
		}

	}

	close(fd);
	puts("\nEnd of program.");
	return SUCCESS;
}
