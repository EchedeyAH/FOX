/* Example of I/O port for PISO-730.

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

   This is an example for 16-bit I/O.
   Connect IDI[0:15] to IDO[0:15], DI[0:15] to DO[0:15].

   v 0.1.0  3 May 2002 by Reed Lai
     Remove the redundant code for ACR.

   v 0.0.0 29 Apr 2002 by Reed Lai
     Create. */

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
	ixpio_reg_t id_o, id_i, d_o, d_i;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	id_o.id = id_i.id = IXPIO_IDIO;
	id_o.value = 0;

	d_o.id  = d_i.id = IXPIO_DIO;
	d_o.value = 0;

	puts("Enter to continue, ESC to exit.");
	while (getchar() != 27) {
		if (ioctl(fd, IXPIO_WRITE_REG, &id_o) ||
			ioctl(fd, IXPIO_WRITE_REG, &d_o)) {
			puts("Failure of IXPIO_WRITE_REG: DIO.");
			break;
		}

		usleep(10);

		if (	ioctl(fd, IXPIO_READ_REG, &id_i) ||
			ioctl(fd, IXPIO_READ_REG, &d_i)) {
			puts("Failure of IXPIO_READ_REG: DIO.");
			break;
		}
		printf("IDO[0:15]=0x%04x, DO[0:15]=0x%04x\nIDI[0:15]=0x%04x, DI[0:15]=0x%04x\n", id_o.value, d_o.value, id_i.value, d_i.value);
		--id_o.value;
		--d_o.value;
	}
	close(fd);
	return SUCCESS;
}
