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

   This example shows the digital output and input by ioctl digital
   commands.  You can connect the CN1 with CN2 and CN3 with CN4 by
   two 20-pin cables to examine the output.

   v 0.0.0 10 Jul 2003 by Reed Lai
     May I take your order? */

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

	dout.data.u32 = 1;

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
		printf("Digital Output: 0x%08x    Input: 0x%08x    <Enter> next, ESC exit ", dout.data.u32, din.data.u32);

		if (dout.data.u32 == 0x80000000) {
			dout.data.u32 = 1;
		} else dout.data.u32 <<= 1;

	}

	close(fd);
	puts("\nEnd of program.");
	return SUCCESS;
}
