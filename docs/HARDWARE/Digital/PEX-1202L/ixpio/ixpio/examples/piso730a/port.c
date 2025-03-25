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

   This is an example for byte I/O.
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
	ixpio_reg_t idol, idoh, idil, idih, dol, doh, dil, dih;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	idol.id = idil.id = IXPIO_IDIO_L;
	idoh.id = idih.id = IXPIO_IDIO_H;
	idol.value = idoh.value = 0;

	dol.id  = dil.id = IXPIO_DIO_L;
	doh.id  = dih.id = IXPIO_DIO_H;
	dol.value = doh.value = 0;

	puts("Enter to continue, ESC to exit.");
	while (getchar() != 27) {
		if (	ioctl(fd, IXPIO_WRITE_REG, &idol) ||
			ioctl(fd, IXPIO_WRITE_REG, &idoh) ||
			ioctl(fd, IXPIO_WRITE_REG, &dol)  ||
			ioctl(fd, IXPIO_WRITE_REG, &doh)  ) {
			
			puts("Failure of IXPIO_READ/WRITE_REG: DIO.");
			break;
		}

		usleep(10);

		if (	ioctl(fd, IXPIO_READ_REG, &idil)  || 
			ioctl(fd, IXPIO_READ_REG, &idih)  ||
			ioctl(fd, IXPIO_READ_REG, &dil)   ||
			ioctl(fd, IXPIO_READ_REG, &dih)   ) {
			
			puts("Failure of IXPIO_READ/WRITE_REG: DIO.");
			break;
		}
		
		printf("IDO[0:7]=0x%02x, IDO[8:15]=0x%02x, DO[0:7]=0x%02x, DO[8:15]=0x%02x\nIDI[0:7]=0x%02x, IDI[8:15]=0x%02x, DI[0:7]=0x%02x, DI[8:15]=0x%02x\n", idol.value, idoh.value, dol.value, doh.value, idil.value, idih.value, dil.value, dih.value);
		++idol.value;
		++idoh.value;
		++dol.value;
		++doh.value;
	}
	close(fd);
	return SUCCESS;
}
