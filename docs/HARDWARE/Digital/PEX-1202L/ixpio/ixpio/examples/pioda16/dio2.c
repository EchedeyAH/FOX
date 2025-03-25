/* Example of Digital I/O for PIO-DA16/DA8/DA4.

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

   This example shows the digital output and input by ioctl digital
   commands.  You can connect the CON1 and CON2 with a 20-pin cable
   to examine the output.

   v 0.0.0  5 Dec 2002 by Reed Lai
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
	int fd;
	char *dev_file;
	ixpio_digital_t din, dout;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	dout.data.u16 = 1;
	din.data.u16 = 0;

	puts("Press <enter> for next, ESC to exit.");
	while (getchar() != 27) {
					
		if (ioctl(fd, IXPIO_DIGITAL_OUT, &dout)) {
			close(fd);
			puts("Failure of Digital output.");
		}
		if (ioctl(fd, IXPIO_DIGITAL_IN, &din)) {
			close(fd);
			puts("Failure of Digital Input.");
		}
		printf("Digital Output: 0x%04x    Input: 0x%04x    <Enter> next, ESC exit ", dout.data.u16, din.data.u16);

		if (dout.data.u16 == 0x8000) {
			dout.data.u16 = 1;
		} else dout.data.u16 <<= 1;

	}

	close(fd);
	puts("\nEnd of program.");
	return SUCCESS;
}
