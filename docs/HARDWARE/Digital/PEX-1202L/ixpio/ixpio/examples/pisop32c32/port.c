/* Example of I/O port for PISO-P32C32.

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

   This is an example for the 32-bit I/O.
   Connect IDI[0:15] to IDO[0:15], DI[0:15] to DO[0:15].

   v 0.0.0  3 May 2002 by Reed Lai
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
	ixpio_reg_t d_o, d_i;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	d_o.id  = d_i.id = IXPIO_DIO;
	d_o.value = 0;

	puts("Enter to continue, ESC to exit.");
	while (getchar() != 27) {
		if (ioctl(fd, IXPIO_WRITE_REG, &d_o)) {
			puts("Failure of IXPIO_READ/WRITE_REG: DIO.");
			break;
		}

		sleep(1);
                if ( ioctl(fd, IXPIO_READ_REG, &d_i)) {
                        puts("Failure of IXPIO_READ/WRITE_REG: DIO.");
                        break;
                } 
		printf("DO[0:31]=0x%08x\nDI[0:31]=0x%08x\n", d_o.value, d_i.value);
		--d_o.value;
	}
	close(fd);
	return SUCCESS;
}
