/* Example of I/O port for PIO-D96.

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

   v 0.0.0 27 Nov 2002 by Reed Lai
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
	ixpio_reg_t reg0, reg1, reg2;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* setup I/O Selection Control Register
	   Port 0,2 as input port
	   Port 1 as output port */
	reg0.id = IXPIO_PCA;
	reg0.value = 0x02;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg0)) {
		printf("Failure of ioctl command IXPIO_WRITE_REG IXPIO_PC.\n");
		close(fd);
		return FAILURE;
	}

	/* write Port1, read Port0, Port2 */
	reg0.id = IXPIO_P0;
	reg1.id = IXPIO_P1;
	reg2.id = IXPIO_P2;
	reg1.value = 1;

	/* read, get out if error or ESC pressed */
	while (getchar() != 27) {
		if( ioctl(fd, IXPIO_WRITE_REG, &reg1) ||
			ioctl(fd, IXPIO_READ_REG, &reg0) ||
			ioctl(fd, IXPIO_READ_REG, &reg2) ) {
			puts("Failure of IXPIO_READ/WRITE_REG");
			close(fd);
			return FAILURE;
		}
		printf("Port0 = 0x%x,\tPort1 = 0x%x,\tPort2 = 0x%x.\n", reg0.value, reg1.value, reg2.value);
		(reg1.value == 8) ? (reg1.value = 1) : (reg1.value <<= 1);
	}
	puts("End of program");

	close(fd);
	return SUCCESS;
}
