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

   This example uses the virtual register IXPIO_DIO to read/write the four
   digital groups (D0-D31) at one sweep. You can loop back the DO to DI,
   connect the CN1 with CN2 and CN3 with CN4 by two 20-pin cables to examine
   the digital I/O.

   v 0.0.0 10 Jul 2003 by Reed Lai
     I copy that... */

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
	ixpio_reg_t din, dout;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	din.id = dout.id = IXPIO_DIO;
	dout.value = 1;

	puts("Press <enter> for next, ESC to exit.");
	while (getchar() != 27) {
					
		if (ioctl(fd, IXPIO_WRITE_REG, &dout)) {
			close(fd);
			puts("Failure of Digital output.");
		}
		if (ioctl(fd, IXPIO_READ_REG, &din)) {
			close(fd);
			puts("Failure of Digital Input.");
		}
		printf("Digital Output: 0x%08x    Input: 0x%08x    <Enter> next, ESC exit ", dout.value, din.value);

		if (dout.value == 0x80000000) dout.value = 1;
		else dout.value <<= 1;
	}

	close(fd);
	puts("\nEnd of program.");
	return SUCCESS;
}
