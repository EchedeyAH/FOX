/* Example of Digital I/O for PISO-P8R8/P8SSR8AC/P8SSR8DC.

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

   v 0.0.0 25 Nov 2002 by Reed Lai
     steal from ixISA P8R8DIO, hee hee... */

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
	ixpio_reg_t ri, ro;

	dev_file = "/dev/ixpio1";

	/* Open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of openning device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* DIO */
	ro.id = IXPIO_DO;
	ro.value = 1;
	ri.id = IXPIO_DI;
	while (getchar() == 10) {
		if (ioctl(fd, IXPIO_WRITE_REG, &ro)) {
			puts("Failure of digital output.");
			break;
		}

		usleep(5000); /* 5ms for relay operating time */

		if (ioctl(fd, IXPIO_READ_REG, &ri)) {
			puts("Failure of digital input.");
			break;
		}
		printf("Relay Output = 0x%2x\tDI =0x%2x\tEsc to exit.\n", ro.value, ri.value);
		(ro.value < 0x80) ? (ro.value <<= 1) : (ro.value = 1);
	}

	close(fd);
	return SUCCESS;
}
