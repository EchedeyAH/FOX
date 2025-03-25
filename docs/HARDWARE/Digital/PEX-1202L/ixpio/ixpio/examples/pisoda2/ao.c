/* Example of analog output for PISO-DA2.

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

   This example shows the analog output by basic register read/write.

   v 0.0.0 10 Dec 2002 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpio.h"

#define DA_CHANNEL_0 0x00;  /* channel 0 */
#define DA_CHANNEL_1 0x40;  /* channel 1 */

int main()
{
	int fd;
	char *dev_file;
	ixpio_reg_t ao1l, ao1h;
	unsigned int ao_value;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	ao1l.id = IXPIO_DA1_L;
	ao1h.id = IXPIO_DA1_H;
	ao_value = 0;

	puts("Press <enter> for next, ESC to exit.");
	while (getchar() != 27) {
					
		ao1l.value = ao_value & 0xff;
		ao1h.value = (ao_value >> 8) | DA_CHANNEL_0;

		if (ioctl(fd, IXPIO_WRITE_REG, &ao1l) ||
			ioctl(fd, IXPIO_WRITE_REG, &ao1h)) {
			close(fd);
			puts("Failure of analog output.");
		}
		printf("Analog output: 0x %02x %02x    <Enter> next, ESC exit ", ao1h.value, ao1l.value);

		if (ao_value == 0x3fff) ao_value = 0;
		else ao_value = (ao_value << 1) + 1;

	}

	close(fd);
	puts("\nEnd of program.");
	return SUCCESS;
}
